#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MODE_NONE		0
#define MODE_APPEND_ALL 	1
#define MODE_DELETE		2
#define MODE_APPEND		3
#define MODE_CONCISE_TABLE	4
#define MODE_VERBOSE_TABLE	5
#define MODE_EXTRACT		6

void usage(void);

int main(int argc, char **argv) {
	char *archivePath = NULL;
	char **files = NULL;
	int c;
	int mode = MODE_NONE;
	while ((c = getopt(argc, argv, "Adqtvx")) != -1) {
		switch (c) {
		case 'A':
			if (mode != MODE_NONE) {
				usage();
			}
			
			mode = MODE_APPEND_ALL;
			break;
		case 'd':
			if (mode != MODE_NONE) {
				usage();
			}
			
			mode = MODE_DELETE;
			break;
		case 'q':
			if (mode != MODE_NONE) {
				usage();
			}
			
			mode = MODE_APPEND;
			break;
		case 't':
			if (mode != MODE_NONE) {
				usage();
			}
			
			mode = MODE_CONCISE_TABLE;
			break;
		case 'v':
			if (mode != MODE_NONE) {
				usage();
			}
			
			mode = MODE_VERBOSE_TABLE;
			break;
		case 'x':
			if (mode != MODE_NONE) {
				usage();
			}
			
			mode = MODE_EXTRACT;
			break;
		default:
			break;
		}
	}
	
	while (optind < argc) {
		if (archivePath == NULL) {
			archivePath = (char*)malloc((strlen(argv[optind]) * sizeof(char)) + 1);
			if (archivePath == NULL) {
				perror(NULL);
				exit(0);
			}
			
			strcpy(archivePath, argv[optind++]);
			
			printf("Archive file: %s\n", archivePath);
		} else {
			printf("File: %s\n", argv[optind++]);
		}
	}
	
	if (mode == MODE_NONE) {
		usage();
	}
}

void usage(void) {
	printf("Usage: myar {Adqtvx} archive-file file...\n");
	printf(" commands:\n");
	printf("  A\t- quick append all \"regular\" file(s) in the current directory\n");
	printf("  d\t- delete file(s) from the archive\n");
	printf("  q\t- quick append  file(s) to the archive\n");
	printf("  t\t- print a concise table of contents in the archive\n");
	printf("  v\t- print a verbose table of contents in the archive\n");
	printf("  x\t- extract named files\n");
	exit(0);
}

