/**
 * @file myar.c
 * @author Dan Albert
 * @date Created 10/26/2011
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
 * Implements an interface for UNIX archive file handling.
 * 
 */
#undef _BSD_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "myar.h"

/// Default file permissions for new archives
#define DEFAULT_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/// Bit mask to select only the file permissions from a file mode
#define PERM_MASK 0x01ff

/// Maximum number of bytes to write in a single write() call
#define BLOCK_SIZE 4096

/// Size of file time string for verbose output
#define SFTIME 18

/// Size of ar file header name string
#define SARFNAME 16

/// Size of ar file header date string
#define SARFDATE 12

/// Size of ar file header UID string
#define SARFUID 6

/// Size of ar file header GID string
#define SARFGID 6

/// Size of ar file header mode string
#define SARFMODE 8

/// Size of ar file header size string
#define SARFSIZE 10

/// Size of ar file header magic number
#define SARFMAG 2

/**
 * @brief Verifies presence and validity of ar file magic number.
 *
 * Preconditions: fd is an file descriptor for a valid archive
 *
 * Postconditions: File pointer has been returned to its original position
 *
 * @param fd File descriptor of an open archive
 * @return true if magic number is valid, false otherwise
 */
bool _ar_check_global_hdr(int fd);

/**
 * @brief Writes ar file magic number to the beginning of a file.
 *
 * Preconditions: fd is an file descriptor for a valid archive
 *
 * Postconditions: File pointer has been returned to its original position,
 * global header has been written
 *
 * @param fd File descriptor of an open archive
 * @return true on success, false otherwise
 */
bool _ar_write_global_hdr(int fd);

/**
 * @brief Loads file header from archive file
 * 
 * Preconditions: fd is an file descriptor for a valid archive, file pointer is
 * at the beginning of a file header, hdr is not NULL
 * 
 * Postconditions: File header has been loaded into hdr
 *
 * @param fd File descriptor of an open archive
 * @param hdr Pointer to ar_hdr to load data into
 * @return true on success, false otherwise
 */
bool _ar_load_hdr(int fd, struct ar_hdr *hdr);

bool _ar_seek(int fd, const char *name, struct ar_hdr *hdr);

void _ar_member_name(struct ar_hdr *hdr, char *name);
size_t _ar_member_size(struct ar_hdr *hdr);

int ar_open(const char *path) {
	struct stat st;
	bool create;
	int fd;

	assert(path);

	if (stat(path, &st) == 0) {
		create = false;
	} else {
		create = true;
	}
	
	fd = open(path, O_RDWR | O_CREAT, DEFAULT_PERMS);
	if (fd == -1) {
		// Report error
		fprintf(stderr, "File (%s) could not be opened\n", path);
		return -1;
	}

	lseek(fd, 0, SEEK_SET);

	if (create == false) {
		if (!_ar_check_global_hdr(fd)) {
			// Report error
			fprintf(stderr, "Bad global header\n");

			// Clean up
			ar_close(fd);

			return -1;
		}
	} else {
		if (_ar_write_global_hdr(fd) == false) {
			// Report error
			fprintf(stderr, "Unable to write global header\n");

			// Clean up
			ar_close(fd);

			return -1;
		}
	}

	return fd;
}

void ar_close(int fd) {
	assert(fd >= 0);

	if (close(fd) == -1) {
		// Report error
		fprintf(stderr, "File could not be closed\n");
	}
}

bool ar_append(int fd, const char *path) {
	struct ar_hdr hdr;
	struct stat st;
	char buf[BLOCK_SIZE];
	int append_fd;

	stat(path, &st);

	append_fd = open(path, O_RDONLY);

	if (append_fd < 0) {
		// Report error
		return false;
	}

	// TODO: Should strip any path preceding file name
	snprintf(hdr.ar_name, SARFNAME, "%-16.16s", path);
	snprintf(hdr.ar_date, SARFDATE, "%12.12u", st.st_mtime);
	snprintf(hdr.ar_uid, SARFUID, "%6.6u", st.st_uid);
	snprintf(hdr.ar_gid, SARFGID, "%6.6u", st.st_gid);
	snprintf(hdr.ar_mode, SARFMODE, "%8.8o", st.st_mode);
	snprintf(hdr.ar_size, SARFSIZE, "%10.10d", st.st_size);
	memcpy(hdr.ar_fmag, ARFMAG, SARFMAG);

	// If on an odd byte offset, write a newline
	if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
		if (write(fd, "\n", sizeof(char)) == -1) {
			// Report error
			fprintf(stderr, "Write error (line %d)\n", __LINE__);

			// Clean up
			close(append_fd);

			return false;
		}
	}

	if (write(fd, &hdr, sizeof(struct ar_hdr)) == -1) {
		// Report error
		fprintf(stderr, "Write error (line %d)\n", __LINE__);

		// Clean up
		close(append_fd);

		return false;
	}

	while (lseek(append_fd, 0, SEEK_CUR) < st.st_size) {
		off_t remaining = st.st_size - lseek(append_fd, 0, SEEK_CUR);
		size_t wr_size = (remaining < BLOCK_SIZE) ? remaining : BLOCK_SIZE;

		if (read(append_fd, buf, wr_size) == -1) {
			// Report error
			fprintf(stderr, "Read error (line %d)\n", __LINE__);

			// Clean up
			close(append_fd);

			return false;
		}

		if (write(fd, buf, wr_size) == -1) {
			// Report error
			fprintf(stderr, "Write error (line %d)\n", __LINE__);

			// Clean up
			close(append_fd);

			return false;
		}
	}

	return true;
}

