#ifndef _FS_H
#define _FS_H

#include <tberry/types.h>
#include <tberry/file_utils.h>

#define BLOCK_SIZE 4096

/*
 * Creates (or overwrites) a file at @path of size @len bytes, then formats it
 */
u8 fs_init(char *path, usize len)

/*
 * Formats the file at @path to be in the ext4holdtheextra filesystem.
 *
 * Warning: Destructive. This will destroy all bytes in the file.
 */
u8 fs_format(char *path);

#endif /* _FS_H */
