#ifndef AR_H
#define AR_H

#include <ar.h>
#include <stdbool.h>
#include <stdint.h>
#include "list.h"

#define SARFMAG 2

struct ar_file {
	struct ar_hdr hdr;
	uint8_t *data;
};

struct ar {
	int fd;
	struct List files;
};

void ar_init(struct ar *a);
void ar_free(struct ar *a);
bool ar_open(struct ar *a, const char *path);
bool ar_close(struct ar *a);

size_t ar_nfiles(struct ar *a);
struct ar_file *ar_get_file(struct ar *a, size_t i);

bool ar_add_file(struct ar *a, const char *path);
bool ar_remove_file(struct ar *a, const char *name);
bool ar_extract_file(struct ar *a, const char *name);
void ar_print_concise(struct ar *a);
void ar_print_verbose(struct ar *a);

bool _ar_check_global_hdr(struct ar *a);
bool _ar_scan(struct ar *a);
bool _ar_load_file(struct ar *a);
bool _ar_load_hdr(struct ar *a, struct ar_hdr *hdr);
bool _ar_load_data(struct ar *a, struct ar_file *file);

#endif // AR_H

