/**
 * @file myar.c
 * @author Dan Albert
 * @date Created 10/26/2011
 * @date Last updated 11/21/2011
 * @version 1.0
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
bool ar_check_global_hdr(int fd);

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
bool ar_write_global_hdr(int fd);

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
bool ar_load_hdr(int fd, struct ar_hdr *hdr);

/**
 * @brief Searches through the archive for specific member and loads header data.
 *
 * Preconditions: fd is an file descriptor for a valid archive, name is not NULL,
 * name refers to a member of the archive, hdr is not NULL
 *
 * Postconditions:
 *
 * @param fd File descriptor of an open archive
 * @param name Name of the member to seek to
 * @param hdr Pointer to ar_hdr to load data into
 */
bool ar_seek(int fd, const char *name, struct ar_hdr *hdr);

/**
 * @brief Converts a mode integer to an ASCII permissions string.
 *
 * Preconditions: str is not NULL, str is an allocated buffer of size SFMODE
 *
 * Postconditions: Permissions string has been loaded into str
 *
 * @param mode File mode integer
 * @param str Character array of size SFMODE to load permissions string into
 */
void ar_mode_str(mode_t mode, char *str);

/**
 * @brief Retrieves a null terminated member name from an ar_hdr structure.
 *
 * Preconditions: hdr is not NULL, hdr is a valid ar header, name is not NULL,
 * name is an allocated buffer of size SARFNAME + 1
 *
 * Postconditions: A null terminated member name has been loaded into name
 *
 * @param hdr Pointer to ar_hdr to retrieve member name from
 * @param name Character array of size SARFNAME + 1 to load the member name into
 */
void ar_member_name(struct ar_hdr *hdr, char *name);

/**
 * @brief Returns a UNIX timestamp from an ar_hdr structure.
 *
 * Preconditions: hdr is not NULL, hdr is a valid ar header
 *
 * Postconditions:
 *
 * @param hdr Pointer to ar_hdr to retrieve member name from
 * @return Integer representation of the UNIX timestamp
 */
time_t ar_member_date(struct ar_hdr *hdr);

/**
 * @brief Returns the owner's user ID from an ar_hdr structure.
 *
 * Preconditions: hdr is not NULL, hdr is a valid ar header
 *
 * Postconditions:
 *
 * @param hdr Pointer to ar_hdr to retrieve owner's user ID from
 * @return File owner's user ID
 */
uid_t ar_member_uid(struct ar_hdr *hdr);

/**
 * @brief Returns the owning group's user ID from an ar_hdr structure.
 *
 * Preconditions: hdr is not NULL, hdr is a valid ar header
 *
 * Postconditions:
 *
 * @param hdr Pointer to ar_hdr to retrieve the owning group's ID from
 * @return File owning group's group ID
 */
gid_t ar_member_gid(struct ar_hdr *hdr);

/**
 * @brief Returns a file's mode from an ar_hdr structure.
 *
 * Preconditions: hdr is not NULL, hdr is a valid ar header
 *
 * Postconditions:
 *
 * @param hdr Pointer to ar_hdr to retrieve the file's mode from
 * @return File mode
 */
mode_t ar_member_mode(struct ar_hdr *hdr);

/**
 * @brief Returns a file's size ID from an ar_hdr structure.
 *
 * Preconditions: hdr is not NULL, hdr is a valid ar header
 *
 * Postconditions:
 *
 * @param hdr Pointer to ar_hdr to retrieve the file's size from
 * @return File size
 */
off_t ar_member_size(struct ar_hdr *hdr);

/**
 * @brief Read data from a file in chunks of BLOCK_SIZE bytes.
 *
 * Preconditions: fd is a valid file descriptor, buf is not NULL,
 * buf is a buffer of size size, from is between zero and the
 * size of the file - size
 *
 * Postconditions: The size bytes following from are loaded into buf
 *
 * @param fd File descriptor to read from
 * @param buf Buffer to read to
 * @param from File offset to read from
 * @param size Number of bytes to read
 * @return true on success, false otherwise
 */
bool block_read(int fd, uint8_t *buf, off_t from, size_t size);

