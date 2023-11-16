#include "Main.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <wx/colordlg.h>
#include <wx/numdlg.h>


// Maps wxWidget key input characters to numerical values for the Chip-8 VM.
const std::map<wxChar, uint8_t> key_map = {
	{'x', 0x0},		{'1', 0x1},
	{'2', 0x2},		{'3', 0x3},
	{'Q', 0x4},		{'W', 0x5},
	{'E', 0x6},		{'A', 0x7},
	{'S', 0x8},		{'D', 0x9},
	{'Z', 0xa},		{'C', 0xb},
	{'4', 0xc},		{'R', 0xd},
	{'F', 0xe},		{'V', 0xf},
};


bool Chip8CPP::OnInit() {
	// _error_file.open(".\\errorlog.txt");
	_old_error_buf = std::cerr.rdbuf();
	// std::cerr.rdbuf(_error_file.rdbuf());
	_frame = new MainFrame();
	_frame->Show(true);
	return true; // The app should continue running after returning.
}


int Chip8CPP::OnExit() {
	std::cerr.rdbuf(_old_error_buf);
	return 0;
}


Chip8ScreenPanel::Chip8ScreenPanel(wxFrame* parent) : wxPanel(parent) {
	SetSize(2, 1); // Set the size to enforce the aspect ratio.
	// Initialize the data structures for the screen image.
	_image_buf = (uint8_t*) calloc(64 * 32, 3);
	_image = new wxImage(64, 32, _image_buf, true);
	_update = false;
	// Bind the paint and resize events.
	Bind(wxEVT_PAINT, &Chip8ScreenPanel::paint_event, this);
	Bind(wxEVT_SIZE, &Chip8ScreenPanel::on_size, this);
}


Chip8ScreenPanel::~Chip8ScreenPanel() {
	delete _image;
}


void Chip8ScreenPanel::mark() {
	_update = true;
	Refresh(false); // Causes EVT_PAINT to be fired (handled by paint_event).
}


void Chip8ScreenPanel::paint_event(wxPaintEvent& e) {
	if (_update) {
		publish_buffer();
		_update = false;
	}
	// Grab the size of the panel.
	int w;
	int h;
	wxPaintDC dc(this);
	dc.GetSize(&w, &h);
	// Obtain a bitmap of the image object with the same size; render it.
	_resized = wxBitmap(_image->Scale(w, h /*, wxIMAGE_QUALITY_HIGH*/));
	dc.DrawBitmap(_resized, 0, 0, false);
}


void Chip8ScreenPanel::on_size(wxSizeEvent& event) {
	Refresh(false); // Causes EVT_PAINT to be fired (handled by paint_event).
	event.Skip();
}


void Chip8ScreenPanel::clear_buffer() {
	size_t i {0};
	while (i < sizeof(_image_buf)) {
		_image_buf[i++] = _backR;
		_image_buf[i++] = _backG;
		_image_buf[i++] = _backB;
	}
}


void Chip8ScreenPanel::publish_buffer() {
	uint64_t* screen {_vm->get_screen_buf()};
	size_t offset {0}; // The image buffer offset.

	// Iterate over the VM's screen.
	for (int y {0}; y < 32; ++y) {
		// Initialize a mask to test pixels in the screen.
		uint64_t mask {1ULL << 63};
		for (int x {0}; x < 64; ++x) {
			// Set the values in the buffer.
			if (mask & screen[y]) {
				_image_buf[offset++] = _foreR;
				_image_buf[offset++] = _foreG;
				_image_buf[offset++] = _foreB;
			} else {
				_image_buf[offset++] = _backR;
				_image_buf[offset++] = _backG;
				_image_buf[offset++] = _backB;
			}

			// Shuffle the bitmask along one bit.
			mask = mask >> 1;
		}
	}
}


