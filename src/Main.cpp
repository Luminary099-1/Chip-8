// Derived from the wxWidgets "Hello World" program.

#include "Main.hpp"

#include <wx/numdlg.h>
#include <fstream>


// 
wxIMPLEMENT_APP(Chip8CPP);


bool Chip8CPP::OnInit() {
	MainFrame *frame = new MainFrame();
	frame->Show(true);
	return true;
}


MainFrame::MainFrame() : wxFrame(NULL, wxID_ANY, "Chip-8 C++ Emulator") {
	SetSize(1280, 720);
	Center();
	_vm = new Chip8(*this, *this, *this, *this);
	_screenBuf = (uint8_t*) malloc(64 * 32 * 3);

	_menu_file = new wxMenu;
	_menu_file->Append(FILE_OPEN, "&Open\tCtrl-O", "Open a Chip-8 program");
	_menu_file->Append(FILE_SAVE, "&Save\tCtrl-S", "Save an emulator state");
	_menu_file->Append(FILE_LOAD, "&Load\tCtrl-L", "Load an emulator state");
	_menu_file->AppendSeparator();
	_menu_file->Append(wxID_EXIT);

	_menu_emu = new wxMenu;
	_menu_emu->Append(EMU_RUN, "&Run\tCtrl-R", "Run the emulator");
	_menu_emu->Append(EMU_STOP, "&Stop\tCtrl-T", "Stop the emulator");
	_menu_emu->Append(EMU_SET_FREQ, "&Set Frequency\t"
		"Ctrl-F", "Set the instruction frequency of the emulator");

	_menu_help = new wxMenu;
	_menu_help->Append(wxID_ABOUT);

	_menuBar = new wxMenuBar;
	_menuBar->Append(_menu_file, "&File");
	_menuBar->Append(_menu_emu, "&Emulation");
	_menuBar->Append(_menu_help, "&Help");

	SetMenuBar(_menuBar);
	CreateStatusBar();

	_screen = new wxImage(64, 32, false);
	this->

	Bind(wxEVT_MENU, &MainFrame::OnOpen, this, FILE_OPEN);
	Bind(wxEVT_MENU, &MainFrame::OnSave, this, FILE_SAVE);
	Bind(wxEVT_MENU, &MainFrame::OnLoad, this, FILE_LOAD);
	Bind(wxEVT_MENU, &MainFrame::OnLoad, this, FILE_LOAD);
	Bind(wxEVT_MENU, &MainFrame::OnSetFreq, this, EMU_SET_FREQ);
	Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
	// Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnExit, this);
}


MainFrame::~MainFrame() {
	delete _vm;
	free(_screenBuf);
}


bool MainFrame::test_key(uint8_t key) {
	// TODO: Complete this.
	return false;
}


uint8_t MainFrame::wait_key() {
	// TODO: Complete this.
	return 0;
}


void MainFrame::draw(uint64_t* screen) {

}


void MainFrame::start_sound() {

}


void MainFrame::stop_sound() {

}


void MainFrame::crashed(const char* what) {
	SetStatusText("");
	wxMessageBox(what, "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
}


void MainFrame::OnOpen(wxCommandEvent& event) {
	// Construct a dialog to select the file path to open.
	wxFileDialog openDialog(this, "Load Chip-8 Program", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);
	// Return if the user doesn't select a file.
	if (openDialog.ShowModal() == wxID_CANCEL) return;
	// Grab the selected file path.
	std::string path = openDialog.GetPath();
	// Open the file and pass it to the VM.
	std::ifstream program_file;
	program_file.open(path);
	try {
		_vm->load_program(program_file);
	} catch (std::exception& e) {
		wxMessageBox(e.what(), "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
	}
}


void MainFrame::OnSave(wxCommandEvent& event) {
	// Pause the VM is it is running.
	bool running = _vm->is_running();
	if (running) _vm->stop();
	// Grab the state.
	uint8_t state[Chip8::_state_size];
	_vm->get_state(state);
	// Construct a dialog to select the file path to open.
	wxFileDialog saveDalog(this, "Save Chip-8 State", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);
	// Do nothing if the user doesn't select a file.
	if (saveDalog.ShowModal() != wxID_CANCEL) {
		// Grab the selected file path.
		std::string path = saveDalog.GetPath();
		std::ofstream state_file;
		state_file.open(path, std::ofstream::out);
		// Write the state to the file and close it.
		state_file << state;
		state_file.close();
	}
	// Resume the VM if it was running.
	if (running) _vm->start();
}


void MainFrame::OnLoad(wxCommandEvent& event) {
	// Pause the VM is it is running.
	if (_vm->is_running()) _vm->stop();
	// Scratch space for the state.
	uint8_t state[Chip8::_state_size];
	// Construct a dialog to select the file path to open.
	wxFileDialog saveDalog(this, "Open Chip-8 State", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);
	// Do nothing if the user doesn't select a file.
	if (saveDalog.ShowModal() != wxID_CANCEL) {
		// Grab the selected file path.
		std::string path = saveDalog.GetPath();
		std::ifstream state_file;
		state_file.open(path, std::ifstream::in);
		// Read the state from the file and close it.
		state_file.read((char*) state, Chip8::_state_size);
		state_file.close();
		// Pass the state to the VM.
		_vm->set_state(state);
	}
}


void MainFrame::OnRun(wxCommandEvent& event) {
	try {
		_vm->start();
	} catch (std::logic_error& e) {
		wxMessageBox(e.what(), "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
		return;
	}
	std::string msg = "VM Running @" + _vm->_freq;
	msg += "Hz";
	SetStatusText(msg);
}


void MainFrame::OnStop(wxCommandEvent& event) {
	_vm->stop();
}


void MainFrame::OnSetFreq(wxCommandEvent& event) {
	// Pause the VM is it is running.
	bool running = _vm->is_running();
	if (running) _vm->stop();
	// Construct a dialog to select the desired frequency,
	wxNumberEntryDialog freqDialog(this, "Set Emulation Frequency", "", "",
		500, 1, 10000);
	// Return if the user doesn't select a file.
	if (freqDialog.ShowModal() == wxID_CANCEL) return;
	// Set the frequency.
	_vm->_freq = (uint16_t) freqDialog.GetValue();
	// Resume the VM if it was running.
	if (running) _vm->start();
}


void MainFrame::OnExit(wxCommandEvent& event) {
	// TODO: Sort out the runtime error from calling this.
	// Close(true);
}


void MainFrame::OnAbout(wxCommandEvent& event) {
	wxMessageBox("This program is a virtual machine for the original Chip-8 "
		"language that most commonly ran on the RCA COSMAC VIP.",
		"About this Chip-8 Emulator", wxOK | wxICON_INFORMATION | wxCENTER);
}