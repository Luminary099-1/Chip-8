#include "Main.hpp"
#include <wx/numdlg.h>
#include <fstream>
#include <sstream>


// Maps wxWidget key input characters to numerical values for the Chip-8 VM.
const std::map<wxChar, uint8_t> key_map = {
	{'0', 0x00},
	{'1', 0x01},
	{'2', 0x02},
	{'3', 0x03},
	{'4', 0x04},
	{'5', 0x05},
	{'6', 0x06},
	{'7', 0x07},
	{'8', 0x08},
	{'9', 0x09},
	{'a', 0x0a},
	{'b', 0x0b},
	{'c', 0x0c},
	{'d', 0x0d},
	{'e', 0x0e},
	{'f', 0x0f}
};


bool Chip8CPP::OnInit() {
	_frame = new MainFrame();
	_frame->Show(true);
	return true; // The app should continue running after returning.
}


Chip8ScreenPanel::Chip8ScreenPanel(wxFrame* parent) : wxPanel(parent) {
	SetSize(2, 1); // Set the size to enforce the aspect ratio.
	// Initialize the data structures for the screen image.
	_image_buf = (uint8_t*) calloc(64 * 32, 3);
	_image = new wxImage(64, 32, _image_buf, true);
	// Bind the paint and resize events.
	Bind(wxEVT_PAINT, &Chip8ScreenPanel::paint_event, this);
	Bind(wxEVT_SIZE, &Chip8ScreenPanel::on_size, this);
}


Chip8ScreenPanel::~Chip8ScreenPanel() {
	delete _image;
}


void Chip8ScreenPanel::paint_event(wxPaintEvent& e) {
	// On a call to draw the panel, call the render.
	wxPaintDC dc(this);
    render(dc);
}


void Chip8ScreenPanel::on_size(wxSizeEvent& event) {
	Refresh(false); // Causes EVT_PAINT to be fired.
}


void Chip8ScreenPanel::draw() {
	uint64_t* screen = _vm->get_screen_buf();
	size_t offset {0}; // The image buffer offset.

	// Iterate over the VM's screen.
	for (int y {0}; y < 32; y ++) {
		// Initialize a mask to test pixels in the screen.
		uint64_t mask {1ULL << 63};
		for (int x {0}; x < 64; x ++) {
			// Render set pixels as white; black otherwise.
			uint8_t value {0x00};
			if (mask & screen[y]) value = 0xff;

			// Set the values in the buffer.
			_image_buf[offset++] = value;
			_image_buf[offset++] = value;
			_image_buf[offset++] = value;

			// Shuffle the bitmask along one bit.
			mask = mask >> 1;
		}
	}

	Refresh(false); // Causes EVT_PAINT to be fired.
}


void Chip8ScreenPanel::render(wxDC& dc) {
	// Grab the size of the panel.
	int w;
	int h;
	dc.GetSize(&w, &h);
	// Obtain a bitmap of the image object with the same size; render it.
	_resized = wxBitmap(_image->Scale(w, h /*, wxIMAGE_QUALITY_HIGH*/));
	dc.DrawBitmap(_resized, 0, 0, false);
}


