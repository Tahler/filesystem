#ifndef _INODE_H
#define _INODE_H

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

#endif /* _INODE_H */
