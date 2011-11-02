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
 * Defines an interface for UNIX archive file handling.
 * 
 */
#ifndef AR_H
#define AR_H

#include <ar.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Opens and verifies an archive file, creating one if it does not exist.
 * 
 * Preconditions: path is not NULL, path refers to either an existing archive or
 * can be created
 * 
 * Postconditions: Archive referred to by path has been opened or a new archive
 * has been created at path
 *
 * @param path Path to an existing archive or location for a new archive to be
 * created.
 * @return File descriptor of the open archive file.
 * @retval -1 An error occured while opening the archive.
 */
int ar_open(const char *path);

/**
 * @brief Closes an open archive file.
 * 
 * Preconditions: fd is a valid file descriptor
 * 
 * Postconditions: fd is closed
 *
 * @param fd File descriptor of an open archive.
 */
void ar_close(int fd);

/**
 * @brief Appends a file to an archive.
 * 
 * Preconditions: fd is an file descriptor for a valid archive, path is not
 * NULL, path refers to an existing file, file referred to by path is readable
 * 
 * Postconditions: The file has been appended to the archive
 *
 * @param fd File descriptor of an open archive
 * @param path Path to the file to be appended to the archive
 * @return true on success, false otherwise
 */
bool ar_append(int fd, const char *path);

/**
 * @brief Removes a member from an archive.
 * 
 * Preconditions: fd is an file descriptor for a valid archive, name is not
 * NULL, name refers to a member of the archive
 * 
 * Postconditions: The member has been removed from the archive
 *
 * @param fd File descriptor of an open archive
 * @param name Name of the member to be removed from the archive
 * @return true on success, false otherwise
 */
bool ar_remove(int fd, const char *name);

/**
 * @brief Extracts a member from an archive
 * 
 * Preconditions: fd is an file descriptor for a valid archive, name is not
 * NULL, name refers to a member of the archive, a file with path name is
 * writable
 * 
 * Postconditions: The member has been extracted from the archive
 *
 * @param fd File descriptor of an open archive
 * @param name Name of the member to be extracted from the archive
 * @return true on success, false otherwise
 */
bool ar_extract(int fd, const char *name);

/**
 * @brief Prints the names of each member in the archive to stdout
 * 
 * Preconditions: fd is an file descriptor for a valid archive
 * 
 * Postconditions: 
 *
 * @param fd File descriptor to an open archive
 */
void ar_print_concise(int fd);

/**
 * @brief Prints formatted header data of each member of the archive to stdout
 * 
 * Preconditions: fd is an file descriptor for a valid archive
 * 
 * Postconditions: 
 *
 * @param fd File descriptor of an open archive
 */
void ar_print_verbose(int fd);


#endif // AR_H

