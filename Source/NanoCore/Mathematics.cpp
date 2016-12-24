#include <math.h>
#include "Mathematics.h"

float ncSqrt( float x )
{
	return sqrtf( x );
}

float ncTan( float x )
{
	return tanf( x );
}

float ncSin( float x )
{
	return sinf( x );
}

float ncCos( float x )
{
	return cosf( x );
}

void  ncSinCos( float x, float & s, float & c )
{
	s = sinf( x );
	c = cosf( x );
}

float ncFloor( float x ) {
	return floorf( x );
}

float ncFrac( float x ) {
	return x - floorf( x );
}

float ncPow( float x, float y ) {
	return powf( x, y );
}

float ncLog( float x ) {
	return logf( x );
}