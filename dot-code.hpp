// dot-code.hpp -- plays US Military "dot code" through a buzzer attached to an Arduino pin
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

#ifndef INCLUDED_dot_code
#define INCLUDED_dot_code

#include "Arduino.h"

class DotCodeBuzzer {
  public:
    DotCodeBuzzer();
    ~DotCodeBuzzer();
    void setup(int pin, byte active_level);
    void start(const char *text);
    void cancel();
    bool done_playing();
    bool buzzing() {
      return m_state == PLAYING_DIT;
    }

  private:
    void buzzer_off();
    void buzzer_on();
    bool next_code_element();
    bool next_code_digit();

    enum {
      PLAYING_DONE,
      PLAYING_DIT,
      PLAYING_GAP,
    } m_state;
    int  m_pin;
    byte m_buzzer_on;
    byte m_buzzer_off;
    const char *m_text;
    const char *m_code;
    byte m_verbosity;
    unsigned m_num_dits;
    unsigned long m_ref_millis;
    unsigned m_gap_time;
};

#endif
