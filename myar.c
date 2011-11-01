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
 * Implements an interface for ar file handling.
 * 
 */
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
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

/**
 * @brief Verifies presence and validity of ar file magic number
 *
 * Preconditions: a is not NULL, a has been opened
 *
 * Postconditions: File pointer has been returned to its original position
 *
 * @param Pointer to ar structure
 * @return true if magic number is valid, false otherwise
 */
bool _ar_check_global_hdr(struct ar *a);

/**
 * @brief Loads each file found in the archive
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive
 * 
 * Postconditions: All files contained in the archive have been loaded
 *
 * @param a Pointer to ar structure
 * @return true on success, false otherwise
 */
bool _ar_scan(struct ar *a);

/**
 * @brief Loads file header and data from archive file
 * 
 * Preconditions: a is not NULL, a is initialized, file pointer is at the beginning of a file header
 * 
 * Postconditions: File header and data have been loaded
 *
 * @param a Pointer to ar structure
 * @return true on success, false otherwise
 */
bool _ar_load_file(struct ar *a);

/**
 * @brief Loads file header from archive file
 * 
 * Preconditions: a is not NULL, a is initialized, file pointer is at the beginning of a file header, hdr is not NULL
 * 
 * Postconditions: File header has been loaded into hdr
 *
 * @param a Pointer to ar structure
 * @param hdr Pointer to ar_hdr to load data into
 * @return true on success, false otherwise
 */
bool _ar_load_hdr(struct ar *a, struct ar_hdr *hdr);

/**
 * @brief Loads file data from archive file
 * 
 * Preconditions: a is not NULL, a is initialized, file pointer is at the beginning of a file header, file is not NULL
 * 
 * Postconditions: File data has been loaded into file.data
 *
 * @param a Pointer to ar structure
 * @param file Pointer to ar_file to load data into
 * @return true on success, false otherwise
 */
bool _ar_load_data(struct ar *a, struct ar_file *file);

/**
 * @brief Copies data from one ar_file to another
 * 
 * Preconditions: dst is not NULL, src is not NULL
 * 
 * Postconditions: Memory has been allocated for *dst, src has been copied to *dst
 *
 * @param dst Pointer to pointer to destination ar_file structure
 * @param src Pointer to source ar_file structure
 *
 * @note dst is a pointer to a pointer because this function allocates memory for dst, and C does not allow variables to be passed by reference. It's ugly, I'm sorry. I didn't know we could use C++ data structures until I had already written this god awful data structure.
 */
void _ar_file_copy(void **dst, void *src);

/**
 * @brief Frees all memory used by an ar_file structure
 * 
 * Preconditions: res is not NULL
 * 
 * Postconditions: Memory allocated for file data has been released, memory allocated for res has been released
 *
 * @param res Pointer to ar_file structure
 */
void _ar_file_release(void *res);

void ar_init(struct ar *a) {
	list_init(&a->files, _ar_file_copy, _ar_file_release);
}

void ar_free(struct ar *a) {
	if (a->fd >= 0) {
		ar_close(a);
	}

	list_free(&a->files);
}

bool ar_open(struct ar *a, const char *path) {
	struct stat st;
	bool create;

	assert(a);
	assert(path);

	if (stat(path, &st) == 0) {
		create = false;
	} else {
		create = true;
	}
	
	a->fd = open(path, O_RDWR | O_CREAT, DEFAULT_PERMS);
	if (a->fd == -1) {
		// Report error
		fprintf(stderr, "File (%s) could not be opened\n", path);
		return false;
	}

	lseek(a->fd, 0, SEEK_SET);

	if (create == false) {
		if (!_ar_check_global_hdr(a)) {
			// Report error
			fprintf(stderr, "Bad global header\n");

			// Clean up
			ar_close(a);

			return false;
		}

		_ar_scan(a);
	}

	return true;
}

