/**
 * @file test.c
 * @author Dan Albert
 * @date Created 10/22/2011
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
 * This program runs all unit tests associated with this project.
 * 
 */
#include <stdio.h>
#include "list.h"

/**
 * @brief Test program entry point.
 *
 * Runs all unit tests for this project.
 *
 * @param argc Number of command line arguments
 * @param argv Array of strings containing command line arguments
 * @return Exit status
 */
int main(int argc, char **argv) {
	printf("list_test()\n");
	list_test();

	return 0;
}

