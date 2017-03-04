#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <tberry/debug.h>

#include "fs.h"

void pause(char *msg)
{
	DEBUG("%s", msg);
	getchar();
}

int main(int argc, char *argv[])
{
	fs_load("fs_file");
	pause("LOADED FILE");

	fs_create("/tmp", false, 42);
	struct fs_file_desc fd_tmp = fs_open("/tmp");
	DEBUG_VAL("%lu", fd_tmp.head_blk_num);
	pause("CREATED and OPENED /tmp");

	fs_create("/other", false, 42);
	struct fs_file_desc fd_other = fs_open("/other");
	DEBUG_VAL("%lu", fd_other.head_blk_num);
	pause("CREATED and OPENED /other");

	char *msg = "this message is 24 chars";
	DEBUG("Writing 2400 characters to /tmp (3 blocks)");
	for (int i = 0; i < 100; ++i) {
		fs_write(&fd_tmp, (u8 *) msg, strlen(msg));
	}
	pause("WROTE to /tmp");

	fs_seek(&fd_tmp, 0);
	assert(strlen(msg) == 24);
	char msg_cpy[24];
	fs_read(&fd_tmp, (u8 *) msg_cpy, 24);
	assert(strcmp(msg, msg_cpy) == 0);
	pause("ASSERTED that the message can be read back");

	fs_delete("/tmp");
	pause("DELETED /tmp");

	fs_create("/tmp2", false, 42);
	fd_tmp = fs_open("/tmp2");
	pause("CREATED and OPENED /tmp2 (should take over /tmp's spot)");

	msg = "srahc 42 si egassem siht";
	DEBUG("Writing 2400 characters to /tmp2 (3 blocks)");
	for (int i = 0; i < 100; ++i) {
		fs_write(&fd_tmp, (u8 *) msg, strlen(msg));
	}
	pause("WROTE to /tmp2 (should have written over /tmp's old blocks)");

	return 0;
}
