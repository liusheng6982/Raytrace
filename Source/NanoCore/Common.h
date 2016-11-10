#ifndef __INC_NANOCORE_COMMON
#define __INC_NANOCORE_COMMON

#ifndef NULL
	#define NULL 0
#endif

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
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
	#define assert(x) //do{if(!(x))__asm{int 3}}while(0)
#else
	#define assert(x)
#endif

namespace NanoCore {
	class XmlNode;
}

template< typename X > X Min( X a, X b ) { return (a<b) ? a : b; }
template< typename X > X Max( X a, X b ) { return (a>b) ? a : b; }
template< typename X > X Clamp( X a, X min, X max ) { return (a<min) ? min : ( (a>max) ? max : a ); }

template< typename T > class RefCountPtr {
	T * m_ptr;
	int * m_pRefCount;

public:
	RefCountPtr() : m_ptr(NULL), m_pRefCount(NULL) {}
	RefCountPtr( const RefCountPtr & other ) : m_ptr(other.m_ptr), m_pRefCount(other.m_pRefCount) {
		m_pRefCount[0]++;
	}
	RefCountPtr( T * p ) : m_ptr(p) {
		m_pRefCount = new int();
		*m_pRefCount = 1;
	}
	~RefCountPtr() {
		Release();
	}
	void operator = ( const RefCountPtr & other ) {
		Release();
		m_ptr = other.m_ptr;
		m_pRefCount = other.m_pRefCount;
		m_pRefCount[0]++;
	}
	T * operator -> () {
		return m_ptr;
	}
	operator bool () {
		return m_ptr != NULL;
	}
	bool operator ! () const {
		return m_ptr == NULL;
	}
	const T * operator -> () const {
		return m_ptr;
	}
	bool Empty() const {
		return m_ptr == NULL;
	}
	void Release() {
		if( m_pRefCount && --m_pRefCount[0] == 0 ) {
			delete m_pRefCount;
			delete m_ptr;
		}
	}
};

#endif