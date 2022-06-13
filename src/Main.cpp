// Derived from the wxWidgets "Hello World" program.

#include "Main.hpp"

#include <wx/numdlg.h>
#include <fstream>


// Tell wxWidgets to use the Chip8CPP class for the GUI app.
wxIMPLEMENT_APP(Chip8CPP);


bool Chip8CPP::OnInit() {
	// TODO: The exit error might be resolvable from here?
	MainFrame *frame = new MainFrame();
	frame->Show(true);
	return true;
}


Chip8ScreenPanel::Chip8ScreenPanel(wxFrame* parent) : wxPanel(parent) {
	// Set the size to enforce the aspect ratio.
	SetSize(2, 1);
	// Initialize the data structures for the screen image.
	_image = new wxImage(64, 32, true);
	_screen_buf = (uint8_t*) malloc(64 * 32 * 3);
	_image->SetData(_screen_buf, true);
	// Bind the paint and resize events.
	Bind(wxEVT_PAINT, &Chip8ScreenPanel::paint_event, this);
	Bind(wxEVT_SIZE, &Chip8ScreenPanel::on_size, this);
}


Chip8ScreenPanel::~Chip8ScreenPanel() {
	delete _image;
	free(_screen_buf);
}


void Chip8ScreenPanel::paint_event(wxPaintEvent& e) {
	// On a call to draw the panel, call the render.
	wxPaintDC dc(this);
    render(dc);
}


void Chip8ScreenPanel::paint_now(uint64_t* screen) {
	// Iterate over the VM's screen.
	for (int y = 0; y < 32; y ++) {
		// Initialize a mask to test pixels in the screen.
		uint64_t mask = 1ULL << 63;
		for (int x = 0; x < 64; x ++) {
			// Compute the base offset in the screen buffer for the next pixel.
			int offset = x * y * 3;
			// If the pixel's bit is set, render it white.
			if (mask & screen[y] > 0) {
				_screen_buf[offset] = 0xffff;
				_screen_buf[offset + 1] = 0xffff;
				_screen_buf[offset + 2] = 0xffff;
			// Otherwise, render it black.
			} else {
				_screen_buf[offset] = 0x0000;
				_screen_buf[offset + 1] = 0x0000;
				_screen_buf[offset + 2] = 0x0000;
			}
			// Shuffle the bitmask along one bit.
			mask = 1 >> mask;
		}
	}
	// Update the rendering to the window.
	_image->SetData(_screen_buf, true);
	wxClientDC dc(this);
    render(dc);
}


void Chip8ScreenPanel::on_size(wxSizeEvent& event) {
	// Refresh causes render.
	Refresh();
	event.Skip();
}


void Chip8ScreenPanel::render(wxDC& dc) {
	// Grab the size of the panel.
	int w, h;
	dc.GetSize(&w, &h);
	// Obtain a bitmap of the image object with the same size; render it.
	_resized = wxBitmap(_image->Scale(w, h /*, wxIMAGE_QUALITY_HIGH*/));
	dc.DrawBitmap(_resized, 0, 0, false);
}


MainFrame::MainFrame() : wxFrame(NULL, wxID_ANY, "Chip-8 C++ Emulator") {
	// Configure the window size and position and create the VM.
	SetSize(1280, 720);
	Center();
	_vm = new Chip8(*this, *this, *this, *this);
	// Set up the display for the VM.
	_screenBuf = (uint8_t*) malloc(64 * 32 * 3);
	_sizer = new wxBoxSizer(wxHORIZONTAL);
	_screen = new Chip8ScreenPanel(this);
	_sizer->Add(_screen, 1, wxSHAPED | wxALIGN_CENTER);
	SetSizer(_sizer);
	// Set up the "File" menu dropdown.
	_menu_file = new wxMenu;
	_menu_file->Append(FILE_OPEN, "&Open\tCtrl-O", "Open a Chip-8 program");
	_menu_file->Append(FILE_SAVE, "&Save\tCtrl-S", "Save an emulator state");
	_menu_file->Append(FILE_LOAD, "&Load\tCtrl-L", "Load an emulator state");
	_menu_file->AppendSeparator();
	_menu_file->Append(wxID_EXIT);
	// Set up the "Emulation" menu dropdown.
	_menu_emu = new wxMenu;
	_menu_emu->Append(EMU_RUN, "&Run\tCtrl-R", "Run the emulator");
	_menu_emu->Append(EMU_STOP, "&Stop\tCtrl-T", "Stop the emulator");
	_menu_emu->Append(EMU_SET_FREQ, "&Set Frequency\t"
		"Ctrl-F", "Set the instruction frequency of the emulator");
	// Set up the "Help" menu dropdown.
	_menu_help = new wxMenu;
	_menu_help->Append(wxID_ABOUT);
	// Add all menu dropdowns to the menu and add the menu and status bars.
	_menuBar = new wxMenuBar;
	_menuBar->Append(_menu_file, "&File");
	_menuBar->Append(_menu_emu, "&Emulation");
	_menuBar->Append(_menu_help, "&Help");
	SetMenuBar(_menuBar);
	CreateStatusBar();
	//Bind events for this window.
	// TODO: Determine if this is the right way to do this.
	Bind(wxEVT_MENU, &MainFrame::on_open, this, FILE_OPEN);
	Bind(wxEVT_MENU, &MainFrame::on_save, this, FILE_SAVE);
	Bind(wxEVT_MENU, &MainFrame::on_load, this, FILE_LOAD);
	Bind(wxEVT_MENU, &MainFrame::on_set_freq, this, EMU_SET_FREQ);
	Bind(wxEVT_MENU, &MainFrame::on_about, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::on_exit, this, wxID_EXIT);
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
	_screen->paint_now(screen);
}


void MainFrame::start_sound() {

}


void MainFrame::stop_sound() {

}


void MainFrame::crashed(const char* what) {
	// Clear the status message and show an error dialog.
	SetStatusText("");
	wxMessageBox(what, "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
}


void MainFrame::on_open(wxCommandEvent& event) {
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


void MainFrame::on_save(wxCommandEvent& event) {
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


void MainFrame::on_load(wxCommandEvent& event) {
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


void MainFrame::on_run(wxCommandEvent& event) {
	// Attempt to start the VM, show and error if something is wrong.
	try {
		_vm->start();
	} catch (std::logic_error& e) {
		wxMessageBox(e.what(), "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
		return;
	}
	// Set the status bar to indicate the VM is running.
	std::string msg = "VM Running @" + _vm->_freq;
	msg += "Hz";
	SetStatusText(msg);
}


void MainFrame::on_stop(wxCommandEvent& event) {
	_vm->stop();
}


void MainFrame::on_set_freq(wxCommandEvent& event) {
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


void MainFrame::on_exit(wxCommandEvent& event) {
	// TODO: Sort out the runtime error from calling this.
	// Close(true);
}


void MainFrame::on_about(wxCommandEvent& event) {
	// Display a message box.
	// TODO: Finalize the content in this box.
	wxMessageBox("This program is a virtual machine for the original Chip-8 "
		"language that most commonly ran on the RCA COSMAC VIP.",
		"About this Chip-8 Emulator", wxOK | wxICON_INFORMATION | wxCENTER);
}