MainFrame::MainFrame() : wxFrame(NULL, wxID_ANY, "Chip-8 C++ Emulator") {
	// Set up the "File" menu dropdown.
	wxMenu* menu_file = new wxMenu;
	menu_file->Append(ID_FILE_OPEN, "&Open\tCtrl-O", "Open a Chip-8 program");
	menu_file->Append(ID_FILE_SAVE, "&Save\tCtrl-S", "Save an emulator state");
	menu_file->Append(ID_FILE_LOAD, "&Load\tCtrl-L", "Load an emulator state");
	menu_file->AppendSeparator();
	menu_file->Append(wxID_EXIT);
	// Set up the "Emulation" menu dropdown.
	wxMenu* menu_emu = new wxMenu;
	menu_emu->Append(ID_EMU_RUN, "&Run\tCtrl-R", "Run the emulator");
	menu_emu->Append(ID_EMU_STOP, "&Stop\tCtrl-T", "Stop the emulator");
	menu_emu->Append(ID_EMU_SET_FREQ, "&Set Frequency\t"
		"Ctrl-F", "Set the instruction frequency of the emulator");
	// Set up the "Help" menu dropdown.
	wxMenu* menu_help = new wxMenu;
	menu_help->Append(wxID_ABOUT);
	// Add all menu dropdowns to the menu and add the menu and status bars.
	wxMenuBar* menuBar = new wxMenuBar;
	menuBar->Append(menu_file, "&File");
	menuBar->Append(menu_emu, "&Emulation");
	menuBar->Append(menu_help, "&Help");
	SetMenuBar(menuBar);
	CreateStatusBar();
	SetStatusText("No program loaded, idle");
	// Set up the sound display for the VM.
	_sound = new wxSound("500.wav", false);
	_screen = new Chip8ScreenPanel(this);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(_screen, 1, wxSHAPED | wxALIGN_CENTER);
	SetSizer(sizer);
	//Bind events for this window.
	Bind(wxEVT_KEY_DOWN, &MainFrame::on_key_down, this);
	Bind(wxEVT_KEY_UP, &MainFrame::on_key_up, this);
	Bind(wxEVT_MENU, &MainFrame::on_open, this, ID_FILE_OPEN);
	Bind(wxEVT_MENU, &MainFrame::on_save, this, ID_FILE_SAVE);
	Bind(wxEVT_MENU, &MainFrame::on_load, this, ID_FILE_LOAD);
	Bind(wxEVT_MENU, &MainFrame::on_run, this, ID_EMU_RUN);
	Bind(wxEVT_MENU, &MainFrame::on_stop, this, ID_EMU_STOP);
	Bind(wxEVT_MENU, &MainFrame::on_set_freq, this, ID_EMU_SET_FREQ);
	Bind(wxEVT_MENU, &MainFrame::on_about, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::on_exit, this, wxID_EXIT);
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::on_close, this, wxID_ANY);
	// Initialize the keyboard key states.
	for (int i = 0; i < 16; i ++) _key_states[i] = false;

	// Configure the window size and position and create the VM.
	SetSize(1280, 720);
	Center();
	_vm = new Chip8(this, _screen, this, this);
	_run_lock.lock();
	_runner = std::thread(&MainFrame::run_vm, this);
	_running = false;
}


bool MainFrame::test_key(uint8_t key) {
	return _key_states.at(key);
}


void MainFrame::start_sound() {
	_sound->Play(wxSOUND_ASYNC | wxSOUND_LOOP);
}


void MainFrame::stop_sound() {
	_sound->Stop();
}


