#include <stdio.h>

#include <tberry/futils.h>
#include <tberry/types.h>

#include "mkfs.h"

#define INODE_SIZE 4

#define SUPER_BLK_OFFSET 0
#define INODE_TBL_OFFSET 1

struct _layout {
	usize num_inode_blks;
	usize num_data_blks;
};

struct _layout calc_layout(usize disk_size, usize blk_size)
{
	usize inodes_per_blk = blk_size / INODE_SIZE;
	usize data_blks_per_inode_blk = inodes_per_blk;

	usize num_blks = disk_size / blk_size;
	// Subtract one for the super block
	usize num_usable_blks = num_blks - 1;
	usize num_inode_blks = num_usable_blks / (1 + data_blks_per_inode_blk);
	usize num_data_blks = num_usable_blks - num_inode_blks;
	struct _layout fs_l = {
		.num_inode_blks = num_inode_blks,
		.num_data_blks = num_data_blks,
	};
	return fs_l;
}

/*
 * @f must be open for writing in binary mode
 */
u8 _format(FILE *f, usize blk_size)
{
	usize disk_size = fsizeof(f);
	struct _layout fs_l = calc_layout(disk_size, blk_size);

	fseek(f, 0, SEEK_SET);
	fwrite(&blk_size, 1, 1, f);
	usize num_blks = fs_l.num_inode_blks + fs_l.num_data_blks;
	fwrite(&num_blks, 1, 1, f);
	fwrite(&fs_l.num_inode_blks, 1, 1, f);
	fwrite(&fs_l.num_data_blks, 1, 1, f);

	// 0 is reserved for ROOT_DIR
	usize next_avl_inode = 1;
	fwrite(&next_avl_inode, 1, 1, f);

	// 0 is reserved for ROOT_DIR
	usize next_avl_blk = 1;
	fwrite(&next_avl_blk, 1, 1, f);

	return 0;
}

u8 fs_init(char *path, usize len, usize blk_size)
{
	u8 ret = 0;
	FILE *f = fopen(path, "wb");
	u8 empty_arr[len];
	for (int i = 0; i < len; ++i) {
		empty_arr[i] = 0;
	}
	bool err = fwrite(empty_arr, len, 1, f);
	if (err) {
		ret = 1;
		goto end;
	}

	// TODO: use `errno` to see the err
	err = _format(f, blk_size);
	if (err) {
		ret = 2;
		goto end;
	}

end:
	fclose(f);
	return ret;
}

u8 fs_format(char *path, usize blk_size)
{
	FILE *f = fopen(path, "w+b");
	u8 ret = _format(f, blk_size);
	fclose(f);
	return ret;
}
