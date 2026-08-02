/*
	Copyright 2006 Luigi Auriemma
	Copyright 2009-2011 Bryan Cain
	Copyright 2026 Damon Caskey (dcurrent)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

	http://www.gnu.org/licenses/gpl.txt
*/

#include "borpak.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#ifdef WIN32
	#include <direct.h>
	#include <windows.h>
	#include "stristr.h"
	#include "scandir.h"

	#define MKDIR(x)    mkdir(x)
	#define PATHSLASH   '\\'
#else
	#include <unistd.h>

	#define stristr		strcasestr
	#define MKDIR(x)    mkdir(x, 0755)
	#define PATHSLASH   '/'
#endif


/*
* File I/O and PAK format handling for borpak.
*
* Header layout:
*
*     bytes 0-3: PACK magic, ASCII "PACK"
*     bytes 4-7: little-endian uint32
*
* PAK32:
*     50 41 43 4B 00 00 00 00
*     P  A  C  K  original PACK value 0
*
* PAK64:
*     50 41 43 4B 01 00 00 00
*     P  A  C  K  PAK64 format id 1
*
* Legacy engines only support the original PACK 
* layout, where bytes 4-7 are 0. PAK32 preserves 
* that exact header and table/footer layout for
* compatibility. Newer engines use the same 32-bit 
* field as a format id: 0 for PAK32, 1 for PAK64.
*/

static const unsigned char pack_magic_bytes[PACK_MAGIC_BYTE_COUNT] = { 'P', 'A', 'C', 'K' };
static const size_t 		pack_magic_len = sizeof(pack_magic_bytes);

static pak_entry_node_t *pak_entries = NULL;
static pak_size_t pak_table_size = 0;

const char *format_name(pak_format_t format) {
	return format == PAK_FORMAT_PAK64 ? "PACK v1 PAK64 (64-bit)" : "PACK v0 PAK32 (32-bit)";
}

/*
* Limits for PAK32 format in gigabytes for 
* user-friendly display.
*/
static const double pak32_data_limit_gb = (double)PAK32_DATA_LIMIT / 1000000000.0;
static const double pak32_table_cushion_gb = (double)PAK32_TABLE_CUSHION / 1000000000.0;
static const double pak32_archive_limit_gb = (double)PAK32_ARCHIVE_LIMIT / 1000000000.0;
static const double pak_nonstreamed_asset_limit_gb = (double)PAK_NONSTREAMED_ASSET_LIMIT / 1000000000.0;

/*
* Caskey, Damon V.
* 2026-06-10
*
* Prints details about the selected PAK format.
*/
void print_format_details(pak_format_t format) {
	printf("- format: %s\n", format_name(format));

	if(format == PAK_FORMAT_PAK32) {
		printf("- PAK32 data limit: %.2f GB.\n", pak32_data_limit_gb);
		printf("- PAK32 table/footer cushion: %.2f GB\n", pak32_table_cushion_gb);
		printf("- PAK32 archive limit: %.2f GB\n", pak32_archive_limit_gb);
		printf("- Use PAK64 for larger archives or new-engine-only builds.\n");
	} else {
		printf("- PAK64 format id: %u\n", (unsigned int)PAK_FORMAT_PAK64);
		printf("- PAK64 offsets/sizes: 64-bit\n");
		printf("- PAK64 non-streamed asset limit: %.2f GB\n", pak_nonstreamed_asset_limit_gb);
		printf("- PAK64 streamed asset exceptions: .wav, .ogg, .oga, and .webm\n");
	}
}

/*
* Caskey, Damon V.
* 2026-06-10
*
* Writes a fixed-size block to a file.
* Exits through write_err() if the write fails.
*/
void file_write(const void *buffer, size_t size, FILE *file_object) {
	if(fwrite(buffer, size, 1, file_object) != 1) {
		write_err();
	}
}

/*
* Caskey, Damon V.
* 2026-06-10
*
* Returns the current file position as a 64-bit 
* archive offset.
*/
pak_offset_t pak_tell(FILE *pak_file_object) {
#ifdef WIN32
	__int64 pos;

	pos = _ftelli64(pak_file_object);
	if(pos < 0) {
		std_err();
	}

	return (pak_offset_t)pos;
#else
	off_t pos;

	pos = ftello(pak_file_object);
	if(pos < 0) {
		std_err();
	}

	return (pak_offset_t)pos;
#endif
}

/*
* Returns the size of an already-open file without 
* changing the final read/write position seen by callers.
*/
pak_size_t pak_file_size(FILE *file_object) {
	pak_offset_t current_position;
	pak_offset_t file_size;

	current_position = pak_tell(file_object);
	pak_seek_relative(file_object, 0, SEEK_END);
	file_size = pak_tell(file_object);
	pak_seek_to_offset(file_object, current_position);

	return (pak_size_t)file_size;
}

