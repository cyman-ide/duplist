
/*
Copyright (C) 2021 cyman

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define ASSURE_READ_SIZE 1024

#define sarrlen(x) (sizeof(x)/sizeof(*x))

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

typedef struct FileInfo {
	size_t size;
	const char * name;
} FileInfo;

#define STRING_BUCKET_CAPACITY 65536 // 64 KiB
typedef struct StringBucket {
	uint32_t capacity;
	uint32_t length;
	char * data;
} StringBucket;

StringBucket
StringBucket_create() {
	StringBucket result;

	result.capacity = STRING_BUCKET_CAPACITY;
	result.length = 0;
	result.data = malloc(STRING_BUCKET_CAPACITY);

	return result;
}

// copied from blkmv
static void
paths_join(char * dest, int num_paths, const char * paths []) {
	int dest_index = 0;
	for (int path_index=0; path_index < num_paths; ++path_index) {
		const char * s = paths[path_index];
		while (*s != '\0') {
			dest[dest_index] = *s;
			dest_index++, s++;
		}
		if (path_index != num_paths-1 && dest[dest_index-1] != '/')
			dest[dest_index++] = '/';
	}
	dest[dest_index] = '\0';
}

// copied from blkmv
static void
make_new_path(const char * dir_name, const char * d_name, char * new_path) {
	if (strcmp(dir_name, ".") != 0) {
		const char * to_join [] = {dir_name, d_name};
		paths_join(new_path, 2, to_join);
	} else {
		strcpy(new_path, d_name);
	}
}

// modified from blkmv
static int
find_recursive(const char * dir_name, FileInfo ** file_list, StringBucket ** file_name_buffer) {
	DIR * directory = opendir(dir_name);
	struct dirent * entry;
	if (!directory) {
		fprintf(stderr, "could not open directory \"%s\"\n", dir_name);
		return -1;
	}

	while ((entry = readdir(directory)) != NULL) {
		if (entry->d_type == DT_REG) { // regular file
			char new_path [PATH_MAX];
			make_new_path(dir_name, entry->d_name, new_path);

			size_t filename_len = strlen(new_path);
			StringBucket * bucket = &(*file_name_buffer)[arrlen(*file_name_buffer)-1];
			if (bucket->length + filename_len + 1 > bucket->capacity) {
				arrput(*file_name_buffer, StringBucket_create());
				bucket = &(*file_name_buffer)[arrlen(*file_name_buffer)-1];
			}
			strcpy(&bucket->data[bucket->length], new_path);
			const char * new_filename = &bucket->data[bucket->length];
			bucket->length += filename_len + 1;

			struct stat file_stat;
			if (stat(new_filename, &file_stat)) {
				fprintf(stderr, "error getting file info from %s\n", new_filename);
				return -1;
			}
			FileInfo new_file = {
				.size = file_stat.st_size,
				.name = new_filename,
			};
			arrput(*file_list, new_file);
		} else if (entry->d_type == DT_DIR && entry->d_name[0] != '.') { // directory
			char new_path [PATH_MAX];
			make_new_path(dir_name, entry->d_name, new_path);

			int result = find_recursive(new_path, file_list, file_name_buffer);
			if (result) {
				closedir(directory);
				return result;
			}
		}
	}

	closedir(directory);
	return 0;
}

static int gSortDirection = 1;
static const char * gFileNameBuffer;

static int
sort_function(const void * voida, const void * voidb) {
	const FileInfo *a = (FileInfo*)voida, *b = (FileInfo*)voidb;
	int result = b->size - a->size;
	if (result != 0) {
		return gSortDirection * result;
	} else {
		return strcmp(a->name, b->name);
	}
}

static void
get_duplicates(const FileInfo * file_list, StringBucket * file_name_buffer, FileInfo ** ptr_dup_list) {
	const FileInfo * p_last_item = &file_list[0];
	size_t dup_count = 0;

	uint8_t * buffer1 = malloc(2 * ASSURE_READ_SIZE);
	uint8_t * buffer2 = buffer1 + ASSURE_READ_SIZE;

	for (int i=1; i < arrlen(file_list); ++i) {
		if (file_list[i].size == p_last_item->size) {
			const char * filename1 = file_list[i].name;
			const char * filename2 = p_last_item->name;
			FILE * file1 = fopen(filename1, "rb");
			FILE * file2 = fopen(filename2, "rb");

			int is_dup = 1, error_occured = 0;
			if (file1 != NULL && file2 != NULL) {
				int large_enough = file_list[i].size >= ASSURE_READ_SIZE;
				int read_size = large_enough ? ASSURE_READ_SIZE : file_list[i].size;
				if (fread(buffer1, read_size, 1, file1) == 1 &&
			        fread(buffer2, read_size, 1, file2) == 1) {
					for (int r=0; r < read_size; ++r) {
						if (buffer1[r] != buffer2[r]) {
							is_dup = 0;
							break;
						}
					}
				} else {
					error_occured = 1;
				}
			} else {
				error_occured = 1;
			}

			if (file1 != NULL) fclose(file1);
			if (file2 != NULL) fclose(file2);

			if (!error_occured) {
				if (is_dup) {
					if (dup_count == 0) {
						arrput(*ptr_dup_list, *p_last_item);
					}
					arrput(*ptr_dup_list, file_list[i]);
					dup_count++;
				}
			} else {
				fprintf(stderr, "failed to read files.\n");
				exit(-1);
			}
		} else {
			dup_count = 0;
		}
		p_last_item = &file_list[i];
	}

	free(buffer1);
}

static void
data_size_fmt(char * dest, size_t num_bytes) {
	static const char * suffix_list [] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};

	double size = (double)num_bytes;
	for (int s=0; s < sarrlen(suffix_list); ++s) {
		if (size < 1024) {
			sprintf(dest, "%.2f %s", size, suffix_list[s]);
			return;
		} else {
			size /= 1024.0;
		}
	}

	strcpy(dest, "BIG"); // this isn't meant to ever happen
}

//  return values:
//  0 : task completed successfully
//  1 : error in arguments
// -1 : unexpected error
int
main(int argc, const char ** argv) {
	if (argc < 2) {
		fprintf(stderr, "no arguments\n");
		return 1;
	}

	const char * search_dir_name = NULL;
	for (int i=1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'r') {
				gSortDirection = -1;
			} else if (argv[i][1] == '-') {
				if (strcmp(&argv[i][2], "reverse") == 0) {
					gSortDirection = -1;
				} else {
					fprintf(stderr, "unknown option \"%s\"\n", &argv[i][2]);
					return 1;
				}
			} else {
				fprintf(stderr, "unknown option \'%c\'\n", argv[i][1]); 
				return 1;
			}
		} else {
			if (search_dir_name == NULL) {
				search_dir_name = argv[i];
			} else {
				fprintf(stderr, "you can only pass one directory.\n");
				return 1;
			}
		}
	}

	if (chdir(search_dir_name)) {
		fprintf(stderr, "failed to change working directory.\n");
		return -1;
	}
	StringBucket * file_name_buffer = NULL;
	arrput(file_name_buffer, StringBucket_create()); // find_recursive will expect an item
	FileInfo * file_list = NULL;
	if (find_recursive(".", &file_list, &file_name_buffer)) {
		return -1;
	}

	qsort(file_list, arrlen(file_list), sizeof(FileInfo), sort_function);
	FileInfo * dup_list = NULL;
	get_duplicates(file_list, file_name_buffer, &dup_list);
	int count_dups = arrlen(dup_list);
	if (count_dups == 0)
		return 0;

	size_t last_item_size = 0;
	char format_buffer [64]; // 64 is way more than we need
	for (int i=0; i < count_dups; ++i) {
		if (dup_list[i].size != last_item_size) {
			printf("\n");
		}
		data_size_fmt(format_buffer, dup_list[i].size);
		printf("(%s) %s\n", format_buffer, dup_list[i].name);
		last_item_size = dup_list[i].size;
	}

	for (int i=arrlen(file_name_buffer)-1; i >= 0; --i) {
		free(file_name_buffer[i].data);
	}

	arrfree(dup_list);
	arrfree(file_list);
	arrfree(file_name_buffer);

	return 0;
}

