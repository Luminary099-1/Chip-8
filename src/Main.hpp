#pragma once

#include "Chip8.hpp"

#include <atomic>
#include <mutex>
#include <fstream>
#include <thread>

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/sound.h>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif


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


class Chip8ScreenPanel : public wxPanel, public Chip8Display {
private:
	uint8_t* _image_buf;	// Space to store the image before passing to WX.
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
	 * @return false to prevent this panel from accepting input focus.
	 */
	bool AcceptsFocus() const override { return false; }

	/**
	 * @brief Handles the paint events for the panel by having the image
	 * rendered to the panel.
	 * 
	 * @param event The paint event being passed down.
	 */
	void paint_event(wxPaintEvent& event);

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

	/**
	 * @brief 
	 * 
	 */
	void draw() override;
};


/**
 * @brief Frame class for the primary window UI of the emulator.
 */
class MainFrame: public wxFrame, public Chip8Keyboard,
	public Chip8Sound, public Chip8Message {
public:
	/**
	 * @brief Creates a new MainFrame instance (including a Chip-8 VM).
	 */
	MainFrame();

private:
	Chip8* 				_vm;		// Chip-8 VM.
	std::thread			_runner;	// Thread to run the VM.
	std::mutex			_run_lock;	// To control the running of the VM thread.
	bool				_running;	// Indicates the the VM is running.
	std::atomic<bool>	_die;		// Indicates the thread should exit.
	Chip8ScreenPanel* 	_screen;	// Chip-8 screen.
	wxSound*			_sound;		// Emits the tone played by the Chip-8 VM.
	std::map<uint8_t, bool> _key_states; // Stores the state of each Chip-8 key.

	/**
	 * @brief 
	 * 
	 * @param key 
	 * @return 
	 */
	bool test_key(uint8_t key) override;

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
	 * @brief Handles instances where the user presses a key on the keyboard. If
	 * the key belongs to those of the Chip-8 VM, its state is regarded as
	 * "pressed" and its value is passed to the VM if it is waiting.
	 * 
	 * @param event The event produced when the user pressed a key.
	 */
	void on_key_down(wxKeyEvent& event);

	/**
	 * @brief Handles instances where the user releases a key on the keyboard.
	 * If the key belongs to those of the Chip-8 VM, its state is regarded as
	 * "unpressed".
	 * 
	 * @param event The event produced when the user releases a key.
	 */
	void on_key_up(wxKeyEvent& event);

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

	/**
	 * @brief Handles the close button on the window, closing the application.
	 * 
 	 * @param event The event produced when the user presses "X" on the window
	 * border.
	 */
	void on_close(wxCloseEvent& event);

	/**
	 * @brief Closes the application by making the runner thread exit and
	 * closing the window.
	 */
	void close();

	/**
	 * @brief Main loop to operate the VM at the specified frequency. If able to
	 * lock the _runner_lock mutex, a batch of VM cycles will be run. If a batch
	 * is run, the thread sleeps for a sixtieth of a second. Execution is paused
	 * by locking the mutex from the main thread. If _die is set, then the
	 * thread exits when able to lock the mutex.
	 */
	static void run_vm(MainFrame* frame);

	/**
	 * @brief Allows the VM to execute cycles by unlocking _runner_lock.
	 */
	void start_vm();

	/**
	 * @brief Prevents the VM to execute cycles by locking _runner_lock.
	 */
	void stop_vm();
};


/**
 * @brief App class for the UI of the emulator.
 */
class Chip8CPP : public wxApp {
	MainFrame* _frame;
	std::ofstream _error_file;
	std::streambuf* _old_error_buf;
	
public:
	/**
	 * @brief Creates the main application window + frame.
	 * 
	 * @return Returns true as the application is to continue running until
	 * closed from the UI.
	 */
	bool OnInit() override;

	/**
	 * @brief Performs cleanup operations just before the application exits.
	 */
	int OnExit() override;
};


// Tell wxWidgets to use the Chip8CPP class for the GUI app.
wxIMPLEMENT_APP(Chip8CPP);
