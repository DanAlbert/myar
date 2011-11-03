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
#define _BSD_SOURCE 1

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

/// Name of temporary archive for remove
#define TEMP_AR_NAME ".temp.a"

/// Size of file time string for verbose output
#define SFTIME 18

/// Size of file mode string for verbose output
#define SFMODE 10

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

void _ar_mode_str(mode_t mode, char *str);

void _ar_member_name(struct ar_hdr *hdr, char *name);
time_t _ar_member_date(struct ar_hdr *hdr);
uid_t _ar_member_uid(struct ar_hdr *hdr);
gid_t _ar_member_gid(struct ar_hdr *hdr);
mode_t _ar_member_mode(struct ar_hdr *hdr);
off_t _ar_member_size(struct ar_hdr *hdr);

bool block_read(int fd, uint8_t *buf, off_t from, size_t size);
bool block_write(int fd, uint8_t *buf, off_t to, size_t size);
bool block_copy(int fd, off_t from, off_t to, size_t size);

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
	char name[SARFNAME + 1];
	char date[SARFDATE + 1];
	char uid[SARFUID + 1];
	char gid[SARFGID + 1];
	char mode[SARFMODE + 1];
	char size[SARFSIZE + 1];
	int append_fd;

	stat(path, &st);

	append_fd = open(path, O_RDONLY);

	if (append_fd < 0) {
		// Report error
		return false;
	}

	// TODO: Should strip any path preceding file name
	snprintf(name, SARFNAME + 1, "%-16s", path);
	snprintf(date, SARFDATE + 1, "%12d", st.st_mtim.tv_sec);
	snprintf(uid, SARFUID + 1, "%6u", st.st_uid);
	snprintf(gid, SARFGID + 1, "%6u", st.st_gid);
	snprintf(mode, SARFMODE + 1, "%8o", st.st_mode);
	snprintf(size, SARFSIZE + 1, "%10d", st.st_size);

	memcpy(hdr.ar_name, name, SARFNAME);
	memcpy(hdr.ar_date, date, SARFDATE);
	memcpy(hdr.ar_uid, uid, SARFUID);
	memcpy(hdr.ar_gid, gid, SARFGID);
	memcpy(hdr.ar_mode, mode, SARFMODE);
	memcpy(hdr.ar_size, size, SARFSIZE);
	memcpy(hdr.ar_fmag, ARFMAG, SARFMAG);

	// Seek to end of file
	lseek(fd, 0, SEEK_END);

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
	struct stat st;
	uint8_t *buf;
	int temp_fd;

	assert(fd >= 0);
	assert(name != NULL);

	fstat(fd, &st);

	temp_fd = open(TEMP_AR_NAME, O_CREAT | O_TRUNC | O_RDWR, DEFAULT_PERMS);
	write(temp_fd, ARMAG, SARMAG);
	lseek(fd, SARMAG, SEEK_SET);

	while (lseek(fd, 0, SEEK_CUR) < st.st_size) {
		char member_name[SARFNAME + 1];
		size_t size;
		off_t next_member;
		off_t pos;

		_ar_load_hdr(fd, &hdr);
		size = _ar_member_size(&hdr);
		_ar_member_name(&hdr, member_name);

		if (strcmp(member_name, name) == 0) {
			// Skip the file
			lseek(fd, size, SEEK_CUR);
		} else {
			if ((lseek(temp_fd, 0, SEEK_CUR) % 2) == 1) {
				write(temp_fd, "\n", sizeof(char));
			}

			write(temp_fd, &hdr, sizeof(struct ar_hdr));

			if (size > 0) {
				buf = (uint8_t *)malloc(size * sizeof(uint8_t));
				block_read(fd, buf, lseek(fd, 0, SEEK_CUR), size);
				block_write(temp_fd, buf, lseek(temp_fd, 0, SEEK_CUR), size);
				free(buf);
			}

			if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
				lseek(fd, 1, SEEK_CUR);
			}
		}
	}

	if (ftruncate(fd, 0) == -1) {
		perror("Could not truncate archive");
	}

	if (fstat(temp_fd, &st) == -1) {
		perror("Could not stat temp file");
	}

	buf = (uint8_t *)malloc(st.st_size * sizeof(uint8_t));
	block_read(temp_fd, buf, lseek(temp_fd, 0, SEEK_SET), st.st_size);
	block_write(fd, buf, lseek(fd, 0, SEEK_SET), st.st_size);

	close(temp_fd);
	unlink(TEMP_AR_NAME);

	return true;
}

