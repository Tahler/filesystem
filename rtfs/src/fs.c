#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <tberry/futils.h>
#include <tberry/types.h>

#include "fs.h"

#define INODE_SIZE 4
#define INODES_PER_BLK (BLK_SIZE / INODE_SIZE)
#define DATA_BLKS_PER_INODE_BLK INODES_PER_BLK

#define SUPER_BLK_OFFSET 0
#define INODE_TBL_OFFSET 1

#define NEXT_AVL_INODE_OFFSET (5 * sizeof(usize))
#define NEXT_AVL_BLK_OFFSET (6 * sizeof(usize))

#define DATA_BLK_NEXT_BLK_NUM_OFFSET 0
#define DATA_BLK_NEXT_BLK_NUM_LEN sizeof(usize)
#define DATA_BLK_USABLE_OFFSET DATA_BLK_NEXT_BLK_NUM_LEN

#define ROOT_DIR_BLK_NUM 0

// The first header item is `usize next_blk_ptr`
#define DIR_BLK_NEXT_AVL_ENTRY_OFFSET sizeof(usize)
#define DIR_BLK_HEADER_LEN (sizeof(usize) + sizeof(u32))
#define DIR_BLK_ENTRY_TBL_OFFSET DIR_BLK_HEADER_LEN

#define DIR_BLK_ENTRY_LEN (sizeof(bool) + MAX_FILENAME_LEN + sizeof(u32))
// Offsets into the entry
#define DIR_BLK_ENTRY_IN_USE_OFFSET 0
#define DIR_BLK_FILENAME_OFFSET (DIR_BLK_ENTRY_IN_USE_OFFSET + sizeof(bool))
#define DIR_BLK_INODE_OFFSET (DIR_BLK_FILENAME_OFFSET + MAX_FILENAME_LEN)

#define PATH_DELIM "/"

struct _layout {
	usize disk_size;
	usize blk_size;
	usize data_blk_usable_len;
	usize num_blks;
	usize num_inode_blks;
	usize num_data_blks;
} layout;

/*
 * The opened backing file
 */
FILE *bk_f;

/*
 * Loads the layout of @backing_file into memory. THIS MUST BE CALLED BEFORE ANY
 * OTHER FUNCTIONS.
 *
 * @backing_file should have been made through `mkfs.ext4holdtheextra`
 */
u8 fs_load(char *backing_file)
{
	bk_f = fopen(backing_file, "r+b");
	assert(fsizeof(bk_f) > 0);
	fread(&layout.disk_size, sizeof(usize), 1, bk_f);
	fread(&layout.blk_size, sizeof(usize), 1, bk_f);
	fread(&layout.num_blks, sizeof(usize), 1, bk_f);
	fread(&layout.num_inode_blks, sizeof(usize), 1, bk_f);
	fread(&layout.num_data_blks, sizeof(usize), 1, bk_f);
	layout.data_blk_usable_len = layout.blk_size - sizeof(usize);
	return 0;
}

u32 _new_inode(bool is_dir, u8 owner, bool read, bool write, usize data_ptr)
{
	u32 inode = 0;
	if (is_dir) {
		inode += 0x80000000;
	}
	inode += owner << 23;
	if (read) {
		inode += 0x00400000;
	}
	if (write) {
		inode += 0x00200000;
	}
	inode += data_ptr & 0x001FFFFF;
	return inode;
}

bool _inode_is_dir(u32 inode)
{
	return (inode >> 31) & 1;
}

/*
 * Returns the 8 bit owner field of &inode
 */
u8 _inode_owner(u32 inode)
{
	return (u8) (inode >> 23);
}

bool _inode_has_read_perm(u32 inode)
{
	return (inode >> 22) & 1;
}

bool _inode_has_write_perm(u32 inode)
{
	return (inode >> 21) & 1;
}

/*
 * Returns the data ptr field of @inode; only the last 21 bits should be active
 */
u32 _inode_data_ptr(u32 inode)
{
	u32 mask = (1 << 21) - 1;
	return inode & mask;
}

