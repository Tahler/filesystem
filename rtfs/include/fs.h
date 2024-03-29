#ifndef _FILE_H
#define _FILE_H

#include <tberry/types.h>

#define MAX_FILENAME_LEN 255

/*
 * Warning: modifying any of the values in this structure will certainly mess
 * up the filesystem
 */
struct fs_file_desc {
	char *path;

	usize head_blk_num;
	usize curr_blk_num;
	usize curr_offset;

	bool is_dir;
	u8 owner;
	bool has_read;
	bool has_write;
};

/*
 * Loads the layout of @backing_file into memory. THIS MUST BE CALLED BEFORE ANY
 * OTHER FUNCTIONS.
 *
 * @backing_file should have been made through `mkfs.ext4holdtheextra`
 */
u8 fs_load(char *backing_file);

/*
 * Creates an empty file at @path
 * Note: @path must be an absolute path (i.e. it must start with '/')
 */
u8 fs_create(char *path, bool is_dir, u8 owner);

/*
 * Opens a file at path, returning info about the opened file
 */
struct fs_file_desc fs_open(char *path);

/*
 * Closes the opened file described by @f
 */
u8 fs_close(struct fs_file_desc *f);

/*
 * Writes @len bytes from @buf to the file at the current seek position
 */
u8 fs_write(struct fs_file_desc *f, u8 *buf, usize len);

/*
 * Reads @len bytes into @buf from the file at the current seek position
 */
u8 fs_read(struct fs_file_desc *f, u8 *buf, usize len);

/*
 * Moves the position of @f to be @offset bytes into the file
 */
u8 fs_seek(struct fs_file_desc *f, usize offset);

/*
 * Deletes the file at @path
 */
u8 fs_delete(char *path);

#endif /* _FILE_H */
