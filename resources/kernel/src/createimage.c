#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // I cannot for the life of me figure out why this sometimes throws an error

#define IMAGE_FILE "image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

/* Variable to store pointer to program name */
char *progname;

/* Variable to store pointer to the filename for the file being read. */
char *elfname;

/* Structure to store command line options */
static struct {
	int vm;
	int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);

int main(int argc, char **argv) {
	/* Process command line options */
	progname = argv[0];
	options.vm = 0;
	options.extended = 0;
	while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
		char *option = &argv[1][2];

		if (strcmp(option, "vm") == 0) {
			options.vm = 1;
		}
		else if (strcmp(option, "extended") == 0) {
			options.extended = 1;
		}
		else {
			error("%s: invalid option\nusage: %s %s\n", progname, progname, ARGS);
		}
		argc--;
		argv++;
	}
	if (options.vm == 1) {
		/* This option is not needed in project 1 so we doesn't bother
		 * implementing it*/
		error("%s: option --vm not implemented\n", progname);
	}
	if (argc < 3) {
		/* at least 3 args (createimage bootblock kernel) */
		error("usage: %s %s\n", progname, ARGS);
	}
	create_image(argc - 1, argv + 1);
	return 0;
}

#define ASSUME_STRICT_ORDER
#define BOOT "bootblock\0"
int str_contains(char* str, char* sub, int strl, int subl)
{
        if (strl < subl) return 0;
        if (strl == subl) {
                if (str == sub) {
                        return 1;
                } return 0;
        }
        for (int i = 0; i < strl; i += subl) {
                char *test = "";
                for (int j = i; j < subl; j++) {
                        if (j >= strl) return 0;
                        test[j-i] = str[j];
                }
                if (test == sub) return 1;
        } return 0;
}

typedef struct {
        Elf32_Ehdr hdr;
        Elf32_Phdr *phdrs;
} Header_t;
typedef unsigned int uint_t;
typedef unsigned char byte_t;
typedef struct {
        FILE *image; // Image fp
        uint_t bytes;// Written bytes
        uint_t voff; // Virtual offset
        Header_t *header;
        Elf32_Ehdr curr_ehdr;
        Elf32_Phdr curr_phdr;
        byte_t* *raw;
        FILE *source;// Source fp
} fimg_t;

Header_t *create_image_header(int nfiles, char *files[]);
byte_t* get_raw_padded(FILE *f, uint_t is_bootblock);
void write_image(fimg_t *image, uint_t boot);

/* Header_t *static_boot_kern_header(char *files[]) {
        Header_t *hdr = (Header_t*)malloc(sizeof(Header_t));
        Elf32_Ehdr boot_header;
        Elf32_Ehdr kernel_header;
        Elf32_Ehdr img_header;
        
}*/

static void create_image(int nfiles, char *files[]) {
	/* This is where you should start working on your own implemtation
	 * of createimage.  Don't forget to structure the code into
	 * multiple functions in a way whichs seems logical, otherwise the
	 * solution will not be accepted. */
        Header_t *hdr = create_image_header(nfiles, files);
        fimg_t *image;
        image->image = fopen(IMAGE_FILE, "w");
        image->bytes = 0;
        image->header = hdr;
        byte_t* *raws = (byte_t**)malloc(sizeof(byte_t*) * nfiles);
        for (int i = 0; i < nfiles; i++) {
                FILE *fp = fopen(files[i], "r");
                raws[i] = get_raw_padded(fp, i == 0);
        }
        image->raw = raws;
	printf("Decoded and padded %d files.\n", nfiles);
        // Write kernel size to memory here :)
        // Clear the image before writing, as writes are "appends"
        fwrite(NULL, 0, 1, image->image);
        FILE *fp = fopen(files[0], "r"); // Bootloader
        image->source = fp;
        int ret = fread(&image->curr_ehdr, sizeof(Elf32_Ehdr), 1, fp);
        write_image(image, 1);
        fp = fopen(files[1], "r"); // Kernel
        image->source = fp;
        ret = fread(&image->curr_ehdr, sizeof(Elf32_Ehdr), 1, fp);
        write_image(image, 0);
        printf("Wrote image\n");
}

