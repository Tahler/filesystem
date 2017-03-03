#include <assert.h>

#include "fs.h"

int main(int argc, char *argv[])
{
	fs_load("fs_file");
	fs_create("/tmp", false, 42);
	return 0;
}