/**
 * @brief Write data to a file in chunks of BLOCK_SIZE bytes.
 *
 * Preconditions: fd is a valid file descriptor, buf is not NULL,
 * buf is a buffer of size size
 *
 * Postconditions: The first size bytes in buf are written to the file
 * at to
 *
 * @param fd File descriptor to write to
 * @param buf Buffer to write from
 * @param from File offset to write to
 * @param size Number of bytes to write
 * @return true on success, false otherwise
 */
bool block_write(int fd, uint8_t *buf, off_t to, size_t size);

int ar_open(const char *path) {
	struct stat st;
	bool create;
	int fd;

	assert(path);

	// Are we creating the file?
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
		// Verify that the archive is valid
		if (!ar_check_global_hdr(fd)) {
			// Report error
			fprintf(stderr, "Bad global header\n");

			// Clean up
			ar_close(fd);

			return -1;
		}
	} else {
		// Write a global header
		if (ar_write_global_hdr(fd) == false) {
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
	char buf[BLOCK_SIZE]; // Read buffer
	char name[SARFNAME + 1];
	char date[SARFDATE + 1];
	char uid[SARFUID + 1];
	char gid[SARFGID + 1];
	char mode[SARFMODE + 1];
	char size[SARFSIZE + 1];
	off_t remaining;
	off_t wr_size;
	int append_fd;

	stat(path, &st);

	append_fd = open(path, O_RDONLY);

	if (append_fd < 0) {
		// Report error
		return false;
	}

	// Create NULL terminated versions of each header value
	snprintf(name, SARFNAME + 1, "%-16s", path);
	snprintf(date, SARFDATE + 1, "%12d", st.st_mtim.tv_sec);
	snprintf(uid, SARFUID + 1, "%6u", st.st_uid);
	snprintf(gid, SARFGID + 1, "%6u", st.st_gid);
	snprintf(mode, SARFMODE + 1, "%8o", st.st_mode);
	snprintf(size, SARFSIZE + 1, "%10d", st.st_size);

	// Fill the header
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

	// Write the header
	if (write(fd, &hdr, sizeof(struct ar_hdr)) == -1) {
		// Report error
		fprintf(stderr, "Write error (line %d)\n", __LINE__);

		// Clean up
		close(append_fd);

		return false;
	}

	// Read data from file and append to archive block by block
	while (lseek(append_fd, 0, SEEK_CUR) < st.st_size) {
		remaining = st.st_size - lseek(append_fd, 0, SEEK_CUR);
		wr_size = (remaining < BLOCK_SIZE) ? remaining : BLOCK_SIZE;

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
	char member_name[SARFNAME + 1];
	size_t size;
	off_t next_member;
	off_t pos;

	struct ar_hdr hdr;
	struct stat st;
	uint8_t *buf;
	int temp_fd;

	assert(fd >= 0);
	assert(name != NULL);

	fstat(fd, &st);

	// Open a temporary file
	temp_fd = open(TEMP_AR_NAME, O_CREAT | O_TRUNC | O_RDWR, DEFAULT_PERMS);
	write(temp_fd, ARMAG, SARMAG);
	lseek(fd, SARMAG, SEEK_SET);

	// For each file in the archive
	while (lseek(fd, 0, SEEK_CUR) < st.st_size) {
		ar_load_hdr(fd, &hdr);
		size = ar_member_size(&hdr);
		ar_member_name(&hdr, member_name);

		// Remove the member?
		if (strcmp(member_name, name) == 0) {
			// Skip the file
			lseek(fd, size, SEEK_CUR);
		} else {
			// Write a newline if we're not on an even byte boundary
			if ((lseek(temp_fd, 0, SEEK_CUR) % 2) == 1) {
				write(temp_fd, "\n", sizeof(char));
			}

			// Write the header
			write(temp_fd, &hdr, sizeof(struct ar_hdr));

			// Copy data to the temp file
			if (size > 0) {
				buf = (uint8_t *)malloc(size * sizeof(uint8_t));
				block_read(fd, buf, lseek(fd, 0, SEEK_CUR), size);
				block_write(temp_fd, buf, lseek(temp_fd, 0, SEEK_CUR), size);
				free(buf);
			}
		}

		// Seek to even byte boundary
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}

	// Clear archive
	if (ftruncate(fd, 0) == -1) {
		perror("Could not truncate archive");
	}

	if (fstat(temp_fd, &st) == -1) {
		perror("Could not stat temp file");
	}

	// Copy contents of temp file to archive
	buf = (uint8_t *)malloc(st.st_size * sizeof(uint8_t));
	block_read(temp_fd, buf, lseek(temp_fd, 0, SEEK_SET), st.st_size);
	block_write(fd, buf, lseek(fd, 0, SEEK_SET), st.st_size);

	// Close and remove the temp archive
	close(temp_fd);
	unlink(TEMP_AR_NAME);

	return true;
}

bool ar_extract(int fd, const char *name) {
	char buf[BLOCK_SIZE]; // Write buffer
	size_t wr_size;

	struct ar_hdr hdr;
	struct utimbuf tbuf;
	size_t size;
	size_t written;
	int extract_fd;

	assert(fd >= 0);
	assert(name != NULL);

	// Find the member
	if (ar_seek(fd, name, &hdr) == false) {
		printf("File %s not found in archive\n", name);
		return false;
	}

	// Create a file to extract to
	extract_fd = creat(name, DEFAULT_PERMS);
	if (extract_fd == -1) {
		perror("Could not open file for extraction");
		return false;
	}

	size = ar_member_size(&hdr);
	written = 0;

	// Write the data to the file block by block
	while (written < size) {
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
	
	if (close(extract_fd) == -1) {
		perror("Could not close file");
		close(extract_fd);
		return false;
	}
	
	// Set file modification time
	tbuf.actime = ar_member_date(&hdr);
	tbuf.modtime = ar_member_date(&hdr);

	if (utime(name, &tbuf) == -1) {
		perror("Unable set modification time");
		close(extract_fd);
		return false;
	}

	return true;
}

void ar_print_concise(int fd) {
	struct ar_hdr hdr;
	char name[SARFNAME + 1];
	off_t ar_size;

	assert(fd >= 0);

	ar_size = lseek(fd, 0, SEEK_END);
	lseek(fd, SARMAG, SEEK_SET);

	// For each member
	while (lseek(fd, 0, SEEK_CUR) < ar_size) {
		if (ar_load_hdr(fd, &hdr) == false) {
			// Report error
			fprintf(stderr, "Could not load ar_hdr (line %d)\n", __LINE__);
			return;
		}

		ar_member_name(&hdr, name);
		printf("%s\n", name);

		// Skip past data
		lseek(fd, ar_member_size(&hdr), SEEK_CUR);

		// If on an odd byte offset, seek ahead one byte
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}
}

void ar_print_verbose(int fd) {
	struct ar_hdr hdr;
	struct tm *time;
	char name[SARFNAME + 1];
	char ftime[SFTIME];
	char mode[SFMODE];
	time_t mtime;
	off_t ar_size;

	assert(fd >= 0);

	ar_size = lseek(fd, 0, SEEK_END);
	lseek(fd, SARMAG, SEEK_SET);

	// For each member
	while (lseek(fd, 0, SEEK_CUR) < ar_size) {
		if (ar_load_hdr(fd, &hdr) == false) {
			// Report error
			fprintf(stderr, "Could not load ar_hdr (line %d)\n", __LINE__);
			return;
		}

		ar_member_name(&hdr, name);
		ar_mode_str(ar_member_mode(&hdr), mode);
		
		mtime = ar_member_date(&hdr);
		time = localtime(&mtime);
		strftime(ftime, SFTIME, "%b %d %H:%M %Y", time);
		
		printf("%s %6d/%-6d %10d %s %s\n",
			mode,
			ar_member_uid(&hdr),
			ar_member_gid(&hdr),
			ar_member_size(&hdr),
			ftime,
			name);

		// Skip past data
		lseek(fd, ar_member_size(&hdr), SEEK_CUR);

		// If on an odd byte offset, seek ahead one byte
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}
}

bool ar_check_global_hdr(int fd) {
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

	// Verify the header is valid
	hdr_good = (memcmp(hdr, ARMAG, SARMAG) == 0);

	// Reset file position
	lseek(fd, init_pos, SEEK_SET);

	return hdr_good;
}

bool ar_write_global_hdr(int fd) {
	assert(fd >= 0);

	lseek(fd, 0, SEEK_SET);

	if (write(fd, ARMAG, SARMAG) == -1) {
		fprintf(stderr, "Write error (line %d)\n", __LINE__);
		return false;
	}

	return true;
}

bool ar_load_hdr(int fd, struct ar_hdr *hdr) {
	assert(fd >= 0);
	assert(hdr);

	if (read(fd, hdr, sizeof(struct ar_hdr)) == -1) {
		fprintf(stderr, "Error reading header data\n");
		return false;
	}

	// Verify file header
	if (memcmp(hdr->ar_fmag, ARFMAG, SARFMAG) != 0) {
		// Magic number incorrect
		fprintf(stderr, "Magic number mismatch\n");
		return false;
	}

	return true;
}

bool ar_seek(int fd, const char *name, struct ar_hdr *hdr) {
	char member_name[SARFNAME + 1];
	off_t size;

	assert(fd >= 0);
	assert(name != NULL);

	// Get file size
	size = lseek(fd, 0, SEEK_END);

	// Seek to end of global header
	lseek(fd, SARMAG, SEEK_SET);

	// For each member
	while (lseek(fd, 0, SEEK_CUR) < size) {
		if (ar_load_hdr(fd, hdr) == false) {
			return false;
		}

		// Did we find our file?
		ar_member_name(hdr, member_name);
		if (strcmp(name, member_name) == 0) {
			// Yes
			return true;
		}

		// Seek to the next header
		lseek(fd, ar_member_size(hdr), SEEK_CUR);

		// If on an odd byte offset, seek ahead one byte
		if ((lseek(fd, 0, SEEK_CUR) % 2) == 1) {
			lseek(fd, 1, SEEK_CUR);
		}
	}

	return false;
}

void ar_mode_str(mode_t mode, char *str) {
	assert(str != NULL);
	
	// Check each file permission
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

void ar_member_name(struct ar_hdr *hdr, char *name) {
	assert(hdr != NULL);

	memset(name, '\0', SARFNAME + 1);
	memcpy(name, hdr->ar_name, SARFNAME);

	// Ensure that there are no trailing spaces or slashes
	while ((name[strlen(name) - 1] == ' ') || (name[strlen(name) - 1] == '/')) {
		name[strlen(name) - 1] = '\0';
	}
}

time_t ar_member_date(struct ar_hdr *hdr) {
	char str[SARFDATE + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_date, SARFDATE);
	str[SARFDATE] = '\0';

	return strtol(str, NULL, 10);
}

uid_t ar_member_uid(struct ar_hdr *hdr) {
	char str[SARFUID + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_uid, SARFUID);
	str[SARFUID] = '\0';

	return strtol(str, NULL, 10);
}

gid_t ar_member_gid(struct ar_hdr *hdr) {
	char str[SARFGID + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_gid, SARFGID);
	str[SARFGID] = '\0';

	return strtol(str, NULL, 10);
}

mode_t ar_member_mode(struct ar_hdr *hdr) {
	char str[SARFMODE + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_mode, SARFMODE);
	str[SARFMODE] = '\0';

	return strtol(str, NULL, 8);
}

off_t ar_member_size(struct ar_hdr *hdr) {
	char str[SARFSIZE + 1];

	assert(hdr != NULL);

	memcpy(str, hdr->ar_size, SARFSIZE);
	str[SARFSIZE] = '\0';

	return strtol(str, NULL, 10);
}

bool block_read(int fd, uint8_t *buf, off_t from, size_t size) {
	size_t count;
	size_t done;

	assert(fd >= 0);
	assert(buf != NULL);
	assert(from >= 0);
	assert(size > 0);
	assert(from + size <= lseek(fd, 0, SEEK_END));

	done = 0;
	lseek(fd, from, SEEK_SET);
	while (done < size) {
		count = ((size - done) < BLOCK_SIZE) ? (size - done) : BLOCK_SIZE;
		if (read(fd, buf + done, count) == -1) {
			perror("Read error");
			return false;
		}

		done += count;
	}

	return true;
}

bool block_write(int fd, uint8_t *buf, off_t to, size_t size) {
	size_t count;
	size_t done;

	assert(fd >= 0);
	assert(buf != NULL);
	assert(to >= 0);
	assert(size > 0);

	done = 0;
	lseek(fd, to, SEEK_SET);
	while (done < size) {
		count = ((size - done) < BLOCK_SIZE) ? (size - done) : BLOCK_SIZE;
		if (write(fd, buf + done, count) == -1) {
			perror("Write error");
			return false;
		}

		done += count;
	}

	return true;
}

