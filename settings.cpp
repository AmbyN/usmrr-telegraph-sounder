// settings.cpp -- maintain settings in the Arduino's EEPROM
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
#include "settings.hpp"
#include "Arduino.h"
#include "EEPROM.h"

constexpr uint8_t MAGIC = 0x23;

// Extract 'num' bits starting at 'start' from 'word'
static inline uint16_t extract_bits(uint16_t word, uint8_t start, uint8_t num)
{
  uint16_t mask = (1U << num) - 1;
  return (word >> start) & mask;
}

// Update CCITT CRC16 with one byte of 'data'
static inline uint16_t ccitt_crc16_update(uint16_t crc, uint8_t data)
{
  constexpr uint16_t poly = 0x1021;
  uint8_t   data_mask = 0x80;
  while (data_mask) {
    const uint16_t feedback = (crc & 0x8000) ? poly : 0;
    const uint16_t data_bit = (data & data_mask) ? 1 : 0;
    crc = 0xffff & (crc << 1);
    crc = 0xffff & (crc  |  data_bit);
    crc = 0xffff & (crc ^ feedback);
    data_mask = data_mask >> 1;
  }
  return crc;
}

// Read one byte from EEPROM, updating offset and CRC
static uint8_t read_eeprom_uint8(uint16_t *offset, uint16_t *crc)
{
  uint8_t data = EEPROM.read(*offset);
  *offset += 1;
  *crc = ccitt_crc16_update(*crc, data);
  return data;
}


// Read uint16_t value from EEPROM (big-endian byte order) and update offset and CRC
uint16_t read_eeprom_uint16(uint16_t *offset, uint16_t *crc)
{
  uint16_t data_hi = read_eeprom_uint8(offset, crc);
  uint16_t data_lo = read_eeprom_uint8(offset, crc);
  return (data_hi << 8) | data_lo;
}

// Write one byte to EEPROM, updating offset and CRC
static void write_eeprom_uint8(uint8_t value, uint16_t *offset, uint16_t *crc)
{
  EEPROM.update(*offset, value);
  *offset += 1;
  *crc = ccitt_crc16_update(*crc, value);
}

// Write uint16_t to EEPROM, updating offset and CRC
void write_eeprom_uint16(uint16_t value, uint16_t *offset, uint16_t *crc)
{
  write_eeprom_uint8(extract_bits(value, 8, 8), offset, crc);
  write_eeprom_uint8(extract_bits(value, 0, 8), offset, crc);
}

// read settings from EEPROM and verify CRC16
bool EepromSettings::read()
{
  uint16_t offset = 0;
  uint16_t crc = 0xffff;
  m_magic = read_eeprom_uint8(&offset, &crc);
  m_station = read_eeprom_uint8(&offset, &crc);
  m_crc16 = read_eeprom_uint16(&offset, &crc);
  return (m_magic == MAGIC && crc == 0);
}

// Write settings to EEPROM
void EepromSettings::write()
{
  // Mark the EEPROM settings as invalid
  EEPROM.update(0, 0);
  // Pre-seed the CRC with our magic value
  m_magic = MAGIC;
  uint16_t crc = ccitt_crc16_update(0xffff, m_magic);
  uint16_t offset = 1;
  write_eeprom_uint8(m_station, &offset, &crc);

  //finish off the crc
  crc = ccitt_crc16_update(crc, 0);
  crc = ccitt_crc16_update(crc, 0);

  m_crc16 = crc;
  // write crc
  EEPROM.update(offset + 0, extract_bits(m_crc16, 8, 8));
  EEPROM.update(offset + 1, extract_bits(m_crc16, 0, 8));
  // write magic
  EEPROM.update(0, m_magic);
}

