/* A solution to towers of hanoi */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void towers(int n, int i, int j);

int step = 0;

int main(int argc, char *argv[]) {
	int i, j, n;

	if (argc < 4) /* to few arguments */
	{
		printf("Usage: hanoi disks source target\n"); /* printf & exit */
		return 0;
	}
	else {
		n = atoi(*++argv); /* 2nd argument */
		i = atoi(*++argv); /* 3rd argument */
		j = atoi(*++argv); /* 4th argument */
	}

	printf("/* process5.c\n *\n * The purpose of this process is to have a big process.\n * It is a solution to the towers of hanoi problem with %d disk.\n */\n", n);
	printf("#include \"common.h\"\n#include \"syslib.h\"\n#include \"util.h\"\n");
	printf("\n#define HANOI_DELAY_VAL 10000\n\n");
	printf("void _start(void)\n{\n");

	towers(n, i, j);

	printf("\n    exit();\n");
	printf("}");

	return 0;
}

/* --towers--
 * n = number of disk, i = starting stick, j = target stick
 */
void towers(int n, int i, int j) {
	int k;

	if (n == 1) {
		printf("    scrprintf(0, 0, \"Step %d: Move a disk from stick %d to stick %d\");\n", ++step, i, j);
		printf("    delay(HANOI_DELAY_VAL);\n");
	}
	else {
		k = 6 - i - j;       /* the other stick */
		towers(n - 1, i, k); /* move n-1 disks from i to k */
		towers(1, i, j);     /* move 1 disk from i to j */
		towers(n - 1, k, j); /* move n-1 disks from k to j */
	}
}