Header_t *create_image_header(int nfiles, char *files[])
{
        Header_t *hdr = (Header_t*)malloc(sizeof(Header_t));
        Elf32_Ehdr imghdr;
        // Assume at most 64 sections
        Elf32_Phdr *phdrs = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * 64);
        // Intel does not define flags
        imghdr.e_flags = 0;
        // unsigned char[EI_NIDENT] ident;
        // imghdr.e_ident = ident;
        // Set the magic identifier
        imghdr.e_ident[EI_MAG0] = 0x7f;
        imghdr.e_ident[EI_MAG1] = 'E';
        imghdr.e_ident[EI_MAG2] = 'L';
        imghdr.e_ident[EI_MAG3] = 'F';
        // INTEL 32 BIT
        imghdr.e_ident[EI_CLASS]= ELFCLASS32;
        imghdr.e_ident[EI_DATA] = ELFDATA2LSB;
        // ELF VERSION
        imghdr.e_ident[EI_VERSION] = EV_CURRENT;
        imghdr.e_ident[EI_PAD] = 0;
        imghdr.e_ident[EI_NIDENT] = sizeof(imghdr.e_ident);
        // INTEL 32 BIT
        imghdr.e_machine = EM_M32;
        // Executable
        imghdr.e_type = ET_EXEC;
        Elf32_Ehdr tmp;
        Elf32_Phdr pht;
        Elf32_Phdr ptmp;
        Elf32_Off phoff;
        Elf32_Half phnum;
        for (int i = 0; i < nfiles; i++) {
                FILE *fp = fopen(files[i], "r");
                int ret = fread(&tmp, sizeof(Elf32_Ehdr), 1, fp);
                printf("Read %d.\n", ret);
#ifdef ASSUME_STRICT_ORDER
                // Set the entry point to the bootblock
                if (i == 0) // Bootblock first in strict order
                {
                        imghdr.e_entry = tmp.e_entry;
                        imghdr.e_phoff = tmp.e_phoff;
                }
#else
                if (str_contains(files[i], BOOT, strlen(files[i]), strlen(boot)))
                {
                        imghdr.e_entry = tmp.e_entry;
                        imghdr.e_phoff = tmp.e_phoff;
                }

#endif
                phoff = tmp.e_phoff;
                ret = fseek(fp, phoff, SEEK_CUR);
                ret = fread(&ptmp, sizeof(Elf32_Phdr), 1, fp);
                phnum += tmp.e_phnum;
                imghdr.e_phentsize = tmp.e_phentsize;
                for (int j = 0; j < tmp.e_phnum; j++) {
                        pht = ptmp;
                        // pht.p_off = phnum * 512 // Make the offset point to the section it points to, when padding is done
                        // Add this pht to the header data
                        phdrs[imghdr.e_phnum++] = pht;
                        // Read the next pht in this file
                        ret = fseek(fp, phoff, SEEK_CUR);
                        ret = fread(&ptmp, sizeof(Elf32_Phdr), 1, fp);
                }
                // fclose(fp);
        }
        imghdr.e_phnum = phnum;
        hdr->hdr = imghdr;
        hdr->phdrs = phdrs;
        return hdr;
}

