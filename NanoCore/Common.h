#ifndef __INC_NANOCORE_COMMON
#define __INC_NANOCORE_COMMON

typedef unsigned char      uint8;
typedef signed char        int8;
typedef unsigned short     uint16;
typedef signed short       int16;
typedef unsigned long      uint32;
typedef signed long        int32;
typedef unsigned long long uint64;
typedef signed long long   int64;

struct ncFloat2 {
	float x,y;
};

struct ncFloat3 {
	float x,y,z;
};

struct ncFloat4 {
	float x,y,z,w;
};


#endif