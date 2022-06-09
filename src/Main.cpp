// Derived from the wxWidgets "Hello World" program.

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "Chip8.hpp"


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
	bool test_key(uint8_t key) override;
	uint8_t wait_key() override;
	void draw(uint64_t* screen) override;
	void start_sound() override;
	void stop_sound() override;
	void crashed() override;

private:
	Chip8* 		vm;

	wxMenuBar*  menuBar;
	wxMenu*     menu_file;
	wxMenu*		menu_emu;
	wxMenu*     menu_help;

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

// 
wxIMPLEMENT_APP(Chip8CPP);


bool Chip8CPP::OnInit() {
	MainFrame *frame = new MainFrame();
	frame->Show(true);
	return true;
}


MainFrame::MainFrame() : wxFrame(NULL, wxID_ANY, "Chip-8 C++ Emulator") {
	vm = new Chip8(*this, *this, *this, *this);

	menu_file = new wxMenu;
	menu_file->Append(FILE_OPEN, "&Open\tCtrl-O", "Open a Chip-8 program");
	menu_file->Append(FILE_OPEN, "&Save\tCtrl-S", "Save an emulator state");
	menu_file->Append(FILE_OPEN, "&Load\tCtrl-L", "Load an emulator state");
	menu_file->AppendSeparator();
	menu_file->Append(wxID_EXIT);

	menu_emu = new wxMenu;
	menu_emu->Append(EMU_RUN, "&Run\tCtrl-R", "Run the emulator");
	menu_emu->Append(EMU_STOP, "&Stop\tCtrl-T", "Stop the emulator");

	menu_help = new wxMenu;
	menu_help->Append(wxID_ABOUT);

	menuBar = new wxMenuBar;
	menuBar->Append(menu_file, "&File");
	menuBar->Append(menu_emu, "&Emulation");
	menuBar->Append(menu_help, "&Help");

	SetMenuBar(menuBar);
	CreateStatusBar();

	Bind(wxEVT_MENU, &MainFrame::OnOpen, this, FILE_OPEN);
	Bind(wxEVT_MENU, &MainFrame::OnSave, this, FILE_SAVE);
	Bind(wxEVT_MENU, &MainFrame::OnLoad, this, FILE_LOAD);
	Bind(wxEVT_MENU, &MainFrame::OnLoad, this, FILE_LOAD);
	Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
}


MainFrame::~MainFrame() {
	delete vm;
}


bool MainFrame::test_key(uint8_t key) {
	return false;
}


uint8_t MainFrame::wait_key() {
	return 0;
}


void MainFrame::draw(uint64_t* screen) {

}


void MainFrame::start_sound() {

}


void MainFrame::stop_sound() {

}


void MainFrame::crashed() {

}


void MainFrame::OnOpen(wxCommandEvent& event) {
	// Construct a dialog to select the file path to open.
	wxFileDialog openDialog(this, "Load Chip-8 Program", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);
	// Return if the user doesn't select a file.
	if (openDialog.ShowModal() == wxID_CANCEL) return;
	// Grab the selected file path.
	std::string path = openDialog.GetPath();
	// TODO: Finish this.
}


void MainFrame::OnSave(wxCommandEvent& event) {

}


void MainFrame::OnLoad(wxCommandEvent& event) {

}


void MainFrame::OnRun(wxCommandEvent& event) {
	try {
		vm->start();
	} catch (std::logic_error& e) {
		wxMessageBox(e.what());
		return;
	}
	std::string msg = "VM Running @" + vm->_freq;
	msg += "Hz";
	SetStatusText(msg);
}


void MainFrame::OnStop(wxCommandEvent& event) {
	vm->stop();
}


void MainFrame::OnSetFreq(wxCommandEvent& event) {

}


void MainFrame::OnExit(wxCommandEvent& event) {
	Close(true);
}


void MainFrame::OnAbout(wxCommandEvent& event) {
	wxMessageBox("This is a wxWidgets Hello World example",
		"About Hello World", wxOK | wxICON_INFORMATION);
}