bool ar_close(struct ar *a) {
	off_t total_sz;

	assert(a);
	
	if (a->fd < 0) {
		return true;
	}

	lseek(a->fd, 0, SEEK_SET);
 
	total_sz = 0;

	write(a->fd, ARMAG, SARMAG);
	total_sz += SARMAG;
	
	for (int i = 0; i < ar_nfiles(a); i++) {
		struct ar_file *file = ar_get_file(a, i);
		char size_str[11];
		off_t size;
		off_t written = 0;

		memset(size_str, '\0', sizeof(size_str));
		strncpy(size_str, file->hdr.ar_size, sizeof(size_str));
		size = strtol(size_str, NULL, 10);

		// Write a single new line if we are not on an even byte offset
		if ((lseek(a->fd, 0, SEEK_CUR) % 2) != 0) {
			write(a->fd, "\n", sizeof(uint8_t));
			total_sz += sizeof(uint8_t);
		}

		write(a->fd, &file->hdr, sizeof(struct ar_hdr));
		total_sz += sizeof(struct ar_hdr);

		while (written < size) {
			int wr_size;

			if ((size - written) < BLOCK_SIZE) {
				wr_size = size - written;
			} else {
				wr_size = BLOCK_SIZE;
			}

			write(a->fd, file->data, wr_size); 
			written += wr_size;
		}

		total_sz += size;
	}

	ftruncate(a->fd, total_sz);

	if (close(a->fd) == -1) {
		// Report error
		fprintf(stderr, "File could not be closed\n");
		return false;
	}

	a->fd = -1;
	return true;
}

size_t ar_nfiles(struct ar *a) {
	return list_size(&a->files);
}

struct ar_file *ar_get_file(struct ar *a, size_t i) {
	return (struct ar_file *)list_get(&a->files, i);
}

bool ar_add_file(struct ar *a, const char *path) {
	struct ar_file file;
	struct stat st;
	char *slash_location;
	char name[17];
	char date[13];
	char uid[7];
	char gid[7];
	char mode[9];
	char size[11];
	int fd;

	stat(path, &st);

	fd = open(path, O_RDONLY);

	if (fd < 0) {
		// Report error
		return false;
	}

	// Should strip any path preceding file name
	snprintf(name, sizeof(name), "%-.16s", path);
	slash_location = rindex(name, '/');

	if (slash_location != NULL) {
		memset(slash_location + 1, ' ', sizeof(name) - (slash_location - name));
	}

	snprintf(date, sizeof(date), "%12u", st.st_mtime);
	snprintf(uid, sizeof(uid), "%6u", st.st_uid);
	snprintf(gid, sizeof(gid), "%6u", st.st_gid);
	snprintf(mode, sizeof(mode), "%8o", st.st_mode);
	snprintf(size, sizeof(size), "%10u", st.st_size);

	memcpy(file.hdr.ar_name, name, sizeof(file.hdr.ar_name));
	memcpy(file.hdr.ar_date, date, sizeof(file.hdr.ar_date));
	memcpy(file.hdr.ar_uid, uid, sizeof(file.hdr.ar_uid));
	memcpy(file.hdr.ar_gid, gid, sizeof(file.hdr.ar_gid));
	memcpy(file.hdr.ar_mode, mode, sizeof(file.hdr.ar_mode));
	memcpy(file.hdr.ar_size, size, sizeof(file.hdr.ar_size));
	memcpy(file.hdr.ar_fmag, ARFMAG, SARFMAG);

	file.data = (uint8_t *)malloc(st.st_size);
	read(fd, file.data, st.st_size);

	list_add_back(&a->files, (void *)&file);

	return true;
}

bool ar_remove_file(struct ar *a, const char *name) {
	for (int i = 0; i < ar_nfiles(a); i++) {
		struct ar_file *file = ar_get_file(a, i);
		char *slash_location;
		char fname[17];

		memset(fname, '\0', sizeof(fname));
		memcpy(fname, file->hdr.ar_name, sizeof(file->hdr.ar_name));
		slash_location = rindex(fname, '/');
		if (slash_location != NULL) {
			*slash_location = '\0';
		}
		
		if (strcmp(fname, name) == 0) {
			list_remove(&a->files, i);
			return true;
		}
	}
	
	return false;
}

bool ar_extract_file(struct ar *a, const char *name) {
	for (int i = 0; i < ar_nfiles(a); i++) {
		struct ar_file * file = ar_get_file(a, i);
		if (memcmp(name, file->hdr.ar_name, strlen(name)) == 0) {
			struct utimbuf tbuf;
			char size_str[11];
			char time_str[13];
			char uid_str[7];
			char gid_str[7];
			char mode_str[9];
			time_t modify;
			int fd;
			int size;
			int mode;

			memset(size_str, '\0', sizeof(size_str));
			memset(time_str, '\0', sizeof(time_str));
			memset(mode_str, '\0', sizeof(mode_str));
			strncpy(size_str, file->hdr.ar_size, sizeof(size_str));
			strncpy(time_str, file->hdr.ar_date, sizeof(time_str));
			strncpy(mode_str, file->hdr.ar_mode, sizeof(mode_str));
			size = strtol(size_str, NULL, 10);
			modify = strtol(time_str, NULL, 10);
			mode = strtol(mode_str, NULL, 8);
			
			fd = creat(name, mode & PERM_MASK);

			if (fd < 0) {
				// Report error
				return false;
			}

			write(fd, file->data, size);

			close(fd);

			chmod(name, mode & PERM_MASK);

			tbuf.actime = modify;
			tbuf.modtime = modify;
			
			if (utime(name, &tbuf) == -1) {
				// Report error
				return false;
			}

			return true;
		}
	}
}

