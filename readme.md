# Chip-8 Emulator
Author: Matthew Michaud (matthew.michaud@alumni.ucalgary.ca)

## Overview
As a first project in emulation development, I've created an emulator/VM for Chip-8, originally devised for creating games on the [COSMAC VIP](https://en.wikipedia.org/wiki/COSMAC_VIP). The emulator is relatively simple, given the weak implementation constraints imposed by Chip-8. In developing the emulator, I made an effort to make it modular so it could be ported to work with different front-ends. In this implementation, I chose [wxWidgets](https://www.wxwidgets.org/) as a front-end for the GUI, display output, keyboard input, and output sound.

Presently, I believe the emulation is essentially correct beyond a small handful of things that could be improved. However, there are a few issues remaining with my front-end like periodic crashes when performing certain actions and somewhat broken sound.

Initially, I also planned to develop a complete test suite for the emulator. I elected to try [Catch2](https://github.com/catchorg/Catch2) for my testing apparatus, but I did not finish the suite and will likely start from scratch with a different framework if I decide to complete it.

This project was also my first use of [CMake](https://cmake.org/), which has proven a valuable addition to my toolbelt.

## Compilation Notes
I've been using [MSVC](https://visualstudio.microsoft.com/vs/community/) to compile the project. It's been necessary to manually disable wxWidget's accessibility option for the build to succeed.

## Works Cited
I made use of the following resources in developing my emulator:
- [The "Awesome CHIP-8" list of curated materials on the Chip-8](https://chip-8.github.io/links/). In particular, I made extensive use of:
  - [CHIP‐8 Instruction Set](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set) by Matthew Mikolay.
  - [CHIP‐8 Technical Reference
](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Technical-Reference) by Matthew Mikolay.
- [Cowgod's Chip-8 Technical Reference v1.0](http://devernay.free.fr/hacks/chip8/.C8TECH10.HTM#3.1).
- [Timendus' CHIP-8 Test Suite](https://github.com/Timendus/chip8-test-suite).
