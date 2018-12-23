// DebugSerial -- Arduino class to enable/disable debug output to the Arduino serial port
//   Copyright (c) 2017, Stephen Paul Williams <spwilliams@gmail.com>
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

#ifndef INCLUDED_debug_serial_h
#define INCLUDED_debug_serial_h
#include <Arduino.h>

// This header file allows enabling or disabling all of the Serial.print() and Serial.println() calls
// in the station_buzzers sketch by whichever of the following two #define/#undef lines is *last*.
// Note, on the Arduino pro-micro, enabling WANT_REAL_SERIAL will cause stations 5 and 6 of the
// train selector to misbehave.
#define WANT_REAL_SERIAL
#undef WANT_REAL_SERIAL

// The 32u4 based Leonardo boards have "Serial" defined in a way that communicates directly over USB
// and leaves the D0/D1 pins completely available.
#ifdef ARDUINO_AVR_LEONARDO
#define WANT_REAL_SERIAL
#endif

#ifdef WANT_REAL_SERIAL

#define DebugSerial_begin(baud_rate) Serial.begin(baud_rate)
#define DebugSerial_print(...) Serial.print(__VA_ARGS__)
#define DebugSerial_println(...) Serial.println(__VA_ARGS__)

#else

#define DebugSerial_begin(baud_rate) do { } while(0)
#define DebugSerial_print(...) do { } while (0)
#define DebugSerial_println(...) do { } while (0)

#endif

#endif
