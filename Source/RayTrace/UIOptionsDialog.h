#ifndef ___INC_RAYTRACE_UI_OPTIONS_DIALOG
#define ___INC_RAYTRACE_UI_OPTIONS_DIALOG

#include <NanoCore/Windows.h>



class OptionsDialog : public NanoCore::WindowInputDialog {
public:
	MainWnd & wnd;
	OptionsDialog( Environment & env, int & previewResolution, int & numThreads ) {
		m_Items.push_back( KeyValuePtr( "Preview resolution", previewResolution ));
		m_Items.push_back( KeyValuePtr( "Raytrace threads", numThreads ));
		m_Items.push_back( KeyValuePtr( "GI bounces", env.GIBounces ));
		m_Items.push_back( KeyValuePtr( "GI samples", env.GISamples ));
		m_Items.push_back( KeyValuePtr( "Sun samples", env.SunSamples ));
		m_Items.push_back( KeyValuePtr( "Sun disk angle", env.SunDiskAngle ));
		m_Items.push_back( KeyValuePtr( "Sun angle 1", env.SunAngle1 ));
		m_Items.push_back( KeyValuePtr( "Sun angle 2", env.SunAngle2 ));
		m_Items.push_back( KeyValuePtr( "Sun strength", env.SunStrength ));
		m_Items.push_back( KeyValuePtr( "Sky strength", env.SkyStrength ));
	}
protected:
	virtual void OnOK() {
		//wnd.m_Image.Init( wnd.m_PreviewResolution, wnd.m_PreviewResolution * wnd.GetHeight() / wnd.GetWidth(), 24 );
		//wnd.PersistOptions( false );
	}
};


#endif