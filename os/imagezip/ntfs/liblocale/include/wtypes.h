
#ifndef _WTYPES_H_
#define _WTYPES_H_

typedef int             __ct_rune_t;
typedef __ct_rune_t     __rune_t;
typedef __ct_rune_t     __wchar_t;
typedef __ct_rune_t     __wint_t;

#define      INTMAX_MIN       (-0x7fffffffffffffffLL-1)
#define      INTMAX_MAX       0x7fffffffffffffffLL
#define      UINTMAX_MAX      0xffffffffffffffffULL


typedef __int64_t       intmax_t;
typedef __uint64_t      uintmax_t;

#endif /* _WTYPES_H_ */

