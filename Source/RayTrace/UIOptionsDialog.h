#ifndef ___INC_RAYTRACE_UI_OPTIONS_DIALOG
#define ___INC_RAYTRACE_UI_OPTIONS_DIALOG

#include <NanoCore/Windows.h>
#include <NanoCore/Serialize.h>



class OptionsDialog : public NanoCore::WindowInputDialog {
public:
	OptionsDialog( Window * parent, int idCommandOk, Environment & env, int & previewResolution, int & numThreads ) {
		m_pParent = parent;
		m_idCommandOk = idCommandOk;
	}
protected:
	Window * m_pParent;
	int      m_idCommandOk;

	virtual void OnOK() {
		m_pParent->SendCommand( m_idCommandOk );
	}
};


#endif