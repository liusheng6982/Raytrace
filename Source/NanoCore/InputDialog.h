#ifndef __INC_NANOCORE_INPUTDIALOG
#define __INC_NANOCORE_INPUTDIALOG

#include <vector>
#include <string>

namespace NanoCore {

class InputDialog
{
public:
	void Add( const wchar_t * pName, std::string & str );
	void Add( const wchar_t * name, float & f );
	void Add( const wchar_t * name, int & i );
	void Add( const wchar_t * pName, std::vector<std::wstring> & combo, int & item );

	void Run( const wchar_t * pCaption );

private:
	struct Impl;
	Impl * m_pImpl;
};

}
#endif