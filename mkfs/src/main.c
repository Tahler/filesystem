#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tberry/err.h>
#include <tberry/futils.h>
#include <tberry/types.h>
#include <tberry/debug.h> // TODO: remove

#include "mkfs.h"

#define MAX_ARG_LEN 255
#define DEFAULT_BLK_SIZE 4096
#define DEFAULT_FS_SIZE "1G"

const char *usage_opt =
	"[OPTIONS] FILE\n"
	"\n"
	"    FILE                 The file to be used to simulate the filesystem.\n"
	"                         Created if the file does not already exist.\n"
	"\n"
	"Options:\n"
	"    -h --help            Display this message\n"
	"    -b --block-size SIZE Set the block size of the filesystem in bytes.\n"
	"                         If omitted, '4096' is assumed.\n"
	"    -s --fs-size SIZE    Set the size of the filesystem.\n"
	"                         If SIZE is suffixed by 'k', 'm', or 'g' (either\n"
	"                         upper-case or lower-case), then it is interpreted in\n"
	"                         kibibytes, mebibytes, or gibibytes, respectively.\n"
	"                         If omitted, and FILE exists, then FILE's size is used.\n"
	"                         Otherwise, '1G' is assumed.";

void exit_print_usage(char *cmd, int exit_status)
{
	eprintf("Usage: %s %s\n", cmd, usage_opt);
	exit(exit_status);
}

/*
 * Prints @msg then usage statement to stderr. The program then exits with a
 * status code of 1.
 *
 * @msg need not include a newline
 */
void exit_invalid_args(char *cmd, char *msg)
{
	eprintf("%s\n", msg);
	exit_print_usage(cmd, 1);
}

enum flag_opt {
	_NONE,
	HELP,
	BLK_SIZE,
	FS_SIZE,
};

/*
 * Returns `_NONE` if @arg did not match a potential flag
 */
enum flag_opt parse_opt(char *arg)
{
	if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
		return HELP;
	} else if (strcmp(arg, "-b") == 0 || strcmp(arg, "--block-size") == 0) {
		return BLK_SIZE;
	} else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--fs-size") == 0) {
		return FS_SIZE;
	}
	return _NONE;
}

usize parse_size(char *size_arg)
{
	usize size_arg_len = strlen(size_arg);
	assert(size_arg_len > 0);
	char last = size_arg[size_arg_len - 1];
	// Falls through to multiply the ending bytes nicely
	usize multiplier = 1;
	switch (tolower(last)) {
	case 'g':
		multiplier *= 1024;
	case 'm':
		multiplier *= 1024;
	case 'k':
		multiplier *= 1024;
	default:;
		// Remove the last character if it was a unit
		char digits[size_arg_len];
		usize num_digits = isalpha(last)
			? size_arg_len - 1
			: size_arg_len;
		// TODO: kinda dumb, should err handle strtol
		memcpy(digits, size_arg, num_digits);
		for (int i = 0; i < num_digits; ++i) {
			assert(isdigit(digits[i]));
		}
		usize num = strtol(digits, /*(char **)*/NULL, 10);
		return num * multiplier;
	}
}

int main(int argc, char *argv[])
{
	// TODO: bounds checking, option argument checking
	if (argc < 2) {
		exit_invalid_args(argv[0], "Missing: FILE");
	}
	bool fs_size_spec = false;
	usize blk_size = DEFAULT_BLK_SIZE;
	char *fs_size = DEFAULT_FS_SIZE;

	// Parse OPTIONS
	for (int i = 1; i < argc - 1; ++i) {
		char *arg = argv[i];
		enum flag_opt flag_opt = parse_opt(arg);

		if (flag_opt == _NONE) {
			char err_msg[MAX_ARG_LEN];
			sprintf(err_msg, "Invalid option: %s", arg);
			exit_invalid_args(argv[0], err_msg);
		} else if (flag_opt == HELP) {
			exit_print_usage(argv[0], 0);
		} else {
			i += 1;
			assert(i < argc - 1);
			switch (flag_opt) {
			case BLK_SIZE:
				blk_size = strtol(argv[i], NULL, 10);
				break;
			case FS_SIZE:
				fs_size_spec = true;
				fs_size = argv[i];
				break;
			default:
				assert(false);
			}
		}
	}
	printf("starting up\n");
	char *filename = argv[argc - 1];

	u8 ret = 0;
	if (fexists(filename) && !fs_size_spec) {
		// Reformat the file, using its current size
		// ret = fs_format(filename, blk_size);
		printf("would call format(%s, %lu)", filename, blk_size);
	} else {
		usize fs_size_in_bytes = parse_size(fs_size);

		// Create / overwrite the file
		// ret = fs_init(filename, fs_size_in_bytes, blk_size);
		printf("would call init(%s, %lu, %lu)", filename, fs_size_in_bytes, blk_size);
	}

	return ret;
}
