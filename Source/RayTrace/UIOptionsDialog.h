#ifndef ___INC_RAYTRACE_UI_OPTIONS_DIALOG
#define ___INC_RAYTRACE_UI_OPTIONS_DIALOG

#include <NanoCore/Windows.h>



class OptionsWnd : public NanoCore::WindowInputDialog {
public:
	MainWnd & wnd;
	OptionsWnd( std::vector<OptionItem> & items ) {
		for( size_t i=0; i<wnd.m_OptionItems.size(); ++i ) {
			OptionItem & it = wnd.m_OptionItems[i];
			if( it.i )
				Add( it.name.c_str(), *it.i );
			else if( it.f )
				Add( it.name.c_str(), *it.f );
			else if( it.str )
				Add( it.name.c_str(), *it.str );
		}
	}
protected:
	virtual void OnOK() {
		wnd.m_Image.Init( wnd.m_PreviewResolution, wnd.m_PreviewResolution * wnd.GetHeight() / wnd.GetWidth(), 24 );
		wnd.PersistOptions( false );
	}
};


#endif