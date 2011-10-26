#include "myar.h"

#define DEFAULT_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

// TODO:
// ar.fd needs to be closed somehow
// load file data
// list test

BOOL ar_open(struct ar *a, const char *path) {
	struct stat status;
	int err;

	assert(a);
	assert(path);

	err = stat(path, &status);
	if (err != 0) {
		// Report error
		return FALSE;
	}

	a->fd = open(path, O_CREAT | DEFAULT_PERMS);
	if (a->fd == -1) {
		// Report error
		return FALSE;
	}

	lseek(a->fd, 0, SEEK_SET);

	if (!_ar_check_global_hdr(a)) {
		// Report error

		// Clean up
		err = close(a->fd);
		if (err == -1) {
			// Report error
		}

		return FALSE;
	}

	_ar_scan(a);
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
	struct ar_file* file;

	assert(a);
	assert(a->fd >= 0);

	// Set file position to the first file hdr
	lseek(a->fd, SARMAG, SEEK_SET);
	_ar_load_file(a);
}

// Asumes file pointer in fd is at the beginning of a file hdr
BOOL _ar_load_file(struct ar *a) {
	struct ar_file* file;

	assert(a);
	assert(a->fd >= 0);

	file = (struct ar_file *)malloc(sizeof(struct ar_file));
	if (file == NULL) {
		// Report error
		return FALSE;
	}

	if (!_ar_load_hdr(a, file->hdr)) {
		// Report error
		free(file);
		return FALSE;
	}

	file->data = (BYTE *)malloc(file->hdr.ar_size * sizeof(BYTE));
	if (file->data == NULL) {
		// Report error
		free(file);
		return FALSE;
	}

	if (!_ar_load_file_data(a, file->data)) {
		// Report error
		free(file->data);
		free(file);
		return FALSE;
	}

	list_add(a->files, file);

	return TRUE;
}

BOOL ar_load_file_hdr(struct ar *a, struct ar_hdr *hdr) {
	assert(a);
	assert(a->fd >= 0);
	assert(hdr);

	if (read(a->fd, hdr, sizeof(struct ar_hdr)) == -1) {
		// Report error
		return FALSE;
	}

	if (memcpy(hdr->ar_fmag, ARFMAG)) {
		// Magic number incorrect
		// Repoprt error
		return FALSE;
	}

	return TRUE;
}

