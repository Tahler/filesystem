#ifndef _TBERRY_DEBUG_H
#define _TBERRY_DEBUG_H

#include <stdio.h>

/*
 * Prints a formatted message to `stderr` if `__DEBUG__` is true.
 * The output text is terminated by a newline and preceded by a the filename and
 * line number that `DEBUG` is called on.
 *
 * Note: `##__VA_ARGS__` is GNU specific and this may behave differently on
 * different compilers.
 */
#define DEBUG(fmt, ...)                                      \
	do {                                                 \
		if (__DEBUG__) {                             \
			fprintf(stderr, "%s:%d: " fmt "\n",  \
					__FILE__,            \
					__LINE__,            \
					##__VA_ARGS__);      \
		}                                            \
	} while (0);

/*
 * Prints the value of `val` to `stderr` using the `DEBUG` macro if `__DEBUG__`
 * is true. The type of the value must be specified through a format string.
 */
#define DEBUG_VAL(fmt, val)                     \
	do {                                    \
		DEBUG("%s: " fmt, #val, val);  \
	} while(0);

#endif /* _TBERRY_DEBUG_H */