bool ar_remove(int fd, const char *name) {
	struct ar_hdr hdr;
	size_t ar_size;
	size_t size;
	off_t member_end;
	off_t pos;

	assert(fd >= 0);
	assert(name != NULL);

	if (_ar_seek(fd, name, &hdr) == false) {
		return false;
	}

	size = _ar_member_size(&hdr);

	// Find beginning of file header
	pos = lseek(fd, -sizeof(struct ar_hdr), SEEK_CUR);

	// Get size of archive file
	ar_size = lseek(fd, 0, SEEK_END);

	// Return to beginning of file header
	lseek(fd, pos, SEEK_SET);

	member_end = pos + sizeof(struct ar_hdr) + size;

	// If there is more data following this member
	if (member_end < ar_size) {
		// TODO: Shift data down

		// TODO: Truncate extra newline if necessary
		ftruncate(fd, ar_size - sizeof(struct ar_hdr) - size);
	} else {
		// Truncate last member
		ftruncate(fd, pos);
	}
	
	return true;
}

bool ar_extract(int fd, const char *name) {
	struct ar_hdr hdr;
	size_t size;
	size_t written;
	int extract_fd;

	assert(fd >= 0);
	assert(name != NULL);

	if (_ar_seek(fd, name, &hdr) == false) {
		return false;
	}

	extract_fd = creat(name, DEFAULT_PERMS);
	if (extract_fd == -1) {
		// Report error
	}

	size = _ar_member_size(&hdr);
	written = 0;

	while (written < size) {
		char buf[BLOCK_SIZE];
		size_t wr_size;

		wr_size = (size - written < BLOCK_SIZE) ? size - written : BLOCK_SIZE;

		if (read(fd, buf, wr_size) == -1) {
			// Report error
			fprintf(stderr, "Read error (line %d)\n", __LINE__);

			// Clean up
			close(extract_fd);

			return false;
		}

		if (write(extract_fd, buf, wr_size) == -1) {
			// Report error
			fprintf(stderr, "Write error (line %d)\n", __LINE__);

			// Clean up
			close(extract_fd);

			return false;
		}

		written += wr_size;
	}

	if (close(extract_fd) == -1) {
		// Report error
		fprintf(stderr, "Could not close file (line %d)\n", __LINE__);

		return false;
	}

	return true;
}

void ar_print_concise(int fd) {
	off_t ar_size;

	assert(fd >= 0);

	ar_size = lseek(fd, 0, SEEK_END);
	lseek(fd, SARMAG, SEEK_SET);

	while (lseek(fd, 0, SEEK_CUR) < ar_size) {
		struct ar_hdr hdr;
		char name[SARFNAME + 1];

		if (_ar_load_hdr(fd, &hdr) == false) {
			// Report error
			fprintf(stderr, "Could not load ar_hdr (line %d)\n", __LINE__);
			return;
		}

		_ar_member_name(&hdr, name);
		printf("%s\n", name);

		// Skip past data
		lseek(fd, _ar_member_size(&hdr), SEEK_CUR);

		// If on an odd byte offset, seek ahead one byte
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}
}