bool ar_extract(int fd, const char *name) {
	struct ar_hdr hdr;
	struct utimbuf tbuf;
	size_t size;
	size_t written;
	int extract_fd;

	assert(fd >= 0);
	assert(name != NULL);

	if (_ar_seek(fd, name, &hdr) == false) {
		printf("File %s not found in archive\n", name);
		return false;
	}

	extract_fd = creat(name, DEFAULT_PERMS);
	if (extract_fd == -1) {
		perror("Could not open file for extraction");
		return false;
	}

	size = _ar_member_size(&hdr);
	written = 0;

	while (written < size) {
		char buf[BLOCK_SIZE];
		size_t wr_size;

		wr_size = (size - written < BLOCK_SIZE) ? size - written : BLOCK_SIZE;

		if (read(fd, buf, wr_size) == -1) {
			perror("Read error");
			close(extract_fd);
			return false;
		}

		if (write(extract_fd, buf, wr_size) == -1) {
			perror("Write error");
			close(extract_fd);
			return false;
		}

		written += wr_size;
	}

	if (fchmod(extract_fd, _ar_member_mode(&hdr) & PERM_MASK) == -1) {
		perror("Unable to chmod() extracted file");
		close(extract_fd);
		return false;
	}
	
	if (close(extract_fd) == -1) {
		perror("Could not close file");
		close(extract_fd);
		return false;
	}
	
	tbuf.actime = _ar_member_date(&hdr);
	tbuf.modtime = _ar_member_date(&hdr);

	if (utime(name, &tbuf) == -1) {
		perror("Unable set modification time");
		close(extract_fd);
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

void ar_print_verbose(int fd) {
	off_t ar_size;

	assert(fd >= 0);

	ar_size = lseek(fd, 0, SEEK_END);
	lseek(fd, SARMAG, SEEK_SET);

	while (lseek(fd, 0, SEEK_CUR) < ar_size) {
		struct ar_hdr hdr;
		struct tm *time;
		char name[SARFNAME + 1];
		char ftime[SFTIME];
		char mode[SFMODE];
		time_t mtime;

		if (_ar_load_hdr(fd, &hdr) == false) {
			// Report error
			fprintf(stderr, "Could not load ar_hdr (line %d)\n", __LINE__);
			return;
		}

		_ar_member_name(&hdr, name);
		_ar_mode_str(_ar_member_mode(&hdr), mode);
		
		mtime = _ar_member_date(&hdr);
		time = localtime(&mtime);
		strftime(ftime, SFTIME, "%b %d %H:%M %Y", time);
		
		printf("%s %6d/%-6d %10d %s %s\n",
			mode,
			_ar_member_uid(&hdr),
			_ar_member_gid(&hdr),
			_ar_member_size(&hdr),
			ftime,
			name);

		// Skip past data
		lseek(fd, _ar_member_size(&hdr), SEEK_CUR);

		// If on an odd byte offset, seek ahead one byte
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}
}

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
		char member_name[SARFNAME + 1];
		if (_ar_load_hdr(fd, hdr) == false) {
			return false;
		}

		_ar_member_name(hdr, member_name);
		if (strcmp(name, member_name) == 0) {
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

void _ar_mode_str(mode_t mode, char *str) {
	assert(str != NULL);
	
	str[0] = (mode & S_IRUSR) ? 'r' : '-';
	str[1] = (mode & S_IWUSR) ? 'w' : '-';
	str[2] = (mode & S_IXUSR) ? 'x' : '-';
	str[3] = (mode & S_IRGRP) ? 'r' : '-';
	str[4] = (mode & S_IWGRP) ? 'w' : '-';
	str[5] = (mode & S_IXGRP) ? 'x' : '-';
	str[6] = (mode & S_IROTH) ? 'r' : '-';
	str[7] = (mode & S_IWOTH) ? 'w' : '-';
	str[8] = (mode & S_IXOTH) ? 'x' : '-';
	str[9] = '\0';
}

void _ar_member_name(struct ar_hdr *hdr, char *name) {
	assert(hdr != NULL);

	memset(name, '\0', SARFNAME + 1);
	memcpy(name, hdr->ar_name, SARFNAME);

	while ((name[strlen(name) - 1] == ' ') || (name[strlen(name) - 1] == '/')) {
		name[strlen(name) - 1] = '\0';
	}
}

time_t _ar_member_date(struct ar_hdr *hdr) {
	char str[SARFDATE + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_date, SARFDATE);
	str[SARFDATE] = '\0';

	return strtol(str, NULL, 10);
}

uid_t _ar_member_uid(struct ar_hdr *hdr) {
	char str[SARFUID + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_uid, SARFUID);
	str[SARFUID] = '\0';

	return strtol(str, NULL, 10);
}

gid_t _ar_member_gid(struct ar_hdr *hdr) {
	char str[SARFGID + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_gid, SARFGID);
	str[SARFGID] = '\0';

	return strtol(str, NULL, 10);
}

mode_t _ar_member_mode(struct ar_hdr *hdr) {
	char str[SARFMODE + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_mode, SARFMODE);
	str[SARFMODE] = '\0';

	return strtol(str, NULL, 8);
}

off_t _ar_member_size(struct ar_hdr *hdr) {
	char str[SARFSIZE + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_size, SARFSIZE);
	str[SARFSIZE] = '\0';

	return strtol(str, NULL, 10);
}

bool block_read(int fd, uint8_t *buf, off_t from, size_t size) {
	size_t done;

	assert(fd >= 0);
	assert(buf != NULL);
	assert(from >= 0);
	assert(size > 0);
	assert(from + size <= lseek(fd, 0, SEEK_END));

	done = 0;
	lseek(fd, from, SEEK_SET);
	while (done < size) {
		size_t count = ((size - done) < BLOCK_SIZE) ? (size - done) : BLOCK_SIZE;
		if (read(fd, buf + done, count) == -1) {
			perror("Read error");
			return false;
		}

		done += count;
	}

	return true;
}

bool block_write(int fd, uint8_t *buf, off_t to, size_t size) {
	size_t done;

	assert(fd >= 0);
	assert(buf != NULL);
	assert(to >= 0);
	assert(size > 0);

	done = 0;
	lseek(fd, to, SEEK_SET);
	while (done < size) {
		size_t count = ((size - done) < BLOCK_SIZE) ? (size - done) : BLOCK_SIZE;
		if (write(fd, buf + done, count) == -1) {
			perror("Write error");
			return false;
		}

		done += count;
	}

	return true;
}

bool block_copy(int fd, off_t from, off_t to, size_t size) {
	uint8_t *buf;
	size_t done;

	assert(fd >= 0);
	assert(from >= 0);
	assert(to >= 0);
	assert(size > 0);
	assert(from + size <= lseek(fd, 0, SEEK_END));
	assert(to + size <= lseek(fd, 0, SEEK_END));

	buf = (uint8_t *)malloc(size);
	if (buf == NULL) {
		perror("Error allocating memory");
		return false;
	}

	if (block_read(fd, buf, from, size) == false) {
		free(buf);
		return false;
	}

	if (block_write(fd, buf, to, size) == false) {
		free(buf);
		return false;
	}

	free(buf);
	return true;
}