void _seek_to_blk_offset(usize blk_num, usize offset)
{
	assert(blk_num < layout.num_blks);
	assert(offset < layout.blk_size);

	usize base_addr = blk_num * layout.blk_size;
	usize addr = base_addr + offset;
	fseek(bk_f, addr, SEEK_SET);
}

void _seek_to_inode(usize inode_num)
{
	usize num_inodes = layout.num_inode_blks * layout.blk_size * INODE_SIZE;
	assert(inode_num < num_inodes);

	usize num_inodes_per_blk = layout.blk_size / INODE_SIZE;
	usize inode_blk_num = inode_num / num_inodes_per_blk;
	usize inode_offset = inode_num % num_inodes_per_blk;
	usize abs_blk_num = INODE_TBL_OFFSET + inode_blk_num;
	usize abs_offset = inode_offset * INODE_SIZE;
	_seek_to_blk_offset(abs_blk_num, abs_offset);
}

void _seek_to_data_addr(usize blk_num, usize offset)
{
	assert(blk_num < layout.num_data_blks);

	usize base_of_data_blks = 1 + layout.num_inode_blks;
	usize abs_blk_num = base_of_data_blks + blk_num;
	_seek_to_blk_offset(abs_blk_num, offset);
}

void _seek_to_data_usable_addr(usize blk_num, usize offset)
{
	_seek_to_data_addr(blk_num, DATA_BLK_USABLE_OFFSET + offset);
}

usize _read_usize()
{
	usize x;
	fread(&x, sizeof(usize), 1, bk_f);
	return x;
}

void _write_usize(usize x)
{
	fwrite(&x, 1, 1, bk_f);
}

u32 _read_u32()
{
	u32 x;
	fread(&x, sizeof(u32), 1, bk_f);
	return x;
}

void _write_u32(u32 x)
{
	fwrite(&x, 1, 1, bk_f);
}

void _read_str(usize len, char *dest)
{
	fread(dest, sizeof(char), len, bk_f);
}

void _write_str(usize len, char *src)
{
	fwrite(src, len, 1, bk_f);
}

bool _read_bool()
{
	bool b;
	fread(&b, sizeof(bool), 1, bk_f);
	return b;
}

void _write_bool(bool b)
{
	fwrite(&b, 1, 1, bk_f);
}

usize _read_next_avl_inode()
{
	_seek_to_blk_offset(SUPER_BLK_OFFSET, NEXT_AVL_INODE_OFFSET);
	return _read_usize();
}

void _write_next_avl_inode(usize next_avl_inode)
{
	_seek_to_blk_offset(SUPER_BLK_OFFSET, NEXT_AVL_INODE_OFFSET);
	_write_usize(next_avl_inode);
}

usize _read_next_avl_blk()
{
	_seek_to_blk_offset(SUPER_BLK_OFFSET, NEXT_AVL_BLK_OFFSET);
	return _read_usize();
}

void _write_next_avl_blk(usize next_avl_blk)
{
	_seek_to_blk_offset(SUPER_BLK_OFFSET, NEXT_AVL_BLK_OFFSET);
	_write_usize(next_avl_blk);
}

u32 _get_inode(usize inode_num)
{
	_seek_to_inode(inode_num);
	return _read_u32();
}

void _set_inode(usize inode_num, u32 inode)
{
	_seek_to_inode(inode_num);
	_write_u32(inode);
}

/*
 * Returns the `inode_num` that is next available
 */
usize _alloc_inode()
{
	usize next_avl_inode = _read_next_avl_inode();

	u32 next_addr = _get_inode(next_avl_inode);

	usize next_next_avl_inode = next_addr == 0
		? next_avl_inode + 1
		: next_addr;
	_write_next_avl_inode(next_next_avl_inode);

	return next_avl_inode;
}

/*
 * Returns the deleted inode
 */
u32 _dealloc_inode(usize inode_num)
{
	// TODO: would usually be usize, but since this is written in, must be
	//       u32 to prevent overflow
	u32 next_avl_inode = _read_next_avl_inode();

	u32 deleted = _get_inode(inode_num);
	_set_inode(inode_num, next_avl_inode);

	_write_next_avl_inode(inode_num);

	return deleted;
}

