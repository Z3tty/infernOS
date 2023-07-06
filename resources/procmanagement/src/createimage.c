/* creatimage.c
 */
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] [--kernel] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define OS_SIZE_LOC 2
#define BOOT_MEM_LOC 0x7c00
#define OS_MEM_LOC 0x8000

/* to align down to a page boundary, just mask off the last 12 bits */
#define ALIGN_PAGE_DOWN(addr) ((addr)&0xfffff000)

/* to align up to a page boundary, first align down and then add 4K to it */
#define ALIGN_PAGE_UP(addr) (((addr)&0xfffff000) + 0x1000)

/* structure to store command line options */
static struct {
	int vm;
	int extended;
	int kernel;
} options;

/* process directory entry */
struct directory_t {
	int location;
	int size;
};

/* */
static struct image_t {
	FILE *img; /* the file pointer to the image file */
	FILE *fp;  /* the file pointer to the input file */

	int nbytes; /* bytes written so far */
	int offset; /* offset of virtual address from physical address */

	int pd_loc; /* the location for next process directory entry */
	int pd_lim; /* the upper limit for process directory, one sector */
	struct directory_t dir;
} image;

/* prototypes of local functions */
static void create_images(int nfiles, char *files[]);
static void create_image(struct image_t *im, char *filename);

static void error(char *fmt, ...);
static void read_ehdr(Elf32_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph, Elf32_Ehdr ehdr);

static void write_segment(struct image_t *im, Elf32_Ehdr ehdr, Elf32_Phdr phdr);
static void write_os_size(struct image_t *im);
static void prepare_process_directory(struct image_t *im);
static void write_process_directory(struct image_t *im);

static void process_start(struct image_t *im, int vaddr);
static void process_end(struct image_t *im);

int main(int argc, char **argv) {
	char *progname = argv[0];

	/* process command line options */
	options.vm = 0;
	options.extended = 0;
	options.kernel = 0;
	while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
		char *option = &argv[1][2];

		if (strcmp(option, "vm") == 0) {
			options.vm = 1;
		}
		else if (strcmp(option, "extended") == 0) {
			options.extended = 1;
		}
		else if (strcmp(option, "kernel") == 0) {
			options.kernel = 1;
		}
		else {
			error("%s: invalid option\nusage: %s %s\n", progname, progname, ARGS);
		}
		argc--;
		argv++;
	}
	if (argc < 2 + options.kernel) {
		/* at least 3 args (createimage bootblock kernel) */
		error("usage: %s %s\n", progname, ARGS);
	}
	create_images(argc - 1, argv + 1);

	return 0;
}

static void create_images(int nfiles, char *files[]) {
	image.img = fopen(IMAGE_FILE, "w");
	assert(image.img != NULL);
	image.nbytes = 0;

	/* boot block */
	create_image(&image, *files);
	nfiles--;
	files++;

	if (options.vm == 1) {
		if (options.kernel == 1) {
			create_image(&image, *files);
			nfiles--;
			files++;
		}
		/* if with vm, the os size will be only the size of kernel, if
		 * any */
		write_os_size(&image);
		prepare_process_directory(&image);
	}
	else {
		/* no vm, the kernel is dealt with just the same as process */
	}

	while (nfiles > 0) {
		create_image(&image, *files);
		nfiles--;
		files++;
		if (options.vm == 1) {
			/* if using vm, update the process directory */
			write_process_directory(&image);
		}
	}

	if (options.vm == 0) {
		/* if there is no vm, the os size will include all the processes
		 */
		write_os_size(&image);
	}

	assert((image.nbytes % SECTOR_SIZE) == 0);
	fclose(image.img);
}

static void create_image(struct image_t *im, char *filename) {
	int ph;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;

	/* open input file */
	im->fp = fopen(filename, "r");
	assert(im->fp != NULL);

	/* read ELF header */
	read_ehdr(&ehdr, im->fp);
	printf("0x%04x: %s\n", ehdr.e_entry, filename);

	/* for each program header */
	for (ph = 0; ph < ehdr.e_phnum; ph++) {
		/* read program header */
		read_phdr(&phdr, im->fp, ph, ehdr);

		if (ph == 0)
			process_start(im, phdr.p_vaddr);

		/* Supposed to avoid including segments spanning unreasonably large ranges */
		if ((phdr.p_type != PT_LOAD) || !(phdr.p_flags & PF_X)) {
			printf("\t\tskipping...\n");
			continue;
		}
		/* write segment to the image */
		write_segment(im, ehdr, phdr);
	}
	process_end(im);

	fclose(im->fp);
}

