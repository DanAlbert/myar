/**
 * @file main.c
 * @author Dan Albert
 * @date Created 10/17/2011
 * @date Last updated 11/01/2011
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * @section DESCRIPTION
 * 
 * This program will illustrate the use of file I/O on UNIX by maintaining a
 * UNIX archive library, in the standard archive format.
 * 
 * @see http://web.engr.oregonstate.edu/cgi-bin/cgiwrap/dmcgrath/classes/11F/cs311/index.cgi?file=project2
 * 
 */
#define _BSD_SOURCE 1

#include <assert.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "myar.h"

/// No mode selected
#define MODE_NONE			0

/// Append all regular files in current working directory mode
#define MODE_APPEND_ALL 	1

/// Delete archive member mode
#define MODE_DELETE			2

/// Append files to archive mode
#define MODE_APPEND			3

/// Print concise archive contents table mode
#define MODE_CONCISE_TABLE	4

/// Print verbose archive contents table mode
#define MODE_VERBOSE_TABLE	5

/// Extract members from archive mode
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

/**
 * @brief Print usage message and exit.
 */
void usage(void);

/**
 * @brief Open or create an archive, append all files and close the archive.
 *
 * Preconditions: path is not NULL, file referred to by path is writable
 *
 * Postconditions: An archive located at path contains all regular files in the current working directory.
 *
 * @param path Path of the archive file to open or create
 */
void append_all(const char *path);

/**
 * @brief Open an archive and print a concise file table.
 *
 * Preconditions: path is not NULL, a valid archive exists at path
 *
 * Postconditions: 
 *
 * @param path Path of the archive file to open
 */
void concise_table(const char *path);

/**
 * @brief Open an archive and print a verbose file table.
 *
 * Preconditions: path is not NULL, a valid archive exists at path
 *
 * Postconditions: 
 *
 * @param path Path of the archive file to open
 */
void verbose_table(const char *path);

/**
 * @brief Remove all archive members defined in the list names from the archive referred to by path.
 *
 * Preconditions: path is not NULL, a valid archive exists at path, names is not NULL, names contains a list of members to be removed from the archive
 *
 * Postconditions: The members defined in the list do not exist in the archive
 *
 * @param path Path of the archive file to open
 * @param names List of members to remove from the archive
 */
void delete(const char *path, struct List *names);

/**
 * @brief Append all files defined in the list names to the archive referred to by path, creating the archvie if it does not exist.
 *
 * Preconditions: path is not NULL, names is not NULL, names contains a list of files to be appended to the archive
 *
 * Postconditions: The files defined in the list are in the archive
 *
 * @param path Path of the archive file to open
 * @param names List of files to add to the archive
 */
void append(const char *path, struct List *names);

/**
 * @brief Extract all members defined in the list names from the archive referred to by path.
 *
 * Preconditions: path is not NULL, names is not NULL, names contains a list of memebers to be extracted from the archive, a file may be written with the name of each member in the list of names
 *
 * Postconditions: The members defined in the list have been extracted
 *
 * @param path Path of the archive file to open
 * @param names List of members to extract from the archive
 */
void extract(const char *path, struct List *names);

/**
 * @brief Program entry point.
 *
 * Parses command line arguments and dispatches necessary information to the proper functions.
 *
 * @param argc Number of command line arguments
 * @param argv Array of strings containing command line arguments
 * @return Exit status
 */
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
	case MODE_APPEND_ALL:
		append_all(archive_path);
		break;
	case MODE_DELETE:
		delete(archive_path, &files);
		break;
	case MODE_APPEND:
		append(archive_path, &files);
		break;	
	case MODE_CONCISE_TABLE:
		concise_table(archive_path);
		break;
	case MODE_VERBOSE_TABLE:
		verbose_table(archive_path);
		break;
	case MODE_EXTRACT:
		extract(archive_path, &files);
		break;
	}
	
	list_free(&files);

	return 0;
}

void append_all(const char *path) {
	DIR *dp;
	struct dirent *de;
	struct ar a;

	dp = opendir("./");
	if (dp == NULL) {
		// Report error
		fprintf(stderr, "Could not open current directory\n");
		return;
	}

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		fprintf(stderr, "Failed to open archive (%s)\n", path);
	} else {
		while ((de = readdir(dp)) != NULL) {
			if ((de->d_type == DT_REG) && (strcmp(de->d_name, path) != 0)) {
				if (ar_add_file(&a, de->d_name) == false) {
					fprintf(stderr, "Failed to add %s to archive\n", de->d_name);
				}
			}
		}
	}

	closedir(dp);

	ar_free(&a);
}

void concise_table(const char *path) {
	struct ar a;

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		fprintf(stderr, "Failed to open archive (%s)\n", path);
	} else {
		ar_print_concise(&a);
	}

	ar_free(&a);
}

void verbose_table(const char *path) {
	struct ar a;

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		fprintf(stderr, "Failed to open archive (%s)\n", path);
	} else {
		ar_print_verbose(&a);
	}

	ar_free(&a);
}

void extract(const char *path, struct List *names) {
	struct ar a;

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		fprintf(stderr, "Failed to open archive (%s)\n", path);
	} else {
		for (int i = 0; i < list_size(names); i++) {	
			char *name = (char *)list_get(names, i);
			if (ar_extract_file(&a, name) == false) {
				fprintf(stderr, "Failed to extract %s from archive\n", name);
			}
		}
	}

	ar_free(&a);
}

void append(const char *path, struct List *names) {
	struct ar a;

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		fprintf(stderr, "Failed to open archive (%s)\n", path);
	} else {
		for (int i = 0; i < list_size(names); i++) {
			char *name = (char *)list_get(names, i);
			if (ar_add_file(&a, name) == false) {
				fprintf(stderr, "Failed to add %s to archive\n", name);
			}
		}
	}

	ar_free(&a);
}

void delete(const char *path, struct List *names) {
	struct ar a;

	ar_init(&a);
	if (ar_open(&a, path) == false) {
		fprintf(stderr, "Failed to open archive (%s)\n", path);
	} else {
		for (int i = 0; i < list_size(names); i++) {
			char *name = (char *)list_get(names, i);
			if (ar_remove_file(&a, name) == false) {
				fprintf(stderr, "Failed to remove %s from archive\n", name);
			}
		}
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

