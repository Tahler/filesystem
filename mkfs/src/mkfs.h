#ifndef _MKFS_H
#define _MKFS_H

#include <tberry/types.h>

/*
 * Creates (or overwrites) a file at @path of size @len bytes, then formats it
 */
u8 fs_init(char *path, usize len, usize blk_size);

/*
 * Formats the file at @path to be in the ext4holdtheextra filesystem.
 *
 * Warning: Destructive. This will destroy all bytes in the file.
 */
u8 fs_format(char *path, usize blk_size);

#endif /* _MKFS_H */
