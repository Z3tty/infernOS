#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMG_FILE "./image"
#define ARGS "[--extended] [--vm] [--fs] [--kernel] <boot> <exec> ..."
#define SECTOR_SIZE     512
#define OS_SIZE_LOC       2
#define BOOT_MEM_LOC 0X7c00
#define OS_MEM_LOC   0x8000
// macro's for aligning up/down to page bound by masking and aligning
#define ALIGN_PAGE_DOWN(addr) ((addr)&0xfffff000)
#define ALIGN_PAGE_UP(addr) (((addr)&0xfffff000) + 0x1000)
// cl opts
static struct {
    int vm;
    int extended;
    int kernel;
    int fs;
} options;
// Proc dir entry
struct directory_t {
    int location;
    int size;
};
static struct image_t {
    FILE *img; // FP to IMG
    FILE *fp;  // FP to INPUT
    int bytes; // bytes written
    int offset;// offset of virt addr from phys addr
    int pdloc; // location of next proc dir entry
    int pdlim; // upper limit for proc dir (1 sector)
    struct directory_t dir;
} image;

static void create_images(int nfiles, char *files[]);
static void create_image(struct image_t *img, char *filename);
static void error(char *fmt, ...);
static void read_ehdr(Elf32_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph, Elf32_Ehdr ehdr);
static void write_segment(struct image_t *img, Elf32_Ehdr ehdr, Elf32_Phdr phdr);
static void write_os_size(struct image_t *img);
static void prep_proc_dir(struct image_t *img);
static void write_proc_dir(struct image_t *img);
static void proc_start(struct image_t *img, int vaddr);
static void proc_end(struct image_t *img);
static void reserve_fsblock(struct image_t *img, int blocks);

int main(int argc, char **argv) {
    char *name = argv[0];
    options.vm = 0;
    options.extended = 0;
    options.kernel = 0;
    options.fs = 0;
    while ((argc > 1) && (argv[1][0] == '-')) {
        char *opt = &argv[1][2];
        if (strcmp(opt, "vm") == 0) options.vm = 1;
        else if (strcmp(opt, "extended") == 0) options.extended = 1;
        else if (strcmp(opt, "kernel") == 0) options.kernel = 1;
        else if (strcmp(opt, "fs") == 0) options.fs = 1;
        else error("%s: invalid option\nusage: %s %s\n", name, name, ARGS);
        argc--; argv++;
    }
    if (argc < 2 + options.kernel) error("usage: %s %s\n", name, ARGS);
    create_images(argc - 1, argv + 1);
}

static void create_images(int nfiles, char *files[]) {
    image.img = fopen(IMG_FILE, "w");
    assert(image.img != NULL);
    image.bytes = 0;
    create_image(&image, *files); // Boot
    nfiles--; files++;
    if (options.vm == 1) {
        if (options.kernel == 1) {
            create_image(&image, *files);
            nfiles--; files++;
        }
        // if vm, os size is only kernel size
        write_os_size(&image);
        prep_proc_dir(&image);
    } else {
        // if no vm kernel is dealt with like proc's
    }
    // Reserve for fs
    if (options.fs == 1) reserve_fsblock(&image, 512 + 2);
    while (nfiles > 0) {
        create_image(&image, *files);
        nfiles--; files++;
        // If vm, update proc dir
        if (options.vm == 1) write_proc_dir(&image);
    }
    // if no vm, os size includes processes
    if (options.vm == 0) write_os_size(&image);
    // Ensure alignment
    assert((image.bytes % SECTOR_SIZE) == 0);
    fclose(image.img);
}

static void create_image(struct image_t *img, char *filename) {
    int ph;
    Elf32_Ehdr ehdr;
    Elf32_Phdr phdr;
    // Open IN
    img->fp = fopen(filename, "r");
    assert(img->fp != NULL);
    // Read header
    read_ehdr(&ehdr, img->fp);
    printf("0x%04x: %s\n", ehdr.e_entry, filename);
    // each header
    for (ph = 0; ph < ehdr.e_phnum; ph++) {
        // Read header
        read_phdr(&phdr, img->fp, ph, ehdr);
        if (ph == 0) proc_start(img, phdr.p_vaddr);
        // write to image
        write_segment(img, ehdr, phdr);
    } proc_end(img);
    fclose(img->fp);
}

static void read_ehdr(Elf32_Ehdr *ehdr, FILE *fp) {
    int ret = fread(ehdr, sizeof(*ehdr), 1, fp); assert(ret == 1);
    assert(
        ehdr->e_ident[EI_MAG1] == 'E' &&
        ehdr->e_ident[EI_MAG1] == 'L' &&
        ehdr->e_ident[EI_MAG1] == 'F'
    );
}

