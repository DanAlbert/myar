/**
 * @file myar.h
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
 * Defines an interface for ar file handling.
 * 
 */
#ifndef AR_H
#define AR_H

#include <ar.h>
#include <stdbool.h>
#include <stdint.h>
#include "list.h"

/// Size of ar file header magic number
#define SARFMAG 2

/**
 * @brief A file header and data
 */
struct ar_file {
	/// ar file header
	struct ar_hdr hdr;

	/// File data contained in archive
	uint8_t *data;
};

/**
 * @brief File descriptor of the archive file and a list of contained files
 */
struct ar {
	/// Archive file descriptor
	int fd;
	
	/// List of files contained in the archive
	struct List files;
};

/**
 * @brief Initializes an ar structure.
 *
 * Preconditions: a is not NULL
 * 
 * Postconditions: a.files is initialized
 *
 * @param a Pointer to a new ar structure
 */
void ar_init(struct ar *a);

/**
 * @brief Cleans up and frees all resources associated with an archive.
 * 
 * Preconditions: a is not NULL
 * 
 * Postconditions: a.fd is closed, a.files is freed
 *
 * @param a Pointer to ar structure
 */
void ar_free(struct ar *a);

/**
 * @brief Opens and loads an archive file, creating one if it does not exist.
 * 
 * Preconditions: a is not NULL, a is initialized, a has not been opened, path is not NULL, path refers to either an existing archive or can be created
 * 
 * Postconditions: Archive referred to by path has been opened and loaded into aor a new archive has been created at path, files in archive have been loaded into a
 *
 * @param a Pointer to a newly initialized ar structure
 * @param path Path to an existing archive or location for a new archive to be created
 * @return true on success, false otherwise
 */
bool ar_open(struct ar *a, const char *path);

/**
 * @brief Writes changes to an archive and closes the archive file.
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive
 * 
 * Postconditions: Archive data is written, a.fd is closed
 *
 * @param a Pointer to archive structure
 * @return true on success, false otherwise
 */
bool ar_close(struct ar *a);


/**
 * @brief Retrieves the number of files in the archive
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive
 * 
 * Postconditions:
 *
 * @param a Pointer to ar structure
 * @return The number of files contained in the archive
 */
size_t ar_nfiles(struct ar *a);

/**
 * @brief Retrieves a pointer to the ith file in an archive
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive, i is between zero and ar.files.size
 * 
 * Postconditions:
 *
 * @param a Pointer to ar structure
 * @param i Index of file to be retrieved
 * @return Pointer to the ith ar_file structure in the archive
 */
struct ar_file *ar_get_file(struct ar *a, size_t i);

/**
 * @brief Adds a file to an archive
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive, path is not NULL, path refers to an existing file, file referred to by path is readable
 * 
 * Postconditions: The file has been added to the archive
 *
 * @param a Pointer to ar structure
 * @param path Path to the file to be added to the archive
 * @return true on success, false otherwise
 */
bool ar_add_file(struct ar *a, const char *path);

/**
 * @brief Removes a file from an archive
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive, name is not NULL, name refers to a file in the archive
 * 
 * Postconditions: The file has been removed from the archive
 *
 * @param a Pointer to ar structure
 * @param name Name of the file to be removed from the archive
 * @return true on success, false otherwise
 */
bool ar_remove_file(struct ar *a, const char *name);

/**
 * @brief Extracts a file from an archive
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive, name is not NULL, name refers to a file in the archive, a file with path name is writable
 * 
 * Postconditions: The file has been extracted from the archive
 *
 * @param a Pointer to ar structure
 * @param name Name of the file to be extracted from the archive
 * @return true on success, false otherwise
 */
bool ar_extract_file(struct ar *a, const char *name);

/**
 * @brief Prints the names of each file in the archive to stdout
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive
 * 
 * Postconditions: 
 *
 * @param a Pointer to ar structure
 */
void ar_print_concise(struct ar *a);

/**
 * @brief Prints formatted header data of each file in the archive to stdout
 * 
 * Preconditions: a is not NULL, a is initialized, a is a valid archive
 * 
 * Postconditions: 
 *
 * @param a Pointer to ar structure
 */
void ar_print_verbose(struct ar *a);


#endif // AR_H

