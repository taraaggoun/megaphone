#ifndef REQUEST_MACROS_H
#define REQUEST_MACROS_H

#if defined(__APPLE__)
#include <limits.h>
#else
#include <linux/limits.h>
#endif

#define MAX_DATALEN         UCHAR_MAX  /* 255 */
#define FILE_PACKET_SIZE    512

#endif /* REQUEST_MACROS_H */
