#include "String.h"
#include <stdarg.h>



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

		std::string s( str+i, j-i );
		result.push_back( s );
		i = j;
	}
}

}