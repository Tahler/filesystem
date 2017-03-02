#ifndef _FILE_H
#define _FILE_H

#include <tberry/types.h>

/*
 * Modifying any of the values in this structure will certainly mess up the
 * filesystem
 */
struct fs_file_desc {
	char *path;
	usize block_num;
	usize offset;
};

/*
 * The options for which to start seeking through the file
 */
enum fs_seek_opt {
	/*
	 * Seek from the start of the file
	 */
	START,

	/*
	 * Seek from the current position in the file
	 */
	CUR,

	/*
	 * Seek from the end of the file
	 */
	END,
};

/*
 * Creates an empty file at @path
 */
u8 fs_create(char *path);

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
 * Moves the seek position by @diff bytes from either the start, current
 * position, or end of the file (according to @option) by @diff
 */
u8 fs_seek(struct fs_file_desc *f, enum fs_seek_opt option, isize diff);

/*
 * Truncates the file described by @f to 0 bytes
 */
u8 fs_truncate(struct fs_file_desc *f);

/*
 * Deletes the file at @path
 */
u8 fs_delete(char *path);

#endif /* _FILE_H */