static void read_ehdr(Elf32_Ehdr *ehdr, FILE *fp) {
	int ret;

	ret = fread(ehdr, sizeof(*ehdr), 1, fp);
	assert(ret == 1);
	assert(ehdr->e_ident[EI_MAG1] == 'E');
	assert(ehdr->e_ident[EI_MAG2] == 'L');
	assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph, Elf32_Ehdr ehdr) {
	int ret;

	fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
	ret = fread(phdr, sizeof(*phdr), 1, fp);
	assert(ret == 1);
	if (options.extended == 1) {
		printf("\tsegment %d\n", ph);
		printf("\t\toffset 0x%04x", phdr->p_offset);
		printf("\t\tvaddr 0x%04x\n", phdr->p_vaddr);
		printf("\t\tfilesz 0x%04x", phdr->p_filesz);
		printf("\t\tmemsz 0x%04x\n", phdr->p_memsz);
	}
}

static void process_start(struct image_t *im, int vaddr) {
	/* this function is called once for each process, im->nbytes should be
	   aligned to sector boundary before calling it, so let's check it */
	assert((im->nbytes % SECTOR_SIZE) == 0);
	im->dir.location = im->nbytes / SECTOR_SIZE;

	if (im->nbytes == 0) {
		/* this is the first image, it's got to be the boot block */
		/* The virtual address of boot block must be 0 */
		assert(vaddr == 0);
		im->offset = 0;
	}
	else if (options.vm == 0) {
		/* if there is no vm, put the image according to their virtual
		 * address */
		im->offset = -OS_MEM_LOC + SECTOR_SIZE;
	}
	else {
		/* using vm, then just start from the next sector */
		im->offset = im->nbytes - ALIGN_PAGE_DOWN(vaddr);
	}
}

static void process_end(struct image_t *im) {
	/* write padding after the process */
	if (im->nbytes % SECTOR_SIZE != 0) {
		while (im->nbytes % SECTOR_SIZE != 0) {
			fputc(0, im->img);
			(im->nbytes)++;
		}
		if (options.extended == 1) {
			printf("\t\tpadding up to 0x%04x\n", im->nbytes);
		}
	}
	/* the size, in sector, of the current process image */
	im->dir.size = im->nbytes / SECTOR_SIZE - im->dir.location;

	if (options.extended == 1) {
		printf("\tProcess starts at sector %d, and spans for %d sectors\n", im->dir.location, im->dir.size);
	}
}

static void write_segment(struct image_t *im, Elf32_Ehdr ehdr, Elf32_Phdr phdr) {
	int phyaddr;

	if (phdr.p_memsz == 0)
		return; /* nothing to write */
	/* find out the physical address */
	phyaddr = phdr.p_vaddr + im->offset;

	if (phyaddr < im->nbytes) {
		error("memory conflict\n");
	}

	/* write padding before the segment */
	if (im->nbytes < phyaddr) {
		while (im->nbytes < phyaddr) {
			fputc(0, im->img);
			(im->nbytes)++;
		}
		if (options.extended == 1) {
			printf("\t\tpadding up to 0x%04x\n", phyaddr);
		}
	}

	/* write the segment itself */
	if (options.extended == 1) {
		printf("\t\twriting 0x%04x bytes\n", phdr.p_memsz);
	}
	fseek(im->fp, phdr.p_offset, SEEK_SET);
	while (phdr.p_filesz-- > 0) {
		fputc(fgetc(im->fp), im->img);
		(im->nbytes)++;
		phdr.p_memsz--;
	}
	while (phdr.p_memsz-- > 0) {
		fputc(0, im->img);
		(im->nbytes)++;
	}

	/* Note: Modified by Han Chen
	   padding here is removed to process_end, this will allow two segments
	   to reside in the same sector */
}

static void write_os_size(struct image_t *im) {
	short os_size;

	/* each image must be padded to be sector-aligned */
	assert((im->nbytes % SECTOR_SIZE) == 0);

	/* -1 to account for the boot block */
	os_size = im->nbytes / SECTOR_SIZE - 1;
	fseek(im->img, OS_SIZE_LOC, SEEK_SET);
	fwrite(&os_size, sizeof(os_size), 1, im->img);
	if (options.extended == 1) {
		printf("os_size: %d sectors\n", os_size);
	}

	/* move the file pointer to the end of file
	   because we will write something else after it */
	fseek(im->img, 0, SEEK_END);
}

static void prepare_process_directory(struct image_t *im) {
	int i;

	/* each image must be padded to be sector-aligned */
	assert((im->nbytes % SECTOR_SIZE) == 0);
	assert(options.vm);

	im->pd_loc = im->nbytes;
	im->pd_lim = im->nbytes + SECTOR_SIZE;

	/* leave a sector for process directory */
	for (i = 0; i < SECTOR_SIZE; i++) {
		fputc(0, im->img);
		im->nbytes++;
	}
}

static void write_process_directory(struct image_t *im) {
	assert(options.vm);

	if (im->pd_loc + sizeof(struct directory_t) >= im->pd_lim) {
		error("Too many processes! Can't hold them in the directory!\n");
	}

	fseek(im->img, im->pd_loc, SEEK_SET);
	fwrite(&im->dir, sizeof(struct directory_t), 1, im->img);
	/* move the pd_loc to the next entry */
	im->pd_loc = ftell(im->img);

	fseek(im->img, 0, SEEK_END);
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