/*
* Seeks using the platform file API.
*
* The offset is signed because SEEK_END needs 
* negative movement when reading archive footers, 
* such as the final 4-byte PAK32 footer or 8-byte 
* PAK64 footer.
*/
void pak_seek_relative(FILE *pak_file_object, int64_t offset, int origin) {
#ifdef WIN32
	if(_fseeki64(pak_file_object, offset, origin) != 0) {
		std_err();
	}
#else
	if(fseeko(pak_file_object, (off_t)offset, origin) != 0) {
		std_err();
	}
#endif
}

/*
* Seeks to an absolute archive offset.
*
* Archive offsets are stored as unsigned values, 
* but the platform seek APIs use signed offsets. 
* Reject values that cannot be represented safely.
*/
void pak_seek_to_offset(FILE *pak_file_object, pak_offset_t offset) {
	if(offset > (pak_offset_t)INT64_MAX) {
		printf("\nError: archive offset is too large for this platform.\n");
		exit(1);
	}

	pak_seek_relative(pak_file_object, (int64_t)offset, SEEK_SET);
}

/*
* Caskey, Damon V.
* 2026-06-10
*
* Checks whether adding one more file would exceed 
* the PACK v0 PAK32 budget. The data section must 
* stop at PAK32_DATA_LIMIT, leaving PAK32_TABLE_CUSHION 
* bytes for every file table record and the final 32-bit 
* footer while keeping the completed archive under INT_MAX.
*/
void validate_pak32_budget(const char *file_path, pak_offset_t data_offset, pak_size_t data_size, pak_u32 record_size) {
	pak_size_t table_budget_without_footer;

	if(data_offset > PAK32_DATA_LIMIT || data_size > PAK32_DATA_LIMIT - data_offset) {
		printf("\nError: PACK v0 PAK32 data limit would be exceeded by %s\n", file_path);
		printf("PACK v0 PAK32 data limit is %.2f GB.\n", pak32_data_limit_gb);
		printf("Use -f pak64 to create a PACK v1 PAK64 archive.\n");
		exit(1);
	}

	table_budget_without_footer = PAK32_TABLE_CUSHION - PAK32_FOOTER_SIZE;

	if(pak_table_size > table_budget_without_footer ||
		(pak_size_t)record_size > table_budget_without_footer - pak_table_size) {
		printf("\nError: PACK v0 PAK32 file table cushion would be exceeded by %s\n", file_path);
		printf("Reserved file table cushion is %.2f GB including the final footer.\n",
			pak32_table_cushion_gb);
		printf("Use -f pak64 to create a PACK v1 PAK64 archive.\n");
		exit(1);
	}
}

