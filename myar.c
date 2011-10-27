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

BOOL ar_open(struct ar *a, const char *path) {
	struct stat status;
	int err;

	assert(a);
	assert(path);

	err = stat(path, &status);
	if (err != 0) {
		// Report error
		fprintf(stderr, "File (%s) does not exist\n", path);
		return FALSE;
	}

	a->fd = open(path, 0);//, O_CREAT | DEFAULT_PERMS);
	if (a->fd == -1) {
		// Report error
		fprintf(stderr, "File (%s) could not be opened\n", path);
		return FALSE;
	}

	lseek(a->fd, 0, SEEK_SET);

	if (!_ar_check_global_hdr(a)) {
		// Report error
		fprintf(stderr, "Bad global header\n", path);

		// Clean up
		err = close(a->fd);
		if (err == -1) {
			// Report error
			fprintf(stderr, "File (%s) could not be closed\n", path);
		}

		return FALSE;
	}

	_ar_scan(a);

	return TRUE;
}

BOOL _ar_check_global_hdr(struct ar *a) {
	char hdr[SARMAG];
	int init_pos;
	BOOL hdr_good;

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

BOOL _ar_scan(struct ar *a) {
	off_t file_size;

	assert(a);
	assert(a->fd >= 0);

	file_size = lseek(a->fd, 0, SEEK_END);

	// Set file position to the first file hdr
	lseek(a->fd, SARMAG, SEEK_SET);

	while (lseek(a->fd, 0, SEEK_CUR) < file_size - 1) {
		if (_ar_load_file(a) == FALSE) {
			return FALSE;
		}
	}

	return TRUE;
}

// Assumes file pointer in fd is at the beginning of a file hdr
BOOL _ar_load_file(struct ar *a) {
	struct ar_file* file;
	assert(a);
	assert(a->fd >= 0);

	file = (struct ar_file *)malloc(sizeof(struct ar_file));
	if (file == NULL) {
		// Report error
		return FALSE;
	}

	if (!_ar_load_hdr(a, &file->hdr)) {
		// Report error
		free(file);
		return FALSE;
	}

	if (!_ar_load_data(a, file)) {
		// Report error
		free(file);
		return FALSE;
	}

	list_add_back(&a->files, (void *)file);

	return TRUE;
}

BOOL _ar_load_hdr(struct ar *a, struct ar_hdr *hdr) {
	assert(a);
	assert(a->fd >= 0);
	assert(hdr);

	if (read(a->fd, hdr, sizeof(struct ar_hdr)) == -1) {
		// Report error
		return FALSE;
	}

	if (memcmp(hdr->ar_fmag, ARFMAG, SARFMAG)) {
		// Magic number incorrect
		// Repoprt error
		return FALSE;
	}

	return TRUE;
}

BOOL _ar_load_data(struct ar *a, struct ar_file *file) {
	int data_size;

	assert(a);
	assert(a->fd >= 0);
	assert(file);

	if (file->data != NULL) {
		free(file->data);
	}

	data_size = atoi(file->hdr.ar_size);
	file->data = (BYTE *)malloc(data_size * sizeof(BYTE));
	if (file->data == NULL) {
		// Report error
		return FALSE;
	}

	if (read(a->fd, file->data, data_size) == -1) {
		// Report error
		free(file->data);
		return FALSE;
	}

	return TRUE;
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

