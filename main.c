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

/**
 * @brief Print usage message and exit.
 */
void usage(void);

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
	char *archive_path = NULL;
	int mode = MODE_NONE;
	int c;
	int fd;

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

	if (optind < argc) {
		archive_path = (char *)malloc((strlen(argv[optind]) + 1) * sizeof(char));
		if (archive_path == NULL) {
			perror(NULL);
			exit(0);
		}
			
		strcpy(archive_path, argv[optind++]);
	} else {
		usage();
	}

	fd = ar_open(archive_path);
	if (fd == -1) {
		fprintf(stderr, "Could not open archive file\n");
		return -1;
	}

	do {
		switch (mode) {
		case MODE_APPEND_ALL:
			append_all(fd, archive_path);
			break;
		case MODE_DELETE:
			//ar_remove(fd, argv[optind++]);
			printf("Not yet implemented\n");
			break;
		case MODE_APPEND:
			ar_append(fd, argv[optind++]);
			break;
		case MODE_CONCISE_TABLE:
			ar_print_concise(fd);
			break;
		case MODE_VERBOSE_TABLE:
			ar_print_verbose(fd);
			break;
		case MODE_EXTRACT:
			ar_extract(fd, argv[optind++]);
			break;
		}
	} while (optind < argc);
	
	ar_close(fd);

	return 0;
}

void append_all(int fd, const char *exclude) {
	DIR *dp;
	struct dirent *de;
	
	assert(fd >= 0);
	assert(exclude != NULL);

	dp = opendir("./");
	if (dp == NULL) {
		// Report error
		fprintf(stderr, "Could not open current directory\n");
		return;
	}

	while ((de = readdir(dp)) != NULL) {
		if ((de->d_type == DT_REG) && (strcmp(de->d_name, exclude) != 0)) {
			if (ar_append(fd, de->d_name) == false) {
				fprintf(stderr, "Failed to add %s to archive\n", de->d_name);
			}
		}
	}

	closedir(dp);
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
