#ifndef AR_H
#define AR_H

#include <ar.h>
#include "list.h"
#include "types.h"

#define SARFMAG 2

struct ar_file {
	struct ar_hdr hdr;
	BYTE *data;
};

struct ar {
	int fd;
	struct List files;
};

void ar_init(struct ar *a);
void ar_free(struct ar *a);
BOOL ar_open(struct ar *a, const char *path);
BOOL ar_close(struct ar *a);

BOOL _ar_check_global_hdr(struct ar *a);
BOOL _ar_scan(struct ar *a);
BOOL _ar_load_file(struct ar *a);
BOOL _ar_load_hdr(struct ar *a, struct ar_hdr *hdr);
BOOL _ar_load_data(struct ar *a, struct ar_file *data);

#endif // AR_H