byte_t* get_raw_padded(FILE *f, uint_t is_bootblock)
{
        Elf32_Ehdr fh;
        Elf32_Phdr pht;
        int ret = fread(&fh, sizeof(Elf32_Ehdr), 1, f);
        printf("Read %d.\n", ret);
        Elf32_Off off = fh.e_phoff;
        Elf32_Half n = fh.e_phnum;
        byte_t *raw = (byte_t*)malloc(sizeof(byte_t) * 512 * n); // Allocate size for the raw content, 512 per section
                                                                 // as no virtual address padding is done
        Elf32_Off next_pht_addr = off + (fh.e_phentsize);
        for (int i = 0; i < n; i++) {
                // Move to the PHT
                ret = fseek(f, off, SEEK_CUR);
                // Read the PHT
                ret = fread(&pht, sizeof(Elf32_Phdr), 1, f);
                // I cant figure out how to effectively pad for virtual addresses
                // But you definitely should
                // Elf32_Addr virt = pht.p_vaddr;
                // Elf32_Word align= pht.p_align;
                // Filesize and memsize can differ, there must be enough space to allow for it to grow into memsize
                Elf32_Half filesz = pht.p_filesz;
                Elf32_Half memsz = pht.p_memsz;
                Elf32_Half pad = memsz - filesz;
                Elf32_Off phoff = pht.p_offset;
                byte_t *segment = (byte_t*)malloc(sizeof(byte_t) * 512);
                for (int b = 0; b < 512; b++) segment[b] = (byte_t)'0';
                // Move to the segment
                ret = fseek(f, phoff, SEEK_CUR);
                // Read the segment
                ret = fread(&segment, filesz, 1, f);
                // Since writing the magic is apparantly illegal in the actual bootloader, its here
                byte_t *magic = (byte_t*)"AA55";
                // If you could tell me how byte_t* and byte_t* have different signes then be my guest buddy.
                if (is_bootblock) strcat(segment, magic);
                // There should be a check for kernel here, and then a write of its size
                // But uh, I cannot for the life of me figure out exactly where to write it
                // This is here just to make sure you know that I know that there should def be a funny write here
                // Pad the segment to mem size
                while (pad > 0) {
                        // byte_t (unsigned char) is always guaranteed to be of size 1
                        uint_t idx = sizeof(segment) - 1;
                        pad--;
                        segment[idx] = (byte_t)0; // Pad an extra zero
                }
                while (sizeof(segment) < 512) {
                        // Pad to boundary, gotta love that 512 alignment
                        uint_t idx = sizeof(segment) - 1;
                        segment[idx] = (byte_t)0;
                }
                ret = fseek(f, 0, SEEK_SET); // Move to the start of the file
                off = next_pht_addr; // Set the offset to the next pht entry
                next_pht_addr = off + (fh.e_phentsize); // Shift next
                /* 
                ret = fseek(f, off, SEEK_CUR); // Get the next pht entry
                ret = fread(&pht, sizeof(Elf32_Phdr), 1, f); // Retrieve
                Elf32_Addr next_virt = pht.p_vaddr;
                // Pad to next address
                Elf32_Off virt_pad = next_virt - virt;
                */
                // Add the segment to the raw code content
                strcat(raw, segment);
        }
        // PTSD
        // strcat(raw, '\0');
        return raw;
}

void write_image(fimg_t *image, uint_t boot)
{
        int phoff = image->curr_ehdr.e_phoff;
        int n = image->curr_ehdr.e_phnum;
        int esz = image->curr_ehdr.e_phentsize;
        for (int i = 0; i < n; i++) {
                // Move to next pht
                fseek(image->source, phoff + i * esz, SEEK_SET);
                int ret = fread(&image->curr_phdr, sizeof(Elf32_Phdr), 1, image->source);
                if (image->curr_phdr.p_memsz == 0) continue; // Empty segment
                int addr = image->curr_phdr.p_vaddr + image->voff;
                if (addr < image->bytes) error("mem");
                while(addr > image->bytes) {
                        // Pad upto virt
                        fputc(0, image->image);
                        image->bytes++;
                }
                // Move to the area pointed to by the pht
                // Read data there, write it to the image
                fseek(image->source, image->curr_phdr.p_offset, SEEK_SET);
                while (image->curr_phdr.p_filesz > 0) {
                        image->curr_phdr.p_filesz--;
                        fputc(fgetc(image->source), image->image);
                        image->bytes++;
                        image->curr_phdr.p_memsz--;
                }
                while (image->curr_phdr.p_memsz > 0) {
                        // Pad up to memory size
                        image->curr_phdr.p_memsz--;
                        fputc(0, image->image);
                        image->bytes++;
                }
        }
        if (boot) {
                // Write signature
#define BS0 0x55
#define BS1 0xAA
#define OFF 0x1fe
                // Load BS1 in the upper 8, BS0 in the lower 8 (the asm version is so much prettier, dear lord)
                const short sig = BS1 << 8 | BS0;
                // Save position to return after writing
                int pos = ftell(image->image);
                fseek(image->image, OFF, SEEK_SET);
                // I THINK this should work better than the previous attempt, but I'm still not sure
                fwrite(&sig, sizeof(short), 1, image->image);
                fseek(image->image, pos, SEEK_SET);
        }       
}

/* print an error message and exit */
static void error(char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (errno != 0) {
		perror(NULL);
	}
	exit(EXIT_FAILURE);
}