void _clear_blk(usize blk_num)
{
	_seek_to_blk_offset(blk_num, 0);
	u8 zero_buf[layout.blk_size];
	memset(zero_buf, 0, layout.blk_size);
	fwrite(zero_buf, sizeof(u8), layout.blk_size, bk_f);
}

void _clear_data_blk(usize blk_num)
{
	usize data_blks_offset = 1 + layout.num_inode_blks;
	_clear_blk(data_blks_offset + blk_num);
}

/*
 * Returns the `data_blk_num` that is next available
 */
usize _alloc_data_blk()
{
	usize next_avl_blk = _read_next_avl_blk();

	_seek_to_data_addr(next_avl_blk, 0);
	usize next_addr = _read_usize();

	usize next_next_avl_blk = next_addr == 0
		? next_avl_blk + 1
		: next_addr;
	_write_next_avl_blk(next_next_avl_blk);

	_clear_data_blk(next_avl_blk);

	return next_avl_blk;
}

/*
 * Recursively deallocates the "linked-list" of blocks after this one as well
 */
void _dealloc_data_blk(usize data_blk_num)
{
	_seek_to_data_addr(data_blk_num, 0);
	usize next_blk = _read_usize();

	usize next_avl_blk = _read_next_avl_blk();
	_seek_to_data_addr(data_blk_num, 0);
	_write_usize(next_avl_blk);
	_write_next_avl_blk(data_blk_num);

	if (next_blk != ROOT_DIR_BLK_NUM) {
		_dealloc_data_blk(next_blk);
	}
}

/*
 * "/hello/me" -> "/hello"
 */
void _get_parent_dir_path(char *path, char *dest)
{
	int last_delim = 0;
	for (int i = 0; path[i] != '\0'; ++i) {
		if (path[i] == '/') {
			last_delim = i;
		}
	}
	// TODO: likely bug here, off by 1
	memcpy(dest, path, last_delim);
	dest[last_delim] = '\0';
}

/*
 * "/hello/me" -> "me"
 */
void _get_end_filename(char *path, char *dest)
{
	int j = 0;
	for (int i = 0; path[i] != '\0'; ++i) {
		if (path[i] == '/') {
			j = 0;
		} else {
			dest[j] = path[i];
			j += 1;
		}
	}
	dest[j] = '\0';
}

void _seek_to_dir_entry_num(usize dir_blk_num, usize entry_num)
{
	usize tbl_base_addr = DIR_BLK_ENTRY_TBL_OFFSET;
	usize entry_offset = entry_num * DIR_BLK_ENTRY_LEN;
	usize entry_addr = tbl_base_addr + entry_offset;
	_seek_to_data_addr(dir_blk_num, entry_addr);
}

usize _get_entry_num(usize dir_blk_num, char *entry_name)
{
	usize entry_num = 0;
	char curr_entry[MAX_FILENAME_LEN];
	do {
		_seek_to_dir_entry_num(dir_blk_num, entry_num);
		bool in_use = _read_bool();
		if (in_use) {
			_read_str(MAX_FILENAME_LEN, curr_entry);
		}
		entry_num += 1;
		// TODO: need to give up at some point
		//       either end of dir indicator or num_entries
	} while (strcmp(entry_name, curr_entry) != 0);
	return entry_num - 1;
}

/*
 * Linear searches through dir blk for a matching name, then returns the inode
 * num associated with it
 */
usize _get_inode_num_of_name(usize dir_blk_num, char *name)
{
	_seek_to_dir_entry_num(dir_blk_num, _get_entry_num(dir_blk_num, name));

	_read_bool(); // skip forward one byte
	char entry_name[MAX_FILENAME_LEN];
	_read_str(MAX_FILENAME_LEN, entry_name);
	assert(strcmp(name, entry_name) == 0);
	u32 entry_num = _read_u32();
	return entry_num;
}

