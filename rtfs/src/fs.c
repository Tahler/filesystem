#include <tberry/types.h>
#include <tberry/file_utils.h>

#include "inode.h"

#define INODE_SIZE 4
#define INODES_PER_BLK (BLK_SIZE / INODE_SIZE)
#define DATA_BLKS_PER_INODE_BLK INODES_PER_BLK

#define SUPER_BLK_OFFSET 0
#define INODE_TBL_OFFSET 1

struct _layout {
	u64 num_inode_blks;
	u64 num_data_blks;
};

struct _layout calc_layout(u64 disk_size)
{
	u64 num_blks = disk_size / BLK_SIZE;
	// Take one for the super block
	u64 num_usable_blks = num_blks - 1;
	u64 num_inode_blks = num_usable_blks / (1 + DATA_BLKS_PER_INODE_BLK);
	u64 num_data_blks = num_usable_blks - num_inode_blks;
	struct _layout fs_l = {
		.num_inode_blks = num_inode_blks,
		.num_data_blks = num_data_blks,
	};
	return fs_l;
}

u8 _seek_to_addr(FILE *f, usize blk_num, usize offset)
{
	assert(offset < BLK_SIZE);
	usize base_addr = blk_num * BLK_SIZE;
	usize addr = base_addr + offset;
	fseek(f, addr, SEEK_SET);
}

u8 _seek_to_data_addr(FILE *f, usize blk_num, usize offset)
{
	assert(offset < BLK_SIZE);
	usize base_addr = blk_num * BLK_SIZE;
	usize addr = base_addr + offset;
	fseek(f, addr, SEEK_SET);
}

u8 _seek_to_inode(FILE *f, usize inode_num)
{
	// TODO: err handle for seeking past num inodes
	// TODO: the next two lines could be useful later

	fseek(f, x, SEEK_SET);
}

// u8 _seek_to_data_blk(FILE *f, usize blk_num)
// {
// 	usize inode_blk_num = inode_num / INODES_PER_BLK;
// 	usize inode_blk_offset = inode_num % INODES_PER_BLK;
// }
