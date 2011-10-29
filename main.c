#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "myar.h"

#define MODE_NONE			0
#define MODE_APPEND_ALL 	1
#define MODE_DELETE			2
#define MODE_APPEND			3
#define MODE_CONCISE_TABLE	4
#define MODE_VERBOSE_TABLE	5
#define MODE_EXTRACT		6

void string_copy(void **dst, void *src) {
	assert(src);
	assert(dst);

	if (*dst != NULL) {
		free(*dst);
	}

	*dst = (void *)malloc((strlen((char *)src) + 1) * sizeof(char));
	strcpy((char *)*dst, (char *)src);
	assert(!strcmp((char *)*dst, src));
}

void usage(void);

void verbose_table(const char *path);

int main(int argc, char **argv) {
	struct List files;
	char *archive_path = NULL;
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

	if (mode == MODE_NONE) {
		usage();
	}
	
	list_init(&files, string_copy, free);

	while (optind < argc) {
		if (archive_path == NULL) {
			archive_path = (char *)malloc((strlen(argv[optind]) + 1) * sizeof(char));
			if (archive_path == NULL) {
				perror(NULL);
				exit(0);
			}
			
			strcpy(archive_path, argv[optind++]);
		} else {
			list_add_back(&files, argv[optind++]);
		}
	}

	if (archive_path == NULL) {
		usage();
	}

	switch (mode) {
	case MODE_VERBOSE_TABLE:
		verbose_table(archive_path);
		break;
	}
	
	list_free(&files);

	return 0;
}

void verbose_table(const char *path) {
	struct ar a;

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		printf("Fail\n");
	} else {
		ar_print(&a);
	}

	ar_free(&a);
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

