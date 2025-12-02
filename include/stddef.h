/* Minimal stddef.h stub for ESBMC on Windows */
#ifndef _STDDEF_H
#define _STDDEF_H

/* Basic types - only define if not already defined by ESBMC */
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned long long size_t;
#endif

#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
typedef long long ptrdiff_t;
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#endif

/* NULL pointer */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Offset of member in struct */
#ifndef offsetof
#define offsetof(type, member) ((size_t)&((type *)0)->member)
#endif

#endif /* _STDDEF_H */

