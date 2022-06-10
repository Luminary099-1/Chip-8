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
	/**
	 * @brief Creates the main application window + frame.
	 * 
	 * @return Returns true as the application is to continue running until
	 * closed from the UI.
	 */
	virtual bool OnInit() override;
};


/**
 * @brief Frame class for the primary window UI of the emulator.
 */
class MainFrame : public wxFrame, public Chip8Keyboard,
	public Chip8Display, public Chip8Sound, public Chip8Message {
public:
	/**
	 * @brief Creates a new MainFrame instance (including a Chip-8 VM).
	 */
	MainFrame();
	/**
	 * @brief Destroys the MainFrame instace.
	 */
	~MainFrame();

private:
	Chip8* 		_vm;			// Chip-8 VM.
	uint8_t*	_screenBuf;		// Chip-8 screen "buffer".

	wxMenuBar*  _menuBar;		// Main window menu bar.
	wxMenu*     _menu_file;		// Main window menu bar "File" tab.
	wxMenu*		_menu_emu;		// Main window menu bar "Emulation" tab.
	wxMenu*     _menu_help;		// Main window menu bar "Help" tab.
	wxImage*	_screen;

	/**
	 * @brief 
	 * 
	 * @param key 
	 * @return 
	 */
	bool test_key(uint8_t key) override;

	/**
	 * @brief 
	 * 
	 * @return uint8_t 
	 */
	uint8_t wait_key() override;

	/**
	 * @brief 
	 * 
	 * @param screen 
	 */
	void draw(uint64_t* screen) override;

	/**
	 * @brief 
	 */
	void start_sound() override;

	/**
	 * @brief 
	 */
	void stop_sound() override;

	/**
	 * @brief 
	 * 
	 * @param what 
	 */
	void crashed(const char* what) override;

	/**
	 * @brief Handles the "File->Open" button on the menu bar, opening a dialog
	 * for the user to select a program to open and execute on the VM.
	 * 
 	 * @param event The event produced when the user presses "File->Open".
	 */
	void OnOpen(wxCommandEvent& event);

	/**
	 * @brief Handles the "File->Save" button on the menu bar, pausing the VM
	 * if it was running and allowing the user to save its state to the location
	 * they specify.
	 * 
 	 * @param event The event produced when the user presses "File->Save".
	 */
	void OnSave(wxCommandEvent& event);

	/**
	 * @brief Handles the "File->Load" button on the menu bar, opening a dialog
	 * for the user to select a VM state to load and continue executing.
	 * 
 	 * @param event The event produced when the user presses "File->Load".
	 */
	void OnLoad(wxCommandEvent& event);

	/**
	 * @brief Handles the "File->Exit" button on the menu bar, closing the
	 * program.
	 * 
 	 * @param event The event produced when the user presses "File->Run".
	 */
	void OnExit(wxCommandEvent& event);

	/**
	 * @brief Handles the "Emulation->Run" button on the menu bar, starting the
	 * VM to execute cycles.
	 * 
 	 * @param event The event produced when the user presses "Emulation->Run".
	 */
	void OnRun(wxCommandEvent& event);

	/**
	 * @brief Handles the "Emulation->Stop" button on the menu bar, stopping the
	 * VM from executing cycles.
	 * 
 	 * @param event The event produced when the user presses "Emulation->Stop".
	 */
	void OnStop(wxCommandEvent& event);

	/**
	 * @brief Handles the "Emulation->Set Frequency" button on the menu bar,
	 * setting the instruction cycle frequency to the value specified by the
	 * user in a dialog.
	 * 
 	 * @param event The event produced when the user presses "Emulation->Set
	 * Frequency".
	 */
	void OnSetFreq(wxCommandEvent& event);

	/**
	 * @brief Handles the "Help->About" button on the menu bar, showing a dialog
	 * that provides information about the emulator itself.
	 * 
 	 * @param event The event produced when the user presses "Help->About".
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