// Map the MRCS Morse Code Buzzer Controller v3.0 pins onto the corresponding Arduino I/O pins.
//   Copyright (c) 2018, Stephen Paul Williams <spwilliams@gmail.com>
//
// This program is free software; you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation; either version
// 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program;
// if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.
//
// Some of the header pins on the MCBC board are not connected
#ifndef INCLUDED_mrcs_mcbc_pins_hpp
#define INCLUDED_mrcs_mcbc_pins_hpp

constexpr uint8_t NC = 255;

constexpr uint8_t LOAD_1 = 13;
constexpr uint8_t LOAD_2 = 12;
constexpr uint8_t LOAD_3 = 11;
constexpr uint8_t LOAD_4 = 10;
constexpr uint8_t LOAD_5 =  9;
constexpr uint8_t LOAD_6 =  8;
constexpr uint8_t LOAD_7 =  0; // labeled RX1
constexpr uint8_t LOAD_8 =  1; // labeled TX0

constexpr uint8_t START_1 = A6;
constexpr uint8_t START_2 =  7;
constexpr uint8_t START_3 =  6;
constexpr uint8_t START_4 =  5;
constexpr uint8_t START_5 =  4;
constexpr uint8_t START_6 =  3;
constexpr uint8_t START_7 =  2;

constexpr uint8_t STOP_1 = A0;
constexpr uint8_t STOP_2 = A1;
constexpr uint8_t STOP_3 = A2;
constexpr uint8_t STOP_4 = A3;
constexpr uint8_t STOP_5 = A4;
constexpr uint8_t STOP_6 = A5;
constexpr uint8_t STOP_7 = A7;

#endif