static int archive_path_has_extension(const char *archive_path, const char *extension) {
	size_t archive_path_length;
	size_t extension_length;
	size_t extension_index;
	char archive_character;
	char extension_character;

	if(!archive_path || !extension) {
		return 0;
	}

	archive_path_length = strlen(archive_path);
	extension_length = strlen(extension);

	if(archive_path_length < extension_length) {
		return 0;
	}

	for(extension_index = 0; extension_index < extension_length; extension_index++) {
		archive_character = archive_path[archive_path_length - extension_length + extension_index];
		extension_character = extension[extension_index];

		if(tolower((unsigned char)archive_character) != tolower((unsigned char)extension_character)) {
			return 0;
		}
	}

	return 1;
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Identify media formats with bounded readers and
* 64-bit file positions in the engine. PAK64 may
* retain these assets beyond the legacy INT_MAX cap.
*/
static int is_streamed_archive_asset(const char *archive_path) {
	return archive_path_has_extension(archive_path, ".wav") ||
		archive_path_has_extension(archive_path, ".ogg") ||
		archive_path_has_extension(archive_path, ".oga") ||
		archive_path_has_extension(archive_path, ".webm");
}

static int validate_asset_size(const char *archive_path, const char *source_file_path, pak_size_t source_file_size, pak_format_t format) {
	if(format == PAK_FORMAT_PAK64 &&
		source_file_size > PAK_NONSTREAMED_ASSET_LIMIT &&
		!is_streamed_archive_asset(archive_path)) {
		printf("\nError: PAK64 non-streamed asset is too large: %s\n", archive_path);
		printf("Source file: %s\n", source_file_path ? source_file_path : "(unknown source)");
		printf("Non-streamed asset limit is %.2f GB.\n", pak_nonstreamed_asset_limit_gb);
		printf("Only .wav, .ogg, .oga, and .webm files may exceed this limit because the engine streams them.\n");
		return 0;
	}

	return 1;
}


/*
* Caskey, Damon V.
* 2026-06-10
*
* Validates that an archive path contains 
* only allowed characters. Returns 1 on pass
* and 0 on failure.
*/
static int is_archive_path_character_allowed(unsigned char character) {
	
	/* 
	* Check if the character is an uppercase letter.
	*/
	if(character >= 'A' && character <= 'Z') {
		return 1;
	}

	/* 
	* Check if the character is a lowercase letter.
	*/
	if(character >= 'a' && character <= 'z') {
		return 1;
	}

	/* 
	* Check if the character is a digit.
	*/
	if(character >= '0' && character <= '9') {
		return 1;
	}

	/* 
	* Check if the character is an allowed punctuation mark.
	*/
	switch(character) {
		case '/':
		case '\\':
		case '.':
		case '_':
		case '-':
			return 1;
		default:
			return 0;
	}
}

int validate_archive_path(const char *archive_path, const char *source_file_path) {
	size_t character_index;
	unsigned char character;

	if(!archive_path || !archive_path[0]) {
		printf("\nError: empty archive path for %s\n", source_file_path ? source_file_path : "(unknown source)");
		return 0;
	}

	for(character_index = 0; archive_path[character_index]; character_index++) {
		character = (unsigned char)archive_path[character_index];

		if(!is_archive_path_character_allowed(character)) {
			printf("\nError: invalid archive path character in %s\n", archive_path);
			printf("Source file: %s\n", source_file_path ? source_file_path : "(unknown source)");
			printf("Invalid byte: 0x%02x at position %zu\n", (unsigned int)character, character_index);
			printf("Archive paths may only use A-Z, a-z, 0-9, slash, backslash, dot, underscore, and hyphen.\n");
			return 0;
		}
	}

	return 1;
}

int copy_archive_path(char *destination, size_t destination_size, const char *source) {
	size_t source_index = 0;
	size_t destination_index = 0;

	if(destination_size == 0) {
		return 0;
	}

	while(source[source_index] == '.' &&
		(source[source_index + 1] == '/' || source[source_index + 1] == '\\')) {
		source_index += 2;
	}

	while(source[source_index]) {
		if(destination_index >= destination_size - 1) {
			destination[destination_index] = '\0';
			return 0;
		}

		destination[destination_index] = source[source_index] == '/' ? '\\' : source[source_index];
		source_index++;
		destination_index++;
	}

	destination[destination_index] = '\0';
	return 1;
}

/*
* Caskey, Damon V.
* 2026-06-10
*
* Derives the archive root from the build 
* input directory. Relative "data" and absolute 
* "/.../data" now both produce archive names 
* rooted at "data", which keeps PAK32 output 
* compatible with legacy engines that ask for 
* data\\... paths. A build from "." keeps the 
* old behavior of storing the current directory's
* contents without a synthetic root prefix.
*/
void copy_archive_root_from_input_directory(char *destination, size_t destination_size, const char *input_directory) {
	const char *component_end;
	const char *component_start;
	size_t component_length;
	char component[ARCHIVE_PATH_MAX_LEN];

	if(destination_size == 0) {
		return;
	}
	destination[0] = '\0';

	if(!input_directory || !input_directory[0]) {
		return;
	}

	component_end = input_directory + strlen(input_directory);
	while(component_end > input_directory &&
		(component_end[-1] == '/' || component_end[-1] == '\\')) {
		component_end--;
	}

	if(component_end == input_directory) {
		return;
	}

	component_start = component_end;
	while(component_start > input_directory &&
		component_start[-1] != '/' && component_start[-1] != '\\') {
		component_start--;
	}

	component_length = (size_t)(component_end - component_start);
	if(component_length == 0 ||
		(component_length == 1 && component_start[0] == '.')) {
		return;
	}

	if(component_length == 2 && component_start[0] == '.' && component_start[1] == '.') {
		printf("\nError: input directory would create an unsafe archive root: %s\n", input_directory);
		exit(1);
	}

	if(component_length >= sizeof(component)) {
		printf("\nError: archive root path too long: %s\n", input_directory);
		exit(1);
	}

	memcpy(component, component_start, component_length);
	component[component_length] = '\0';
	if(!copy_archive_path(destination, destination_size, component)) {
		printf("\nError: archive root path too long: %s\n", input_directory);
		exit(1);
	}
}

/*
* Caskey, Damon V.
* 2026-06-10
*
* Rejects archive paths that could escape the 
* extraction directory.
*
* PAK entries should always be relative paths. 
* Absolute paths, Windows drive-qualified paths, 
* and any ".." path component are considered unsafe.
*/
int is_unsafe_path(const char *name) {
	const char *path_cursor;
	const char *component_start;
	size_t component_length;

	/*
	* Empty or missing names are invalid archive entries.
	*/
	if(!name || !name[0]) {
		return 1;
	}

	/*
	* Absolute Unix or Windows-style paths would write outside the
	* extraction directory.
	*/
	if(name[0] == '/' || name[0] == '\\') {
		return 1;
	}

#ifdef WIN32
	/*
	* Reject Windows drive-qualified paths such as "C:\foo".
	*/
	if(strchr(name, ':')) {
		return 1;
	}
#endif

	path_cursor = name;

	/*
	* Walk each path component and reject ".." anywhere in the path.
	* This catches:
	*
	*   ..
	*   ../file
	*   folder/..
	*   folder/../file
	*   folder\..\file
	*/
	while(*path_cursor) {
		component_start = path_cursor;

		while(*path_cursor && *path_cursor != '/' && *path_cursor != '\\') {
			path_cursor++;
		}

		component_length = (size_t)(path_cursor - component_start);

		if(component_length == 2 &&
			component_start[0] == '.' &&
			component_start[1] == '.') {
			return 1;
		}

		/*
		* Skip the path separator and continue with the next component.
		*/
		if(*path_cursor) {
			path_cursor++;
		}
	}

	return 0;
}

int main(int argc, char *argv[]) {

	pak_entry_node_t *entry_node;
	pak_entry_node_t *entry_node_free;
	
	pak_entry_t    entry;
	FILE    *pak_file_object = NULL;
	pak_offset_t table_offset;
	pak_offset_t table_end_offset;
	pak_offset_t table_start_offset;
	pak_offset_t remaining_table_bytes;
	pak_offset_t header_size;
	pak_u32 entry_base_size;
	int     i;
	size_t	name_length;
	int		list    = 0;
	int		build   = 0;
	pak_u32	pack_format_id = 0;
	size_t	file_count   = 0;
	int 	fgetc_result = 0;
	int	exit_status = 0;
	pak_format_t format = PAK_FORMAT_PAK32;

	unsigned char pack_magic_buffer[PACK_MAGIC_BYTE_COUNT];
	char	curdir[512];
	char	archive_root[ARCHIVE_PATH_MAX_LEN];
	char	*input_dir    = NULL;
	char	*pattern      = NULL;
	char	*pak_filename;

	setbuf(stdout, NULL);

#ifdef BORPAK_WINPAUSE
	atexit(winpause);
#endif

	fputs("\n"
		"BOR PAK extractor/builder v0.1\n"
		"originally by Luigi Auriemma\n"
		"e-mail: aluigi@autistici.org\n"
		"web:    aluigi.org\n"
		"\n"
		"Updated to v0.2 by SX\n"
		"e-mail: sumolx@gmail.com\n"
		"web:    https://www.chronocrash.com\n"
		"\n"
		"Updated to v0.3 by Plombo\n"
		"web:    https://www.chronocrash.com\n"
		"\n"
		"Updated to v0.4 by Damon Caskey (dcurrent)\n"
		"web:    https://www.chronocrash.com, https://www.caskeys.com/dc\n"
		"\n", stdout);

	if(argc < 2) {
		printf("\n"
			"Usage: %s [options] <file.PAK>\n"
			"\n"
			"-d DIR   files folder, default is the current\n"
			"-b       build a PAK from the files folder, default is extraction\n"
			"-f FORMAT   specify the format for building the archive\n"
			"\t\t pak32 - V0 32-bit (default). Max data size: %.2f GB.\n"
			"\t\t pak64 - V1 64-bit. Requires OpenBOR 4x or later. Max data size: 9 exabytes. \n"
			"-l       list files without extracting\n"
			"-p PAT   extract only the files which contain PAT in their name\n"
			"\n", argv[0], pak32_data_limit_gb);
		exit(1);
	}

	argc--;
	for(i = 1; i < argc; i++) {
		
		if(argv[i][0] != '-') {
			 break;
		}
		
		if(!strcmp(argv[i], "-f")) {
			const char *format_argument;

			if(i + 1 >= argc) {
				printf("\nError: missing argument for -f option\n\n");
				exit(1);
			}

			format_argument = argv[++i];
			if(!strcmp(format_argument, "pak32")) {
				format = PAK_FORMAT_PAK32;
			} else if(!strcmp(format_argument, "pak64")) {
				format = PAK_FORMAT_PAK64;
			} else {
				printf("\nError: invalid format specified for -f option\n\n");
				exit(1);
			}

			continue;
		}

		switch(argv[i][1]) {
			case 'd': 
			
				if(i + 1 >= argc) {
					printf("\nError: missing argument for -d option\n\n");
					exit(1);
				}
			
				input_dir = argv[++i];    break;
			
			case 'b': 
			
				build = 1;            break;
		
			case 'l': 
			
				list  = 1;            break;
			case 'p': 
			
				if(i + 1 >= argc) {
					printf("\nError: missing argument for -p option\n\n");
					exit(1);
				}

				pattern  = argv[++i];    break;
			default: {
				printf("\nError: wrong command-line argument (%s)\n\n", argv[i]);
				exit(1);
				} break;
		}
	}
	
	if(i != argc) {
		printf("\nError: expected one archive filename after options\n\n");
		exit(1);
	}

	pak_filename = argv[argc];

	if(!pak_filename || pak_filename[0] == '-') {
		printf("\nError: missing archive filename\n\n");
		exit(1);
	}

	if(build) {
		pack_format_id = (pak_u32)format;
		pak_table_size = 0;

		if(!input_dir) {
			printf("\n"
				"Error: please specify the files directory with the -d option and don't specify\n"
				"       the current\n\n");
			exit(1);
		}

		copy_archive_root_from_input_directory(archive_root, sizeof(archive_root), input_dir);

		printf("- create file: %s\n", pak_filename);
		print_format_details(format);
		printf("- directory: %s\n", input_dir);
		printf("- archive root: %s\n\n", archive_root[0] ? archive_root : "(directory contents)");
		pak_file_object = fopen(pak_filename, "rb");
		if(pak_file_object) {
			fclose(pak_file_object);
			printf("- a file with the same name already exists, overwrite? (y/N)\n  ");
			
			fgetc_result = fgetc(stdin);
			if(fgetc_result == EOF || tolower((unsigned char)fgetc_result) != 'y') {
				exit(1);
			}
		}
		pak_file_object = fopen(pak_filename, "wb");

		if(!pak_file_object){
			std_err();
		}

		file_write(pack_magic_bytes, pack_magic_len, pak_file_object);
		write_le_uint(pak_file_object, pack_format_id, 32);

		/* 
		* Print header based on format. 
		*/
		if(format == PAK_FORMAT_PAK64) {
			printf(
				"            offset                 size   filename\n"
				"-------------------------------------------------\n");
		} else {
			printf(
				"    offset       size   filename\n"
				"--------------------------------\n");
		}

		if(pack_directory(pak_file_object, input_dir, archive_root, format) < 0) {
			printf("\nError: an error occurred during the directory scanning\n");
			exit_status = 1;
			goto quit;
		}

		table_offset = pak_tell(pak_file_object);
		if(format == PAK_FORMAT_PAK32 && table_offset > PAK32_DATA_LIMIT) {
			printf("\nError: PACK v0 PAK32 data area has exceeded the limit.\n");
			printf("PACK v0 PAK32 data limit is %.2f GB.\n", pak32_data_limit_gb);
			printf("Use -f pak64 to create a PACK v1 PAK64 archive.\n");
			exit(1);
		}

		if(format == PAK_FORMAT_PAK64) {
			printf("- files info offset: %016" PRIx64 "\n", (uint64_t)table_offset);
		} else {
			printf("- files info offset: %08x\n", (unsigned int)table_offset);
		}

		entry_node = pak_entries;
		while(entry_node) {
			if(format == PAK_FORMAT_PAK64) {
				write_le_uint(pak_file_object, entry_node->entry.record_size,  32);
				write_le_u64(pak_file_object, entry_node->entry.data_offset);
				write_le_u64(pak_file_object, entry_node->entry.data_size);
				file_write(entry_node->entry.name, entry_node->entry.record_size - PAK64_ENTRY_BASE_SIZE, pak_file_object);
			} else {
				write_le_uint(pak_file_object, entry_node->entry.record_size,  32);
				write_le_uint(pak_file_object, (pak_u32)entry_node->entry.data_offset,  32);
				write_le_uint(pak_file_object, (pak_u32)entry_node->entry.data_size, 32);
				file_write(entry_node->entry.name, entry_node->entry.record_size - PAK32_ENTRY_BASE_SIZE, pak_file_object);
			}

			entry_node_free = entry_node;
			entry_node     = entry_node->next;
			free(entry_node_free->entry.name);
			free(entry_node_free);
			file_count++;
		}

		if(format == PAK_FORMAT_PAK64) {
			write_le_u64(pak_file_object, table_offset);
		} else {
			pak_offset_t footer_offset;

			/*
			* The PAK32 PACK footer is a final 32-bit table offset. The
			* build-time data limit and table cushion should keep this safe;
			* keep a final guard here in case a future code path bypasses them.
			*/
			footer_offset = pak_tell(pak_file_object);

			if(footer_offset > PAK32_ARCHIVE_LIMIT - PAK32_FOOTER_SIZE) {
				printf("\nError: PACK v0 PAK32 archive exceeds the 32-bit size limit.\n");
				printf("Use -f pak64 to create a PACK v1 PAK64 archive.\n");
				exit(1);
			}

			write_le_uint(pak_file_object, (pak_u32)table_offset, 32);
		}

	} else {

		printf("- open file: %s\n", pak_filename);
		pak_file_object = fopen(pak_filename, "rb");

		if(!pak_file_object){
			std_err();
		}

		if(input_dir) {
			printf("- change directory: %s\n", input_dir);
			
			if(chdir(input_dir) < 0) {
				std_err();
			}
		}

		if(fread(pack_magic_buffer, 1, pack_magic_len, pak_file_object) != pack_magic_len) {
			exit_status = 1;
			goto quit;
		}

		if(memcmp(pack_magic_buffer, pack_magic_bytes, pack_magic_len) != 0) {
			printf("\nError: unsupported pack format\n");
			exit_status = 1;
			goto quit;
		}

		pack_format_id = read_le_uint(pak_file_object, 32);
		header_size = PACK_HEADER_SIZE;

		if(pack_format_id == (pak_u32)PAK_FORMAT_PAK32) {
			format = PAK_FORMAT_PAK32;
			entry_base_size = PAK32_ENTRY_BASE_SIZE;
		} else if(pack_format_id == (pak_u32)PAK_FORMAT_PAK64) {
			format = PAK_FORMAT_PAK64;
			entry_base_size = PAK64_ENTRY_BASE_SIZE;
		} else {
			printf("\nError: unsupported PACK format id %u\n", (unsigned int)pack_format_id);
			exit_status = 1;
			goto quit;
		}

		printf("- format: %s\n", format_name(format));
		printf("- format id: %u\n", (unsigned int)pack_format_id);

		if(format == PAK_FORMAT_PAK64) {
			pak_seek_relative(pak_file_object, -8, SEEK_END);
			table_end_offset = pak_tell(pak_file_object);
			table_offset = read_le_u64(pak_file_object);
		} else {
			pak_seek_relative(pak_file_object, -4, SEEK_END);
			table_end_offset = pak_tell(pak_file_object);
			table_offset = read_le_uint(pak_file_object, 32);
		}

		if(table_offset < header_size || table_offset > table_end_offset) {
			printf("\nError: malformed %s file table offset\n", format_name(format));
			exit_status = 1;
			goto quit;
		}

		table_start_offset = table_offset;

		pak_seek_to_offset(pak_file_object, table_offset);

		while(table_offset < table_end_offset) {
			remaining_table_bytes = table_end_offset - table_offset;

			if(remaining_table_bytes < entry_base_size) {
				printf("\nError: malformed %s file table\n", format_name(format));
				exit_status = 1;
				goto quit;
			}

			entry.record_size = read_le_uint(pak_file_object, 32);

			if(format == PAK_FORMAT_PAK64) {
				entry.data_offset = read_le_u64(pak_file_object);
				entry.data_size   = read_le_u64(pak_file_object);
			} else {
				entry.data_offset = read_le_uint(pak_file_object, 32);
				entry.data_size   = read_le_uint(pak_file_object, 32);
			}

			if(entry.record_size <= entry_base_size) {
				printf("\nError: malformed %s file table entry\n", format_name(format));
				exit_status = 1;
				goto quit;
			}

			if(entry.record_size > remaining_table_bytes) {
				printf("\nError: malformed %s file table\n", format_name(format));
				exit_status = 1;
				goto quit;
			}

			name_length = entry.record_size - entry_base_size;

			entry.name = malloc(name_length + 1);
			
			if(!entry.name) { 
				std_err();
			}
			

			if(fread(entry.name, name_length, 1, pak_file_object) != 1) {
				free(entry.name);
				std_err();
			}

			/*
			*  Archive names are stored as raw bytes. Add the terminator
			*  before using the name with normal C string functions.
			*/
			entry.name[name_length] = '\0';

			if(!pattern || (pattern && stristr(entry.name, pattern))) {
				printf("  %s\n", entry.name);
				if(!list) {

					/*
					* Verify safe extraction before writing any files.
					* This prevents malicious or malformed archive entries
					* from escaping the extraction directory.
					*/
					if(is_unsafe_path(entry.name)) {
						printf("  skipping %s: path traversal detected\n", entry.name);
					} else {

						if(entry.data_offset > table_start_offset ||
							entry.data_size > table_start_offset - entry.data_offset) {
							printf("\nError: malformed %s file data range: %s\n", format_name(format), entry.name);
							free(entry.name);
							exit_status = 1;
							goto quit;
						}

						extract_file(pak_file_object, entry.name, entry.data_offset, entry.data_size);
					}
				}
				file_count++;
			}

			free(entry.name);
			table_offset += entry.record_size;
			pak_seek_to_offset(pak_file_object, table_offset);
		}
	}

quit:
	if(pak_file_object) {
		fclose(pak_file_object);
	}
	printf("- finished: %zu files\n", file_count);
	printf("- current directory: %s\n", getcwd(curdir, sizeof(curdir)));
	
	return exit_status;
}

int pack_file(FILE *pak_file_object, const char *source_file_path, const char *archive_file_path, pak_format_t format) {

	pak_entry_node_t   *entry_node;
	FILE    *source_file_object;
	size_t  bytes_read;
	size_t  name_length;
	pak_offset_t data_offset;
	pak_size_t	data_size;
	pak_size_t	source_file_size;
	pak_u32 entry_base_size;
	pak_u32 record_size;

	unsigned char 	copy_buffer[8192];
	char archive_name[ARCHIVE_PATH_MAX_LEN];

	if(!copy_archive_path(archive_name, sizeof(archive_name), archive_file_path)) {
		printf("\nError: archive path too long: %s\n", archive_file_path);
		return -1;
	}

	if(!validate_archive_path(archive_name, source_file_path)) {
		return -1;
	}

	// printf("  %s\r", archive_name);
	source_file_object = fopen(source_file_path, "rb");
	if(!source_file_object){ 
		std_err();
	}

	entry_base_size = format == PAK_FORMAT_PAK64 ? PAK64_ENTRY_BASE_SIZE : PAK32_ENTRY_BASE_SIZE;
	name_length = strlen(archive_name) + 1;

	if(name_length > (size_t)UINT32_MAX - entry_base_size) {
		printf("\nError: file name too long for %s format: %s\n", format_name(format), archive_name);
		exit(1);
	}

	record_size = (pak_u32)(entry_base_size + name_length);
	data_offset = pak_tell(pak_file_object);
	source_file_size = pak_file_size(source_file_object);

	if(!validate_asset_size(archive_name, source_file_path, source_file_size, format)) {
		fclose(source_file_object);
		return -1;
	}

	if(format == PAK_FORMAT_PAK32) {
		validate_pak32_budget(archive_name, data_offset, source_file_size, record_size);
	}

	data_size = 0;

	while((bytes_read = fread(copy_buffer, 1, sizeof(copy_buffer), source_file_object)) != 0) {
		file_write(copy_buffer, bytes_read, pak_file_object);
		data_size += bytes_read;
	}

	if(ferror(source_file_object)) {
		std_err();
	}

	fclose(source_file_object);

	if(data_size != source_file_size) {
		printf("\nError: file changed while packing: %s\n", source_file_path);
		exit(1);
	}

	for(entry_node = pak_entries; entry_node && entry_node->next; entry_node = entry_node->next);

	if(entry_node) {
		entry_node->next = malloc(sizeof(pak_entry_node_t));

		if(!entry_node->next) { 
			std_err();
		}
		entry_node       = entry_node->next;

	} else {
		pak_entries      = malloc(sizeof(pak_entry_node_t));

		if(!pak_entries) { 
			std_err();
		}
		entry_node       = pak_entries;
	}
	entry_node->next = NULL;

	entry_node->entry.record_size = record_size;
	entry_node->entry.data_offset = data_offset;
	entry_node->entry.data_size = data_size;
	entry_node->entry.name = malloc(name_length);

	if(!entry_node->entry.name) { 
		std_err();
	}

	memcpy(entry_node->entry.name, archive_name, name_length);
	pak_table_size += record_size;

	if(format == PAK_FORMAT_PAK64) {
		printf("  %016" PRIx64 " %20" PRIu64 "   %s\n", (uint64_t)data_offset, (uint64_t)data_size, archive_name);
	} else {
		printf("  %08x %10" PRIu64 "   %s\n", (unsigned int)data_offset, (uint64_t)data_size, archive_name);
	}

	return(0);
}


void extract_file(FILE *pak_file_object, char *file_path, pak_offset_t data_offset, pak_size_t data_size) {
	FILE    *output_file_object;
	size_t  chunk_size;
	size_t  bytes_read;
	pak_size_t 	bytes_remaining;
	unsigned char  copy_buffer[8192];
	char	*path_cursor;

	pak_seek_to_offset(pak_file_object, data_offset);

	for(path_cursor = file_path; *path_cursor; path_cursor++) {
		if((*path_cursor == '\\') || (*path_cursor == '/')) {
			*path_cursor = 0;
			MKDIR(file_path);
			*path_cursor = PATHSLASH;
		} else {
			*path_cursor = (char)tolower((unsigned char)*path_cursor);
		}
	}

	output_file_object = fopen(file_path, "wb");
	if(!output_file_object){ 
		std_err();
	}

	bytes_remaining = data_size;

	while(bytes_remaining) {
		chunk_size = sizeof(copy_buffer);

		if(bytes_remaining < (pak_size_t)chunk_size) {
			chunk_size = (size_t)bytes_remaining;
		}

		bytes_read = fread(copy_buffer, 1, chunk_size, pak_file_object);

		if(bytes_read != chunk_size) {
			printf("\nError: unexpected end of archive while extracting %s\n", file_path);
			exit(1);
		}

		file_write(copy_buffer, bytes_read, output_file_object);
		bytes_remaining -= bytes_read;
	}

	fclose(output_file_object);
}

pak_u32 read_le_uint(FILE *pak_file_object, int bits) {
	pak_u32   num;
	int     i;
	int 	bytes;
	unsigned char  tmp[sizeof(pak_u32)];
	
	bytes = bits >> 3;
	
	if(fread(tmp, bytes, 1, pak_file_object) != 1) {
		std_err();
	}

	for(num = i = 0; i < bytes; i++) {
		num |= ((pak_u32)tmp[i] << (i << 3));
	}

	return(num);
}

uint64_t read_le_u64(FILE *pak_file_object) {
	uint64_t num;
	int i;
	unsigned char tmp[sizeof(uint64_t)];

	if(fread(tmp, sizeof(tmp), 1, pak_file_object) != 1) {
		std_err();
	}

	for(num = i = 0; i < (int)sizeof(tmp); i++) {
		num |= ((uint64_t)tmp[i] << (i << 3));
	}

	return num;
}

void write_le_uint(FILE *pak_file_object, pak_u32 num, int bits) {
	int     i;
	int bytes;
	unsigned char  tmp[sizeof(pak_u32)];
	
	bytes = bits >> 3;
	for(i = 0; i < bytes; i++) {
		tmp[i] = (num >> (i << 3)) & 0xff;
	}
	file_write(tmp, bytes, pak_file_object);
}

void write_le_u64(FILE *pak_file_object, uint64_t value) {
	int i;
	unsigned char tmp[sizeof(uint64_t)];

	for(i = 0; i < (int)sizeof(tmp); i++) {
		tmp[i] = (unsigned char)((value >> (i << 3)) & 0xff);
	}

	file_write(tmp, sizeof(tmp), pak_file_object);
}

int pack_directory(FILE *pak_file_object, const char *source_directory_path, const char *archive_directory_path, pak_format_t format) {
	char  child_source_path[SOURCE_PATH_MAX_LEN];
	char  child_archive_path[ARCHIVE_PATH_MAX_LEN];

	struct  dirent  **namelist;
	int             n,
					i;
	int written;

	n = scandir(source_directory_path, &namelist, NULL, alphasort);
	if(n < 0) {
		/*
		* scandir() fails for normal files. Let pack_file() handle the path
		* instead of calling stat() first. Some C runtimes report EOVERFLOW
		* from stat() for files above 2GB even when the file can be opened and
		* copied with the large-file APIs used by pack_file().
		*/
		if(pack_file(pak_file_object, source_directory_path, archive_directory_path, format) < 0) {
			return(-1);
		}

	} else {
		for(i = 0; i < n; i++) {    // Changed by Plombo
			
			/*
			*  scandir() allocates every returned entry, including "." and "..".
			*  Free skipped entries before continuing.
			*/
			if(!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) {
				free(namelist[i]);
				continue;
			}

			/*
			*  Construct the source path and the archive path separately. This
			*  keeps absolute host paths out of the pack table while still reading
			*  files from the exact input directory the user supplied.
			*/
			written = snprintf(child_source_path, sizeof(child_source_path), "%s/%s", source_directory_path, namelist[i]->d_name);
			if(written < 0 || (size_t)written >= sizeof(child_source_path)) {
				printf("\nError: source path too long: %s/%s\n", source_directory_path, namelist[i]->d_name);
				goto quit;
			}

			if(archive_directory_path && archive_directory_path[0]) {
				written = snprintf(child_archive_path, sizeof(child_archive_path), "%s/%s", archive_directory_path, namelist[i]->d_name);
			} else {
				written = snprintf(child_archive_path, sizeof(child_archive_path), "%s", namelist[i]->d_name);
			}
			if(written < 0 || (size_t)written >= sizeof(child_archive_path)) {
				printf("\nError: archive path too long: %s/%s\n", archive_directory_path ? archive_directory_path : "", namelist[i]->d_name);
				goto quit;
			}

			/*
			* Recurse into the child path and let scandir() decide whether it is a
			* directory. If it is a file, the recursive call falls through to
			* pack_file(). This avoids stat() size overflows on large streamed
			* media files.
			*/
			if(pack_directory(pak_file_object, child_source_path, child_archive_path, format) < 0) {
				goto quit;
			}
			free(namelist[i]);
		}
		free(namelist);
	}

	return(0);
quit:
	for(; i < n; i++) {
		free(namelist[i]);
	}
	free(namelist);
	return(-1);
}


void write_err(void) {
	printf("\nError: write error; probably out of disk space\n\n");
	exit(1);
}

void std_err(void) {
	perror("\nError");
	exit(1);
}

#ifdef BORPAK_WINPAUSE
void winpause(void) {
	system("pause");
}
#endif