MainFrame::MainFrame() : wxFrame(NULL, wxID_ANY, "Chip-8 C++ Emulator") {
	// Set up the "File" menu dropdown.
	wxMenu* menu_file = new wxMenu;
	menu_file->Append(
		ID_FILE_OPEN, "&Open ROM\tCtrl-O", "Open a Chip-8 program");
	menu_file->Append(
		ID_FILE_SAVE, "&Save State\tCtrl-S", "Save an emulator state");
	menu_file->Append(
		ID_FILE_LOAD, "&Load State\tCtrl-L", "Load an emulator state");
	menu_file->AppendSeparator();
	menu_file->Append(wxID_EXIT);
	// Set up the "Emulation" menu dropdown.
	wxMenu* menu_emu = new wxMenu;
	menu_emu->Append(ID_EMU_RUN, "&Run\tCtrl-R", "Run the emulator");
	menu_emu->Append(ID_EMU_STOP, "&Stop\tCtrl-T", "Stop the emulator");
	menu_emu->Append(ID_EMU_SET_FREQ, "&Set Frequency\t"
		"Ctrl-F", "Set the instruction frequency of the emulator");
	menu_emu->AppendSeparator();
	menu_emu->Append(ID_EMU_SET_FORE, "Set Foreground Color",
		"Set the display's forground color.");
	menu_emu->Append(ID_EMU_SET_BACK, "Set Background Color",
		"Set the display's background color.");
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
	SetStatusText("No program loaded, idle.");
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
	Bind(wxEVT_MENU, &MainFrame::on_set_color, this, ID_EMU_SET_FORE);
	Bind(wxEVT_MENU, &MainFrame::on_set_color, this, ID_EMU_SET_BACK);
	Bind(wxEVT_MENU, &MainFrame::on_about, this, wxID_ABOUT);
	Bind(wxEVT_MENU, &MainFrame::on_exit, this, wxID_EXIT);
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::on_close, this, wxID_ANY);
	// Initialize the keyboard key states.
	for (int i = 0; i < 16; ++i) _key_states[i] = false;

	// Configure the window size and position and create the VM.
	SetSize(1280, 720);
	Center();
	SetFocus();
	_vm = new Chip8(this, _screen, this, this);
	_run_lock.lock();
	_die = false;
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
		uint8_t key_val {key_map.at(event.GetUnicodeKey())};
		_key_states[key_val] = false;
		_vm->key_released(key_val);
	} catch (std::out_of_range& e) {
		return;
	}
}


void MainFrame::on_key_down(wxKeyEvent& event) {
	// Set the key in the map if it is valid.
	try {
		uint8_t key_val {key_map.at(event.GetUnicodeKey())};
		_key_states[key_val] = true;
		_vm->key_pressed(key_val);
	} catch (std::out_of_range& e) {
		return;
	}
}


// TODO: Add file extention recognition.
void MainFrame::on_open(wxCommandEvent& event) {
	stop_vm();

	// Construct a dialog to select the file path to open.
	wxFileDialog openDialog(this, "Load Chip-8 Program", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);
	// Return if the user doesn't select a file.
	if (openDialog.ShowModal() == wxID_CANCEL) return;
	// Grab the selected file path.
	std::string path {openDialog.GetPath()};
	// Open the file and read it into a string.
	std::ifstream program_file(path, std::fstream::binary);
	std::stringstream sstr;
	sstr << program_file.rdbuf();
	// Pass the string of the program to the VM.
	try {
		std::string program {sstr.str()};
		_vm->load_program(program);
		SetStatusText("Idle");
	} catch (std::exception& e) {
		wxMessageBox(e.what(), "Chip-8 Error", wxOK | wxICON_ERROR | wxCENTER);
	}
	
	_screen->clear_buffer();
	_screen->mark();
	SetFocus();
}


