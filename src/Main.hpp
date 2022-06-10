// Derived from the wxWidgets "Hello World" program.

#include "Chip8.hpp"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif


/**
 * @brief App class for the UI of the emulator.
 */
class Chip8CPP : public wxApp {
public:
	virtual bool OnInit();
};


/**
 * @brief Frame class for the primary window UI of the emulator.
 */
class MainFrame : public wxFrame, public Chip8Keyboard,
	public Chip8Display, public Chip8Sound, public Chip8Message {
public:
	MainFrame();
	~MainFrame();

private:
	Chip8* 		_vm;
	uint8_t*	_screenBuf;

	wxMenuBar*  _menuBar;
	wxMenu*     _menu_file;
	wxMenu*		_menu_emu;
	wxMenu*     _menu_help;
	wxImage*	_screen;

	bool test_key(uint8_t key) override;
	uint8_t wait_key() override;
	void draw(uint64_t* screen) override;
	void start_sound() override;
	void stop_sound() override;
	void crashed(const char* what) override;

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnOpen(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnSave(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnLoad(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnRun(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnStop(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnSetFreq(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnExit(wxCommandEvent& event);

	/**
	 * @brief 
	 * 
 	 * @param event 
	 */
	void OnAbout(wxCommandEvent& event);
};


/**
 * @brief Enumeration of all events required in the UI.
 */
enum {
	FILE_OPEN = 0,
	FILE_SAVE,
	FILE_LOAD,
	EMU_RUN,
	EMU_STOP,
	EMU_SET_FREQ,
};