#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "myar.h"

#define DEFAULT_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

// TODO:
// ar.fd needs to be closed somehow
// load file data
// list test

void _ar_file_copy(void **dst, void *src);
void _ar_file_release(void *res);

void ar_init(struct ar *a) {
	list_init(&a->files, _ar_file_copy, _ar_file_release);
}

void ar_free(struct ar *a) {
	list_free(&a->files);

	if (a->fd >= 0) {
		ar_close(a);
	}
}

bool ar_open(struct ar *a, const char *path) {
	struct stat status;
	int err;

	assert(a);
	assert(path);

	err = stat(path, &status);
	if (err != 0) {
		// Report error
		fprintf(stderr, "File (%s) does not exist\n", path);
		return false;
	}

	a->fd = open(path, 0);//, O_CREAT | DEFAULT_PERMS);
	if (a->fd == -1) {
		// Report error
		fprintf(stderr, "File (%s) could not be opened\n", path);
		return false;
	}

	lseek(a->fd, 0, SEEK_SET);

	if (!_ar_check_global_hdr(a)) {
		// Report error
		fprintf(stderr, "Bad global header\n");

		// Clean up
		ar_close(a);

		return false;
	}

	_ar_scan(a);

	return true;
}

bool ar_close(struct ar *a) {
	if (close(a->fd) == -1) {
		// Report error
		fprintf(stderr, "File could not be closed\n");
		return false;
	}

	a->fd = -1;
	return true;
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
	struct ar_file* file;
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

	if (memcmp(hdr->ar_fmag, ARFMAG, SARFMAG)) {
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