void MainFrame::crashed(const char* what) {
	// Clear the status message and show an error dialog.
	SetStatusText("");
	wxMessageBox(what, "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
}


void MainFrame::on_key_up(wxKeyEvent& event) {
	// Unset the key in the map if it is valid.
	try {
		_key_states[key_map.at(event.GetUnicodeKey())] = false;
	} catch (std::out_of_range& e) {
		return;
	}
}


void MainFrame::on_key_down(wxKeyEvent& event) {
	// Set the key in the map if it is valid.
	try {
		uint8_t key = key_map.at(event.GetUnicodeKey());
		_vm->key_pressed(key);
		_key_states[key] = true;
	} catch (std::out_of_range& e) {
		return;
	}
}


void MainFrame::on_open(wxCommandEvent& event) {
	// Construct a dialog to select the file path to open.
	wxFileDialog openDialog(this, "Load Chip-8 Program", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);
	// Return if the user doesn't select a file.
	if (openDialog.ShowModal() == wxID_CANCEL) return;
	// Grab the selected file path.
	std::string path = openDialog.GetPath();
	// Open the file and read it into a string.
	std::ifstream program_file(path, std::fstream::binary);
	std::stringstream sstr;
	sstr << program_file.rdbuf();
	// Pass the string of the program to the VM.
	try {
		_vm->load_program(sstr.str());
		SetStatusText("Idle");
	} catch (std::exception& e) {
		wxMessageBox(e.what(), "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
	}
}


// FIXME: Save/load not working.
void MainFrame::on_save(wxCommandEvent& event) {
	Chip8SaveState state = _vm->get_state();

	// Construct a dialog to select the file path to open.
	wxFileDialog saveDalog(this, "Save Chip-8 State", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);

	// Do nothing if the user doesn't select a file.
	if (saveDalog.ShowModal() != wxID_CANCEL) {
		// Grab the selected file path and open the file.
		std::string path = saveDalog.GetPath();
		std::ofstream state_file;
		state_file.open(path, std::ofstream::out);
		// Write the state to the file and close it.
		state_file .write((char*) &state, sizeof(state));
		state_file.close();
	}
}


// FIXME: Save/load not working.
void MainFrame::on_load(wxCommandEvent& event) {
	stop_vm();

	// Construct a dialog to select the file path to open.
	wxFileDialog saveDalog(this, "Open Chip-8 State", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);

	// Do nothing if the user doesn't select a file.
	if (saveDalog.ShowModal() != wxID_CANCEL) {
		// Grab the selected file path.
		std::string path = saveDalog.GetPath();
		// Read the state from the file and close it.
		std::ifstream state_file(path, std::fstream::binary);
		Chip8SaveState state {};
		state_file.read((char*) &state, sizeof(state));
		state_file.close();
		// Pass the state to the VM.
		_vm->set_state(state);
	}
}


void MainFrame::on_run(wxCommandEvent& event) {
	if (!_vm->is_programmed()) {
		wxMessageBox("Unable to start the VM without loading a program.",
			"Error", wxOK | wxICON_ERROR | wxCENTRE, this);
		return;
	}

	std::string msg = "VM Running @" + std::to_string(_vm->frequency()) + "Hz.";
	SetStatusText(msg);
	start_vm();
}


void MainFrame::on_stop(wxCommandEvent& event) {
	stop_vm();
	SetStatusText("Idle.");
}


void MainFrame::on_set_freq(wxCommandEvent& event) {
	// Construct a dialog to select the desired frequency,
	wxNumberEntryDialog freqDialog(
		this, "Set Emulation Frequency", "", "", _vm->frequency(), 1, 10000);

	// If the user accepts, set the frequency.
	if (freqDialog.ShowModal() != wxID_CANCEL)
		_vm->frequency((uint16_t) freqDialog.GetValue());
}


void MainFrame::on_exit(wxCommandEvent& event) {
	close();
}


void MainFrame::on_close(wxCloseEvent& event) {
	close();
}


void MainFrame::close() {
	_die = true;
	start_vm();
	_runner.join();
	delete _vm;
	this->Destroy();
}


void MainFrame::on_about(wxCommandEvent& event) {
	// Display a message box.
	// TODO: Finalize the content in this box.
	wxMessageBox("This program is a virtual machine for the original Chip-8 "
		"language that most commonly ran on the RCA COSMAC VIP.",
		"About this Chip-8 Emulator", wxOK | wxICON_INFORMATION | wxCENTER);
}


void MainFrame::run_vm(MainFrame* frame) {
	static constexpr Chip8::_TimeType cycle_period {1000U / 60U};
	while (true) {
		frame->_run_lock.lock();
		if (frame->_die) {
			frame->_run_lock.unlock();
			break;
		}
		frame->_vm->run(cycle_period);
		frame->_run_lock.unlock();
		std::this_thread::sleep_for(cycle_period);
	}
}


void MainFrame::start_vm() {
	if (!_running) _run_lock.unlock();
	_running = true;
}


void MainFrame::stop_vm() {
	if (_running) _run_lock.lock();
	_running = false;
}
