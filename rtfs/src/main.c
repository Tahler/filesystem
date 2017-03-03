#include <assert.h>
#include <stdio.h>

#include "fs.h"

int main(int argc, char *argv[])
{
	fs_load("fs_file");
	fs_create("/tmp", false, 42);
	getchar();
	fs_delete("/tmp");
	getchar();
	fs_create("/temp", false, 42);
	getchar();
	fs_delete("/temp");
	return 0;
}
