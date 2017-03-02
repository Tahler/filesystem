# Our design

*Ext4, hold the extra*

The filesystem is strongly influenced by the Ext4 filesystem, but takes
liberties for simplification where possible.

## Overview

Every file (directories are files) is described through two pieces: an *inode*
and a linked list of data blocks.

Inode, a term borrowed from the Ext filesystem, holds metadata about the file.

The inode points to the first data block (i.e. the head of the linked list).

## Blocks

The entire disk is split into 4 KiB blocks.

## Super block

The first block on disk is known as the "super block".

The super block contains the block size, number of blocks on disk, the inode
table size (in blocks), the number of data blocks, the next available inode
entry, and the next available data block.

## Inode table

The next N blocks, where N is the inode table size (specified in the super
block), contain the inode table.

The inode table is a sequential array of inode entries.

### Layout

The first entry (i.e. inode) in the inode table represents the root dir.

### Inode entries

Each inode entry represents a file with 32 bits.

| Dir?  | Owner  | Read? | Write? | Data ptr |
|-------|--------|-------|--------|----------|
| 1 bit | 8 bits | 1 bit | 1 bit  | 21 bits  |

Fields:
- `Directory?` indicates if the file is a directory; if the bit is off, the file
  is a regular file
- `Owner` is an 8 bit unsigned integer which is the user ID
- `Read?` indicates if this file has read permissions
- `Write?` indicates if this file has write permissions
- `Data ptr` is the data block number of the first data block; 21 bits allows
  indexing into 2,097,152 blocks (4 GiB)

### Size

The inode table is exactly large enough, such that if you kept allocating 1
block files, you would simultaneously run out of disk space and inode space.

#### Sizing algorithm

An inode is 4 bytes long and a block is 4 kilobytes long.
Given `N` gigabytes of disk space, how many inodes need to exist such that there
is 1 inode for each block?

Python implementation:
```python
block_len = 4096
inode_len = 4
# how many inodes can fit in one block of memory
inodes_per_block = block_len // inode_len # 1024
# one inode block can describe this many data blocks
data_blocks_per_inode_block = inodes_per_block
gb = 1024 * 1024 * 1024 # 1073741824
blocks_per_gb = gb // block_len # 262144

def calc_num_inode_blocks(num_gbs):
    num_blocks = num_gbs * blocks_per_gb
    num_inode_blocks = num_blocks // (1 + data_blocks_per_inode_block)
    num_data_blocks = num_blocks - num_inode_blocks
    return num_inode_blocks
```

## Data blocks

The rest of the disk is for data. Each data block is addressed by a data block
number, which can then be multiplied by the block size and offset by the start
of data space sector for the physical address.

Each data block first starts with a 21 bit integer (padded with 11 bits)
indicating the next data block. The number is 0 if there is no next data block.

The first data block is pointed to by the file's inode.

## Filename limit

Filenames are limited to 255 characters. This way the directory entries can be
easily traversed, created, and deleted.

## Directory structure

A "directory" is still a file, whose contents are contained in data blocks.

The contents are a linear array of entries. Each entry consists of a filename
and inode number. The filename is a null-terminated string (limited to 255
characters) and the inode number is an unsigned 16 bit integer.

Therefore, to `cd $arg`, for example, one would have to linearly traverse the
directory file contents for the entry with the filename `$arg`.

Note: This is how the Ext2 filesystem worked. Ext3 improved by using a
balanced "hash tree". Read more [here](https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Hash_Tree_Directories).

## Pseudocode for file creation / deletion

```
alloc_data_blk()
  next_available = super_blk.next_available_blk
  if data_blks[next_available] == 0
    super_blk.next_available_blk = next_available + 1
  else
    super_blk.next_available_blk = data_blks[next_available]
  return next_available

get_child(parent_dir_inode, child_name)
  // `find_child` linear searches for the child_name, returning the inode number
  // associated with it, or 0 if none is found
  return find_child(data_blks[parent_dir_inode], child_name)

// Returns the data block that holds the directory contents for the path
get_dir_blk(path)
  path_breadcrumbs = path.split("/")
  inode = inode_tbl[0]
  foreach breadcrumb in path_breadcrumbs
    inode = get_child(inode, breadcrumb)
  return inode.data_ptr

create(parent_dir_full_path, filename, is_dir, owner)
  next_available = super_blk.next_available_inode
  if inode_tbl[next_available] == 0
    super_blk.next_available_inode = next_available + 1
  else
    super_blk.next_available_inode = inode_tbl[next_available]

  inode = new(inode)
  inode.owner = owner
  inode.is_dir = is_dir
  inode.data_ptr = alloc_data_blk()

  inode_tbl[next_available] = inode

  parent_dir_blk = get_dir_blk(parent_dir_full_path)
  // `add_child` pushes an entry into the dir_blk
  add_child(parent_dir_blk, inode)

// Note: don't call this on non-empty directories or suffer a filesystem leak
delete(parent_dir_full_path, filename)
  parent_dir_blk = get_dir_blk(parent_dir_full_path)
  // `remove_child` writes 0s across the entry matching `filename`, swaps the
  // deleted entry with the last entry, then returns the deleted entry's inode
  inode = remove_child(parent_dir_blk, filename)
  inode_tbl.remove(inode)
```