static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph, Elf32_Ehdr ehdr) {
    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    int ret = fread(phdr, sizeof(*phdr), 1, fp); assert(ret == 1);
    if (options.extended == 1) {
        printf("\tsegment %d\n", ph);
		printf("\t\toffset 0x%04x", phdr->p_offset);
		printf("\t\tvaddr 0x%04x\n", phdr->p_vaddr);
		printf("\t\tfilesz 0x%04x", phdr->p_filesz);
		printf("\t\tmemsz 0x%04x\n", phdr->p_memsz);
    }
}

static void proc_start(struct image_t *img, int vaddr) {
    // Called once per proc, img bytes should be aligned to sec bound
    // before call
    assert((img->bytes % SECTOR_SIZE) == 0);
    img->dir.location = img->bytes / SECTOR_SIZE;
    if (img->bytes == 0) {
        // First image has to be boot
        // vaddr has to be 0
        assert(vaddr == 0);
        img->offset = 0;
    }
    // if no vm, place image according to vaddr
    else if (options.vm == 0) img->offset = -OS_MEM_LOC + SECTOR_SIZE;
    // if vm, start from next sector
    else img->offset = img->bytes - ALIGN_PAGE_DOWN(vaddr);
}

static void proc_end(struct image_t *img) {
    // Pad after each proc
    if (img->bytes % SECTOR_SIZE != 0) {
        while (img->bytes % SECTOR_SIZE != 0) {
            fputc(0, img->img); (img->bytes)++;
        }
        if (options.extended == 1) 
            printf("\t\tpadding up to 0x%04x\n", img->bytes);
    }
    // set size in sectors of curr proc img
    img->dir.size = img->bytes / SECTOR_SIZE - img->dir.location;
    if (options.extended == 1)
		printf(
            "\tProcess starts at sector %d, and spans for "
		    "%d sectors\n",
		    img->dir.location, img->dir.size
        );
}

static void write_segment(struct image_t *img, Elf32_Ehdr ehdr, Elf32_Phdr phdr) {
    // If nothing to write, end
    if (phdr.p_memsz == 0) return;
    int phyaddr = phdr.p_vaddr + img->offset;
    if (phyaddr < img->bytes) error("! Memory conflict\n");
    // pad before each segment
    if (img->bytes < phyaddr) {
        while(img->bytes < phyaddr) {
            fputc(0, img->img); (img->bytes)++;
        }
        if (options.extended == 1)
			printf("\t\tpadding up to 0x%04x\n", phyaddr);
    }
    // write
    if (options.extended == 1)
		printf("\t\twriting 0x%04x bytes\n", phdr.p_memsz);
    fseek(img->fp, phdr.p_offset, SEEK_SET);
    while (phdr.p_filesz-- > 0) {
        fputc(fgetc(img->fp), img->img);
        (img->bytes)++;
        phdr.p_memsz--;
    }
    while (phdr.p_memsz-- > 0) {
        fputc(0, img->img);
        (img->bytes)++;
    }
}

static void write_os_size(struct image_t *img) {
    // each image must be sector aligned
    assert((img->bytes % SECTOR_SIZE) == 0);
    // -1 for boot
    short os_size = img->bytes / SECTOR_SIZE - 1;
    fseek(img->img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&os_size, sizeof(os_size), 1, img->img);
    if (options.extended == 1)
        printf("os_size: %d sectors\n", os_size);
    // move fp to eof to allow writing beyond
    fseek(img->img, 0, SEEK_END);
}

static void prep_proc_dir(struct image_t *img) {
    // once again, image MUST be sector aligned
    assert((img->bytes % SECTOR_SIZE) == 0); assert(options.vm);
    img->pdloc = img->bytes; img->pdlim = img->bytes + SECTOR_SIZE;
    // leave 1 sector for proc dir
    int i;
    for (i = 0; i < SECTOR_SIZE; i++) {
        fputc(0, img->img); (img->bytes)++;
    }
}

static void write_proc_dir(struct image_t *img) {
    assert(options.vm);
    if (img->pdloc + sizeof( struct directory_t ) >= img->pdlim)
        error("Too many processes");
    fseek(img->img, img->pdloc, SEEK_SET);
    fwrite(&img->dir, sizeof( struct directory_t ), 1, img->img);
    // go next
    img->pdloc = ftell(img->img);
    fseek(img->img, 0, SEEK_END);
}

static void reserve_fsblock(struct image_t *img, int blocks) {
    fseek(img->img, 0, SEEK_END);
    int left = blocks * SECTOR_SIZE + 1;
    while (--left)
        if (fputc(0, img->img) == EOF) break;
    if (left) error ("Unable to reserve %d blocks\n", blocks);
    img->bytes += blocks * SECTOR_SIZE;
    if (options.extended == 1)
		printf("Reserved %d blocks for the filesystem\n", blocks);
}

static void error(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (errno != 0) perror(NULL);
	exit(EXIT_FAILURE);
}