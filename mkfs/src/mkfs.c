#include <stdio.h>
#include <string.h>

#include <tberry/futils.h>
#include <tberry/types.h>
#include <tberry/debug.h>

#include "mkfs.h"

#define INODE_SIZE 4

#define SUPER_BLK_OFFSET 0
#define INODE_TBL_OFFSET 1

// Flags `dir`, `read`, `write`, and `data_blk` 0
#define ROOT_INODE_VAL 0x80600000

/*
 * Essentially gets written to the super block
 */
struct _layout {
	usize disk_size;
	usize blk_size;
	usize num_blks;
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
		.disk_size = disk_size,
		.blk_size = blk_size,
		.num_blks = num_blks,
		.num_inode_blks = num_inode_blks,
		.num_data_blks = num_data_blks,
	};
	return fs_l;
}

u8 _write_super_blk(FILE *f, struct _layout layout)
{
	// First available is 1 because 0 is reserved for ROOT_DIR
	usize next_avl_inode = 1;
	usize next_avl_blk = 1;

	DEBUG("Formatting...");
	DEBUG_VAL("%d", layout.disk_size);
	DEBUG_VAL("%d", layout.blk_size);
	DEBUG_VAL("%d", layout.num_blks);
	DEBUG_VAL("%d", layout.num_inode_blks);
	DEBUG_VAL("%d", layout.num_data_blks);
	DEBUG_VAL("%d", next_avl_inode);
	DEBUG_VAL("%d", next_avl_blk);

	usize to_write[] = {
		layout.disk_size,
		layout.blk_size,
		layout.num_blks,
		layout.num_inode_blks,
		layout.num_data_blks,
		next_avl_inode,
		next_avl_blk,
	};
	fseek(f, 0, SEEK_SET);
	fwrite(&to_write, 8, 7, f);

	return 0;
}

u8 _write_root_inode(FILE *f, usize blk_size)
{
	usize addr = INODE_TBL_OFFSET * blk_size;
	fseek(f, addr, SEEK_SET);

	u32 root_inode = ROOT_INODE_VAL;
	fwrite(&root_inode, sizeof(root_inode), 1, f);

	return 0;
}

/*
 * @f must be open for writing in binary mode
 */
u8 _format(FILE *f, usize blk_size)
{
	u8 ret = 0;

	usize disk_size = fsizeof(f);
	struct _layout layout = calc_layout(disk_size, blk_size);

	ret = _write_super_blk(f, layout);
	if (ret) {
		return ret;
	}
	ret = _write_root_inode(f, blk_size);
	return ret;
}

u8 _extend_file_len(FILE *f, usize len)
{
	// Set the file size to @len
	fseek(f, len - 1, SEEK_SET);
	u8 blank_byte = 0;
	fwrite(&blank_byte, 1, 1, f);
	return 0;
}

u8 fs_init(char *path, usize len, usize blk_size)
{
	u8 ret = 0;
	FILE *f = fopen(path, "wb");
	_extend_file_len(f, len);

	// TODO: use `errno` to see the err
	bool err = _format(f, blk_size);
	if (err) {
		ret = 1;
	}
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
