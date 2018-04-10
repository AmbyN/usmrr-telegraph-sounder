// settings.hpp -- define the applicaiton's saved settings structure
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
#ifndef INCLUDED_SETTINGS_HPP
#define INCLUDED_SETTINGS_HPP

#include <stdint.h>

// Our saved settings structure
struct EepromSettings {
    uint8_t m_magic; // 0x23 if we programmed them
    uint8_t m_station;  // Our station number
    uint16_t m_crc16;
  public:
    EepromSettings() {}
    ~EepromSettings() {}

    bool read();
    void write();
};

#endif
