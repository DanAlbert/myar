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

#define DEFAULT_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define PERM_MASK 0x01ff
#define BLOCK_SIZE 4096

void _ar_file_copy(void **dst, void *src);
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
	snprintf(name, sizeof(name), "%-.15s/", path);
	memset(rindex(name, '/') + 1, ' ', sizeof(name) - ((rindex(name, '/') + 1) - name) - 1);

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
		char fname[17];

		memset(fname, '\0', sizeof(fname));
		memcpy(fname, file->hdr.ar_name, sizeof(file->hdr.ar_name));
		*rindex(fname, '/') = '\0';
		
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

void ar_print(struct ar *a) {
	for (int i = 0; i < ar_nfiles(a); i++) {
		struct ar_hdr *hdr;
		char *ctime_str;
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

		*rindex(name, '/') = '\0'; // Replace the terminating / with a null
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

		ctime_str = ctime(&date);
		*rindex(ctime_str, '\n') = '\0';
		printf("%s %6d/%-6d %10d %s %s\n", perm_str, uid, gid, size, ctime_str, name);
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

// Assumes file pointer in fd is at the beginning of a file hdr
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

