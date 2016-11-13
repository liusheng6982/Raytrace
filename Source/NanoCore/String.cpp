#include "String.h"
#include <stdarg.h>
#include "Common.h"



namespace NanoCore {



static char ReadNextCharacter( const char *& match ) {
	char c = *match++;
	if( c != '\\' )
		return c;
	return *match++;
}

int StrPatternMatch( const char * buf, const char * match, ... )
{
	va_list args;
	va_start( args, match );

	int pos, j;
	for( pos = 0; *match; ) {
		char c = ReadNextCharacter( match );
		if( c == '%' ) {
			char sep = ReadNextCharacter( match );
			for( j=0; buf[pos+j] && buf[pos+j] != sep; ++j );
			std::string * dest = va_arg( args, std::string* );
			dest->assign( buf+pos, j );
			pos += j;
			c = sep;
		}
		switch( *match ) {
		case '*':
			for( j=0; buf[pos+j] == c; ++j );
			match++;
			pos += j;
			break;
		case '+':
			for( j=0; buf[pos+j] == c; ++j );
			if( !j )
				return 0;
			match++;
			pos += j;
			break;
		default:
			if( buf[pos] != c ) return 0;
			pos++;
			break;
		}
	}
	va_end( args );
	return pos;
}

std::wstring StrGetPath( std::wstring wFilename ) {
	size_t k = wFilename.find_last_of( L'/' );
	if( k == std::wstring::npos ) k = wFilename.find_last_of( L'\\' );
	if( k != std::wstring::npos ) wFilename.erase( k+1 );
	return wFilename;
}

std::wstring StrGetFilename( std::wstring wPathname ) {
	size_t k1 = wPathname.find_last_of( L'/' );
	size_t k2 = wPathname.find_last_of( L'\\' );

	if( k1 == std::wstring::npos ) k1 = 0;
	if( k2 == std::wstring::npos ) k2 = 0;

	size_t k = Max( k1, k2 );
	return std::wstring( wPathname.begin() + k, wPathname.end() );
}

void StrReplaceExtension( std::wstring & file, const wchar_t * newext ) {
	size_t k = file.find_last_of( L'.' );
	if( k != std::wstring::npos ) {
		file.erase( k+1 );
		file += newext;
	} else {
		file += L".";
		file += newext;
	}
}

std::wstring StrMbsToWcs( const char * str ) {
	std::wstring s( str, str + strlen(str) );
	return s;
}

std::string  StrWcsToMbs( const wchar_t * str ) {
	std::string s( str, str + wcslen(str) );
	return s;
}

void StrSplit( const char * str, const char * separators, std::vector<std::string> & result ) {
	bool sep[128] = { false };
	while( *separators ) {
		sep[*separators++] = true;
	}

	for( int i=0,j; str[i]; ) {
		while( sep[str[i]] ) ++i;
		for( j=i; str[j] && !sep[str[j]]; ++j );

		if( j-i > 0 ) {
			std::string s( str+i, j-i );
			result.push_back( s );
		}
		i = j;
	}
}

void StrTrim( std::string & str ) {
	while( !str.empty() && str[0] == ' ' ) str.erase( str.begin());
	while( !str.empty() && str.back() == ' ' ) str.resize( str.size()-1 );
}

void StrLwr( std::string & s ) {
	strlwr( &s[0] );
}

}