/*
 * Recursively traverses until reaching the parent directory of the end file in
 * @path, the returns the of the data block representing said directory.
 */
usize _get_parent_dir_blk_num(char *path)
{
	usize path_len = strlen(path);

	char path_cpy[path_len];
	strcpy(path_cpy, path);

	usize dir_blk_num = ROOT_DIR_BLK_NUM;
	// TODO: unix would allow escaping of '/' with '\/'
	char *sub_dir = strtok(path_cpy, PATH_DELIM);
	char *next;
	// read up until the last token (i.e. the parent dir)
	while ((next = strtok(NULL, PATH_DELIM)) != NULL) {
		// search for sub_dir in dir_blk
		u32 inode_num = _get_inode_num_of_name(dir_blk_num, sub_dir);
		u32 inode = _get_inode(inode_num);
		dir_blk_num = _inode_data_ptr(inode);
		sub_dir = next;
	}
	return dir_blk_num;
}

u32 _get_inode_of_path(char *path)
{
	usize parent_dir_blk_num = _get_parent_dir_blk_num(path);
	char filename[MAX_FILENAME_LEN];
	_get_end_filename(path, filename);
	usize inode_num = _get_inode_num_of_name(parent_dir_blk_num, filename);
	u32 inode = _get_inode(inode_num);
	return inode;
}

void _add_dir_entry(usize dir_blk_num, char *entry_name, u32 entry_num)
{
	_seek_to_data_addr(dir_blk_num, 0);
	/*usize next_blk =*/ _read_usize();

	u32 next_avl_entry = _read_u32();
	_seek_to_dir_entry_num(dir_blk_num, next_avl_entry);

	bool in_use = _read_bool();
	assert(!in_use);
	u32 next_addr = _read_u32();
	u32 next_next_avl_entry = next_addr == 0
		? next_avl_entry + 1
		: next_addr;

	_seek_to_data_addr(dir_blk_num, DIR_BLK_NEXT_AVL_ENTRY_OFFSET);
	_write_u32(next_next_avl_entry);

	_seek_to_dir_entry_num(dir_blk_num, next_avl_entry);
	_write_bool(true);
	_write_str(MAX_FILENAME_LEN, entry_name);
	_write_u32(entry_num);
}

u32 _del_dir_entry(usize dir_blk_num, char *entry_name)
{
	_seek_to_data_addr(dir_blk_num, DIR_BLK_NEXT_AVL_ENTRY_OFFSET);
	u32 next_avl_entry = _read_u32();

	usize dir_entry_num = _get_entry_num(dir_blk_num, entry_name);
	_seek_to_dir_entry_num(dir_blk_num, dir_entry_num);
	_write_bool(false);
	_write_u32(next_avl_entry);

	usize dir_entry_inode_offset = DIR_BLK_ENTRY_TBL_OFFSET
		+ (dir_entry_num * DIR_BLK_ENTRY_LEN)
		+ DIR_BLK_INODE_OFFSET;
	_seek_to_data_addr(dir_blk_num, dir_entry_inode_offset);
	u32 inode_num = _read_u32();

	_seek_to_data_addr(dir_blk_num, DIR_BLK_NEXT_AVL_ENTRY_OFFSET);
	_write_u32(dir_entry_num);

	return inode_num;
}

/*
 * Note: the parent directory must exist
 */
void _create_path(char *path, usize inode_num)
{
	assert(path[0] == '/');

	usize parent_dir_blk_num = _get_parent_dir_blk_num(path);

	char filename[MAX_FILENAME_LEN];
	_get_end_filename(path, filename);

	_add_dir_entry(parent_dir_blk_num, filename, inode_num);
}

/*
 * Note: the parent directory must exist
 * Returns the inode entry of the deleted path
 */
u32 _del_path(char *path)
{
	assert(path[0] == '/');

	usize parent_dir_blk_num = _get_parent_dir_blk_num(path);

	char filename[MAX_FILENAME_LEN];
	_get_end_filename(path, filename);

	return _del_dir_entry(parent_dir_blk_num, filename);
}

