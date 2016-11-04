#ifndef __INC_NANOCORE_COMMON
#define __INC_NANOCORE_COMMON

#ifndef NULL
	#define NULL 0
#endif

typedef unsigned char      uint8;
typedef signed char        int8;
typedef unsigned short     uint16;
typedef signed short       int16;
typedef unsigned long      uint32;
typedef signed long        int32;
typedef unsigned long long uint64;
typedef signed long long   int64;

#ifndef NDEBUG
	#define assert(x) do{if(!(x))__asm{int 3}}while(0)
#else
	#define assert(x)
#endif

#endif