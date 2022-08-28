#ifndef __COMPILER_H__
#define __COMPILER_H__

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#endif