void ar_print_concise(struct ar *a) {
	for (int i = 0; i < ar_nfiles(a); i++) {
		struct ar_hdr *hdr;

		char *slash_location;
		char name[17];
		
		hdr = &ar_get_file(a, i)->hdr;

		memset(name, '\0', sizeof(name));
		strncpy(name, hdr->ar_name, sizeof(name));

		slash_location = rindex(name, '/');
		if (slash_location != NULL) {
			*slash_location = '\0'; // Replace the terminating / with a null
		}

		printf("%s\n", name);
	}
}

void ar_print_verbose(struct ar *a) {
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
}

bool _ar_check_global_hdr(struct ar *a) {
	char hdr[SARMAG];
	int init_pos;
	bool hdr_good;

	assert(a);
	assert(a->fd >= 0);

	// Remember initial position
   	init_pos = lseek(a->fd, 0, SEEK_CUR);

	lseek(a->fd, 0, SEEK_SET);
	read(a->fd, hdr, SARMAG);

	hdr_good = !memcmp(hdr, ARMAG, SARMAG);

	// Reset file position
	lseek(a->fd, init_pos, SEEK_SET);

	return hdr_good;
}

bool _ar_scan(struct ar *a) {
	off_t file_size;

	assert(a);
	assert(a->fd >= 0);

	file_size = lseek(a->fd, 0, SEEK_END);

	// Set file position to the first file hdr
	lseek(a->fd, SARMAG, SEEK_SET);

	while (lseek(a->fd, 0, SEEK_CUR) < file_size - 1) {
		if (_ar_load_file(a) == false) {
			fprintf(stderr, "Failed to scan\n");
			return false;
		}

		if ((lseek(a->fd, 0, SEEK_CUR) % 2) == 1) {
			// All file headers begin on an even byte boundary
			// Advance to even byte offset if on an odd
			lseek(a->fd, 1, SEEK_CUR);
		}
	}

	return true;
}

bool _ar_load_file(struct ar *a) {
	struct ar_file *file;
	assert(a);
	assert(a->fd >= 0);

	file = (struct ar_file *)malloc(sizeof(struct ar_file));
	if (file == NULL) {
		fprintf(stderr, "Could not allocate memory for ar_file\n");
		return false;
	}

	if (!_ar_load_hdr(a, &file->hdr)) {
		fprintf(stderr, "Error loading file header\n");
		free(file);
		return false;
	}

	if (!_ar_load_data(a, file)) {
		fprintf(stderr, "Error loading file data\n");
		free(file);
		return false;
	}

	list_add_back(&a->files, (void *)file);

	return true;
}

bool _ar_load_hdr(struct ar *a, struct ar_hdr *hdr) {
	assert(a);
	assert(a->fd >= 0);
	assert(hdr);

	if (read(a->fd, hdr, sizeof(struct ar_hdr)) == -1) {
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

bool _ar_load_data(struct ar *a, struct ar_file *file) {
	int data_size;

	assert(a);
	assert(a->fd >= 0);
	assert(file);

	if (file->data != NULL) {
		free(file->data);
	}

	data_size = atoi(file->hdr.ar_size);
	file->data = (uint8_t *)malloc(data_size * sizeof(uint8_t));
	if (file->data == NULL) {
		// Report error
		return false;
	}

	if (read(a->fd, file->data, data_size) == -1) {
		// Report error
		free(file->data);
		return false;
	}

	return true;
}

void _ar_file_copy(void **dst, void *src) {
	assert(dst);
	assert(src);

	*dst = (struct ar_file *)malloc(sizeof(struct ar_file));
	assert(*dst);

	memcpy(*dst, src, sizeof(struct ar_file));
}

void _ar_file_release(void *res) {
	if (res != NULL) {
		if (((struct ar_file *)res)->data != NULL) {
			free(((struct ar_file *)res)->data);
		}

		free(res);
	}
}