// TODO: Add file extention recognition.
void MainFrame::on_save(wxCommandEvent& event) {
	// Construct a dialog to select the file path to open.
	wxFileDialog saveDalog(this, "Save Chip-8 State", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Do nothing if the user doesn't select a file.
	if (saveDalog.ShowModal() != wxID_CANCEL) {
		// Grab the selected file path and open the file.
		std::string path {saveDalog.GetPath()};
		std::ofstream state_file;
		state_file.open(path, std::ofstream::out | std::ofstream::binary);
		// Write the state to the file and close it.
		state_file << *_vm;
		state_file.close();
	}
	SetFocus();
}


// TODO: Add file extention recognition.
void MainFrame::on_load(wxCommandEvent& event) {
	stop_vm();

	// Construct a dialog to select the file path to open.
	wxFileDialog saveDalog(this, "Open Chip-8 State", "", "",
		wxFileSelectorDefaultWildcardStr, wxFD_OPEN);

	// Do nothing if the user doesn't select a file.
	if (saveDalog.ShowModal() != wxID_CANCEL) {
		// Grab the selected file path.
		std::string path {saveDalog.GetPath()};
		// Read the state from the file and close it.
		std::ifstream state_file(path, std::fstream::binary);
		state_file >> *_vm;
		state_file.close();
	}

	_screen->mark();
	SetFocus();
}


void MainFrame::on_run(wxCommandEvent& event) {
	if (!_vm->is_programmed()) {
		wxMessageBox("Unable to start the VM without loading a program.",
			"Error", wxOK | wxICON_ERROR | wxCENTRE, this);
		return;
	}
	start_vm();
}


void MainFrame::on_stop(wxCommandEvent& event) {
	stop_vm();
}


void MainFrame::on_set_freq(wxCommandEvent& event) {
	// Construct a dialog to select the desired frequency,
	wxNumberEntryDialog freqDialog(
		this, "Set Emulation Frequency", "", "", _vm->frequency(), 1, 10000);

	// If the user accepts, set the frequency.
	if (freqDialog.ShowModal() != wxID_CANCEL)
		_vm->frequency((uint16_t) freqDialog.GetValue());

	if (_running) show_running_status();
	
	SetFocus();
}


void MainFrame::on_set_color(wxCommandEvent& event) {
	// Construct a dialog to select the desired color,
	wxColourDialog colorDialog(this);
	// If the user accepts, set the forground color.
	if (colorDialog.ShowModal() != wxID_CANCEL) {
		wxColour c = colorDialog.GetColourData().GetColour();
		if (event.GetId() == ID_EMU_SET_FORE) {
			_screen->_foreR = c.GetRed();
			_screen->_foreG = c.GetGreen();
			_screen->_foreB = c.GetBlue();
		} else {
			_screen->_backR = c.GetRed();
			_screen->_backG = c.GetGreen();
			_screen->_backB = c.GetBlue();
		}
		_screen->mark();
	}
	SetFocus();
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
	// TODO: Finalize the content in this box.
	wxMessageBox("This program is a virtual machine for the original Chip-8 "
		"language that most commonly ran on the RCA COSMAC VIP.",
		"About this Chip-8 Emulator", wxOK | wxICON_INFORMATION | wxCENTER);
}


void MainFrame::run_vm(MainFrame* frame) {
	using clock = std::chrono::steady_clock;
	static constexpr Chip8::_TimeType batch_period {Chip8::_billion / 60U};
	
	while (true) {
		Chip8::_TimeType start_time {clock::now().time_since_epoch()};
		frame->_run_lock.lock();
		if (frame->_die) {
			frame->_run_lock.unlock();
			break;
		}
		frame->_vm->run(batch_period);
		frame->_run_lock.unlock();
		Chip8::_TimeType delta {clock::now().time_since_epoch() - start_time};
		Chip8::_TimeType sleep_period {batch_period - delta};
		if (sleep_period.count() > 0)
			std::this_thread::sleep_for(batch_period - delta);
	}
}


void MainFrame::start_vm() {
	if (_running) return;
	_run_lock.unlock();
	_running = true;
	show_running_status();
}


void MainFrame::stop_vm() {
	if (!_running) return;
	_run_lock.lock();
	_running = false;
	SetStatusText("Idle.");
}


void MainFrame::show_running_status() {
	std::string msg {"VM Running @" + std::to_string(_vm->frequency()) + "Hz."};
	SetStatusText(msg);
}