void _del_inode(usize inode_num)
{
	u32 deleted_inode = _dealloc_inode(inode_num);
	usize data_blk_num = _inode_data_ptr(deleted_inode);
	_dealloc_data_blk(data_blk_num);
}

/*
 * Creates an empty file at @path
 */
u8 fs_create(char *path, bool is_dir, u8 owner)
{
	usize inode_num = _alloc_inode();
	usize data_blk = _alloc_data_blk();
	u32 inode = _new_inode(is_dir, owner, true, true, data_blk);
	_set_inode(inode_num, inode);
	_create_path(path, inode_num);
	fflush(bk_f);
	return 0;
}

/*
 * Deletes the file at @path
 */
u8 fs_delete(char *path)
{
	usize inode_num = _del_path(path);
	_del_inode(inode_num);
	fflush(bk_f);
	return 0;
}

/*
 * Opens a file at path, returning info about the opened file
 */
struct fs_file_desc fs_open(char *path)
{
	u32 inode = _get_inode_of_path(path);
	usize data_blk_num = _inode_data_ptr(inode);
	struct fs_file_desc fd = {
		.path = path,
		.head_blk_num = data_blk_num,
		.curr_blk_num = data_blk_num,
		.curr_offset = 0,
		.is_dir = _inode_is_dir(inode),
		.owner = _inode_owner(inode),
		.has_read = _inode_has_read_perm(inode),
		.has_write = _inode_has_write_perm(inode),
	};
	return fd;
}

/*
 * Creates or retrieves the next data block
 */
usize _get_next_data_blk_num(usize curr_blk)
{
	_seek_to_data_addr(curr_blk, 0);
	usize next_blk_num = _read_usize();
	if (next_blk_num == 0) {
		next_blk_num = _alloc_data_blk();
		_seek_to_data_addr(curr_blk, 0);
		_write_usize(next_blk_num);
	}
	return next_blk_num;
}

u8 _traversal_loop(struct fs_file_desc *fd,
	           u8 *buf,
	           usize len,
	           u8 func(u8 *, usize))
{
	usize bytes_remaining = len;
	while (bytes_remaining > 0) {
		usize post_write_offset = fd->curr_offset + bytes_remaining;
		bool overflow = post_write_offset > layout.data_blk_usable_len;

		usize cpy_len = overflow
			? layout.data_blk_usable_len - fd->curr_offset
			: bytes_remaining;

		_seek_to_data_usable_addr(fd->curr_blk_num, fd->curr_offset);
		func(buf, cpy_len);
		bytes_remaining -= cpy_len;

		fd->curr_offset += cpy_len;
		if (fd->curr_offset >= layout.data_blk_usable_len) {
			fd->curr_blk_num =
				_get_next_data_blk_num(fd->curr_blk_num);
			fd->curr_offset = 0;
		}
	}
	return 0;
}

u8 _write_func(u8 *buf, usize len)
{
	usize num_written = fwrite(buf, sizeof(buf[0]), len, bk_f);
	return num_written != len;
}

u8 _read_func(u8 *buf, usize len)
{
	usize num_read = fread(buf, sizeof(buf[0]), len, bk_f);
	return num_read != len;
}

u8 _no_op(u8 *buf, usize len)
{
	return 0;
}

/*
 * Writes @len bytes from @buf to the file at the current seek position
 */
u8 fs_write(struct fs_file_desc *fd, u8 *buf, usize len)
{
	_traversal_loop(fd, buf, len, *_write_func);
	fflush(bk_f);
	return 0;
}

/*
 * Reads @len bytes into @buf from the file at the current seek position
 */
u8 fs_read(struct fs_file_desc *fd, u8 *buf, usize len)
{
	_traversal_loop(fd, buf, len, *_read_func);
	return 0;
}

u8 fs_seek(struct fs_file_desc *fd, usize offset)
{
	fd->curr_blk_num = fd->head_blk_num;
	fd->curr_offset = 0;
	_traversal_loop(fd, NULL, offset, *_no_op);
	return 0;
}
