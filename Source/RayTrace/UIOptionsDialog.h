#ifndef ___INC_RAYTRACE_UI_OPTIONS_DIALOG
#define ___INC_RAYTRACE_UI_OPTIONS_DIALOG

#include <NanoCore/Windows.h>



class OptionsWnd : public NanoCore::WindowInputDialog {
public:
	MainWnd & wnd;
	OptionsWnd( Environment & env, int & previewResolution, int & numThreads ) {
		m_Items.push_back( Item( "Preview resolution", previewResolution ));
		m_Items.push_back( Item( "Raytrace threads", numThreads ));
		m_Items.push_back( Item( "GI bounces", env.GIBounces ));
		m_Items.push_back( Item( "GI samples", env.GISamples ));
		m_Items.push_back( Item( "Sun samples", env.SunSamples ));
		m_Items.push_back( Item( "Sun disk angle", env.SunDiskAngle ));
		m_Items.push_back( Item( "Sun angle 1", env.SunAngle1 ));
		m_Items.push_back( Item( "Sun angle 2", env.SunAngle2 ));
		m_Items.push_back( Item( "Sun strength", env.SunStrength ));
		m_Items.push_back( Item( "Sky strength", env.SkyStrength ));
	}
protected:
	virtual void OnOK() {
		wnd.m_Image.Init( wnd.m_PreviewResolution, wnd.m_PreviewResolution * wnd.GetHeight() / wnd.GetWidth(), 24 );
		wnd.PersistOptions( false );
	}
};


#endif