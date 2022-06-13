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
	bool OnInit() override;
};


// https://wiki.wxwidgets.org/An_image_panel
class Chip8ScreenPanel : public wxPanel {
private:
	uint8_t* _screen_buf;	// Space to store the image before passing to WX.
	wxImage* _image;		// The image that will contain the screen data.
	wxBitmap _resized;		// Stores the resized screen to be rendered.

public:
	/**
	 * @brief Construct a new Chip8ScreenPanel object.
	 * 
	 * @param parent The parent frame to this panel.
	 */
	Chip8ScreenPanel(wxFrame* parent);

	/**
	 * @brief Destroy the Chip8ScreenPanel.
	 */
	~Chip8ScreenPanel();

	/**
	 * @brief Handles the paint events for the panel by having the image
	 * rendered to the panel.
	 * 
	 * @param event The paint event being passed down.
	 */
	void paint_event(wxPaintEvent& event);

	/**
	 * @brief Updates the data in the member _image with the Chip-8 VM screen
	 * data that is referenced.
	 * 
	 * @param screen A pointer to an array of 32, 64-bit unsigned integers that
	 * makes up the screen of the Chip-8 VM.
	 */
	void paint_now(uint64_t* screen);

	/**
	 * @brief Handles the resize events for the panel by having the image
	 * rendered to fill the new size.
	 * 
	 * @param event The size event being passed down (skipped).
	 */
	void on_size(wxSizeEvent& event);

	/**
	 * @brief Renders the current contents of the member _image to the panel.
	 * 
	 * @param dc The device context onto which the _image is to be rendered.
	 */
	void render(wxDC& dc);
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

private:
	Chip8* 				_vm;		// Chip-8 VM.
	Chip8ScreenPanel* 	_screen;	// Chip-8 screen.

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
	void on_open(wxCommandEvent& event);

	/**
	 * @brief Handles the "File->Save" button on the menu bar, pausing the VM
	 * if it was running and allowing the user to save its state to the location
	 * they specify.
	 * 
 	 * @param event The event produced when the user presses "File->Save".
	 */
	void on_save(wxCommandEvent& event);

	/**
	 * @brief Handles the "File->Load" button on the menu bar, opening a dialog
	 * for the user to select a VM state to load and continue executing.
	 * 
 	 * @param event The event produced when the user presses "File->Load".
	 */
	void on_load(wxCommandEvent& event);

	/**
	 * @brief Handles the "File->Exit" button on the menu bar, closing the
	 * program.
	 * 
 	 * @param event The event produced when the user presses "File->Run".
	 */
	void on_exit(wxCommandEvent& event);

	/**
	 * @brief Handles the "Emulation->Run" button on the menu bar, starting the
	 * VM to execute cycles.
	 * 
 	 * @param event The event produced when the user presses "Emulation->Run".
	 */
	void on_run(wxCommandEvent& event);

	/**
	 * @brief Handles the "Emulation->Stop" button on the menu bar, stopping the
	 * VM from executing cycles.
	 * 
 	 * @param event The event produced when the user presses "Emulation->Stop".
	 */
	void on_stop(wxCommandEvent& event);

	/**
	 * @brief Handles the "Emulation->Set Frequency" button on the menu bar,
	 * setting the instruction cycle frequency to the value specified by the
	 * user in a dialog.
	 * 
 	 * @param event The event produced when the user presses "Emulation->Set
	 * Frequency".
	 */
	void on_set_freq(wxCommandEvent& event);

	/**
	 * @brief Handles the "Help->About" button on the menu bar, showing a dialog
	 * that provides information about the emulator itself.
	 * 
 	 * @param event The event produced when the user presses "Help->About".
	 */
	void on_about(wxCommandEvent& event);
};


/**
 * @brief Enumeration of all events required in the UI.
 */
enum {
	ID_FILE_OPEN = 0,
	ID_FILE_SAVE,
	ID_FILE_LOAD,
	ID_FILE_EXIT,
	ID_EMU_RUN,
	ID_EMU_STOP,
	ID_EMU_SET_FREQ,
};


// Tell wxWidgets to use the Chip8CPP class for the GUI app.
wxIMPLEMENT_APP(Chip8CPP);