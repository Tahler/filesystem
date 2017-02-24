#ifndef _TBERRY_FILE_UTILS_H
#define _TBERRY_FILE_UTILS_H

#include <stdio.h>

/*
 * Returns the size of the file in
 */
long fsizeof(FILE *f)
{
	long curr = ftell(f);
	fseek(f, 0, SEEK_END);
	long end = ftell(f);
	fseek(f, curr, SEEK_SET);
	return end;
}

#endif /* _TBERRY_FILE_UTILS_H */