/*void ar_print_verbose(struct ar *a) {
	for (int i = 0; i < ar_nfiles(a); i++) {
		struct ar_hdr *hdr;
		struct tm *time;
		char *slash_location;
		char ftime[SFTIME];
		char name[17];
		char date_str[13];
		char uid_str[7];
		char gid_str[7];
		char mode_str[9];
		char perm_str[10];
		char size_str[11];
		time_t date;
		int uid;
		int gid;
		int mode;
		int size;
		
		hdr = &ar_get_file(a, i)->hdr;

		memset(name, '\0', sizeof(name));
		memset(date_str, '\0', sizeof(date_str));
		memset(uid_str, '\0', sizeof(uid_str));
		memset(gid_str, '\0', sizeof(gid_str));
		memset(mode_str, '\0', sizeof(mode_str));
		memset(perm_str, '\0', sizeof(perm_str));
		memset(size_str, '\0', sizeof(size_str));

		strncpy(name, hdr->ar_name, sizeof(name));
		strncpy(date_str, hdr->ar_date, sizeof(date_str));
		strncpy(uid_str, hdr->ar_uid, sizeof(uid_str));
		strncpy(gid_str, hdr->ar_gid, sizeof(gid_str));
		strncpy(mode_str, hdr->ar_mode, sizeof(mode_str));
		strncpy(size_str, hdr->ar_size, sizeof(size_str));

		slash_location = rindex(name, '/');
		if (slash_location != NULL) {
			*slash_location = '\0'; // Replace the terminating / with a null
		}

		date = strtol(date_str, NULL, 10);
		uid = strtol(uid_str, NULL, 10);
		gid = strtol(gid_str, NULL, 10);
		mode = strtol(mode_str, NULL, 8);
		size = strtol(size_str, NULL, 10);

		perm_str[0] = (mode & S_IRUSR) ? 'r' : '-';
		perm_str[1] = (mode & S_IWUSR) ? 'w' : '-';
		perm_str[2] = (mode & S_IXUSR) ? 'x' : '-';
		perm_str[3] = (mode & S_IRGRP) ? 'r' : '-';
		perm_str[4] = (mode & S_IWGRP) ? 'w' : '-';
		perm_str[5] = (mode & S_IXGRP) ? 'x' : '-';
		perm_str[6] = (mode & S_IROTH) ? 'r' : '-';
		perm_str[7] = (mode & S_IWOTH) ? 'w' : '-';
		perm_str[8] = (mode & S_IXOTH) ? 'x' : '-';
		perm_str[9] = '\0';

		time = localtime(&date);
		strftime(ftime, SFTIME, "%b %d %H:%M %Y", time);
		printf("%s %6d/%-6d %10d %s %s\n", perm_str, uid, gid, size, ftime, name);
	}
}*/

bool _ar_check_global_hdr(int fd) {
	char hdr[SARMAG];
	int init_pos;
	bool hdr_good;

	assert(fd >= 0);

	// Remember initial position
   	init_pos = lseek(fd, 0, SEEK_CUR);

	lseek(fd, 0, SEEK_SET);

	if (read(fd, hdr, SARMAG) == -1) {
		// Report error
		fprintf(stderr, "Read error (line %d)\n", __LINE__);

		return false;
	}

	hdr_good = (memcmp(hdr, ARMAG, SARMAG) == 0);

	// Reset file position
	lseek(fd, init_pos, SEEK_SET);

	return hdr_good;
}

bool _ar_write_global_hdr(int fd) {
	assert(fd >= 0);

	lseek(fd, 0, SEEK_SET);

	if (write(fd, ARMAG, SARMAG) == -1) {
		fprintf(stderr, "Write error (line %d)\n", __LINE__);
		return false;
	}

	return true;
}

bool _ar_load_hdr(int fd, struct ar_hdr *hdr) {
	assert(fd >= 0);
	assert(hdr);

	if (read(fd, hdr, sizeof(struct ar_hdr)) == -1) {
		fprintf(stderr, "Error reading header data\n");
		return false;
	}

	if (memcmp(hdr->ar_fmag, ARFMAG, SARFMAG) != 0) {
		// Magic number incorrect
		fprintf(stderr, "Magic number mismatch\n");
		return false;
	}

	return true;
}

bool _ar_seek(int fd, const char *name, struct ar_hdr *hdr) {
	off_t size;

	assert(fd >= 0);
	assert(name != NULL);

	// Get file size
	size = lseek(fd, 0, SEEK_END);

	// Seek to end of global header
	lseek(fd, SARMAG, SEEK_SET);

	while (lseek(fd, 0, SEEK_CUR) < size) {
		if (_ar_load_hdr(fd, hdr) == false) {
			return false;
		}

		if (memcmp(name, hdr->ar_name, strlen(name)) == 0) {
			return true;
		}

		// Seek to the next header
		lseek(fd, _ar_member_size(hdr), SEEK_CUR);

		// If on an odd byte offset, seek ahead one byte
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}

	return false;
}

void _ar_member_name(struct ar_hdr *hdr, char *name) {
	assert(hdr != NULL);

	memcpy(name, hdr->ar_name, SARFNAME);
	name[SARFNAME + 1] = '\0';

	while ((name[strlen(name)] == ' ') || (name[strlen(name)] == '/')) {
		name[strlen(name)] = '\0';
	}
}

size_t _ar_member_size(struct ar_hdr *hdr) {
	char size_str[SARFSIZE + 1];

	assert(hdr != NULL);

	memcpy(size_str, hdr->ar_size, SARFSIZE);
	size_str[SARFSIZE] = '\0';

	return strtol(size_str, NULL, 10);
}
