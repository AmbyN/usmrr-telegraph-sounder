// dot_code.cpp -- plays the usmrr dot code through a buzzer attached to an Arduino pin
//   Copyright (c) 2013-2017, Stephen Paul Williams <spwilliams@gmail.com>
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

#include "dot-code.hpp"
#include "Arduino.h"
#include "debug-serial.h"

static constexpr unsigned dit_time =  50; // milliseconds
static constexpr unsigned gap_time = 100;

struct DotCodeEncode {
  const char *prefix;
  const char *code;
};

static DotCodeEncode encode_table[] = {
  // This is the "normal" encode table which swallows characters from
  // the beginning of the string as we move long, replacing them with
  // the corresponding digits. This is searched linearly as we swallow
  // the string so we find the suffix abbreviations first.
  //
  // The space character (Ascii ' ') will be treated as an end-of-word (3),
  //
  // A punctuation character will play either the end-of-sentence (33) or
  // end-of-message (333) indication, depending upon whether the next character
  // is a space or the end of the string or followed by a '~'.
  //
  // The special character '~' will play 1 second of silence to allow spacing
  // out the message.
  //
  // For Bernie's railroad, the station OS messages are:
  //
  //    "WWW OS ~~ST E1.~~~~~"
  // or
  //  2212.2212.2212.3.12.121.33.delay.delay.1112.11.3.21.12221.333
  //
  // In Bernie's case we repeat the message after a 5 second delay using
  // a series of trailing '~' characters.
  {"AND", "2222"},
  {"ED", "1222"},
  {"ING", "1121"},
  {"TION", "2221"},
  {"A", "11"},
  {"B", "1221"},
  {"C", "212"},
  {"D", "111"},
  {"E", "21"},
  {"F", "1112"},
  {"G", "1122"},
  {"H", "211"},
  {"I", "2"},
  {"J", "2211"},
  {"K", "1212"},
  {"L", "112"},
  {"M", "2112"},
  {"N", "22"},
  {"O", "12"},
  {"P", "2121"},
  {"Q", "2122"},
  {"R", "122"},
  {"S", "121"},
  {"T", "1"},
  {"U", "221"},
  {"V", "2111"},
  {"W", "2212"},
  {"X", "1211"},
  {"Y", "222"},
  {"Z", "1111"},
  {"0", "11111"},
  {"1", "12221"},
  {"2", "21112"},
  {"3", "11211"},
  {"4", "11121"},
  {"5", "11112"},
  {"6", "21111"},
  {"7", "22111"},
  {"8", "22221"},
  {"9", "22122"},
};
static constexpr size_t encode_table_num = sizeof(encode_table) / sizeof(encode_table[0]);

DotCodeBuzzer::DotCodeBuzzer()
  : m_state(PLAYING_DONE)
  , m_pin(-1)
  , m_buzzer_on(LOW)
  , m_buzzer_off(HIGH)
  , m_text(nullptr)
  , m_code(nullptr)
  , m_verbosity(1)
{
}

DotCodeBuzzer::~DotCodeBuzzer()
{
  buzzer_off();
}

void
DotCodeBuzzer::buzzer_off()
{
  if (m_pin != -1)
    digitalWrite(m_pin, m_buzzer_off);
}

void
DotCodeBuzzer::buzzer_on()
{
  if (m_pin != -1)
    digitalWrite(m_pin, m_buzzer_on);
}

void
DotCodeBuzzer::setup(int pin, byte buzzer_on)
{
  m_pin  = pin;
  m_buzzer_on = buzzer_on;
  m_buzzer_off = (buzzer_on == LOW ? HIGH : LOW);
  pinMode(m_pin, OUTPUT);
  buzzer_off();
}

void
DotCodeBuzzer::start(const char *text)
{
  m_state = PLAYING_GAP;
  m_text = text;
  m_code = "";
  m_ref_millis = millis();
  m_gap_time = 500;
}

void
DotCodeBuzzer::cancel()
{
  buzzer_off();
  m_state = PLAYING_DONE;
}

static inline bool istilde(char c) {
  return c == '~';
}

bool
DotCodeBuzzer::next_code_element()
{
  if (!m_text || *m_text == 0) {
    // Natural end of message so we are done
    if (m_verbosity > 2) DebugSerial_println("eom");
    buzzer_off();
    m_state = PLAYING_DONE;
    DebugSerial_println();
    return true;
  } else if (istilde(*m_text)) {
    // Ascii SYN play 1 second of silence
    m_text++;
    m_code = "";
    m_gap_time = 1000;
    m_ref_millis = millis();
    m_state = PLAYING_GAP;
    m_num_dits = 0;
    DebugSerial_print('~');
    return false;
  } else if (ispunct(*m_text)) {
    // Skip any whitespace after the punctuation to get to the beginning of the
    // next sentence.
    do {
      m_text++;
    } while ((*m_text != 0) && isspace(*m_text));

    // What's next in the text?
    if ((*m_text == 0) || istilde(*m_text)) {
      // End of the message
      m_code = "333";
    } else {
      // Just end of sentence
      m_code = "33";
    }
  } else if (isspace(*m_text)) {
    // Skip over whitespace
    do {
      *m_text++;
    } while (*m_text && isspace(*m_text));
    m_code = "3";
  } else {
    // Find the prefix string that is the best match to the text
    m_code = nullptr;
    for (int ii = 0; ii < encode_table_num; ii++) {
      const char *prefix_str = encode_table[ii].prefix;
      const size_t prefix_len = strlen(prefix_str);
      if (strncmp(m_text, prefix_str, prefix_len) == 0) {
        m_code = encode_table[ii].code;
        m_text += prefix_len;
        break;
      }
    }
    if (!m_code) {
      // Garbled crap -- play the error sequence.
      m_text = nullptr;
      m_code = "121212333";
    }
  }

  // Start the first bit of the new character
  return next_code_digit();
}

bool
DotCodeBuzzer::next_code_digit()
{
  const char dit = *m_code++;
  if (dit == 0) {
    // We already accounted for the pause in the code as we played the dit that just ended
    return next_code_element();
  } else {
    m_num_dits = dit - '0';
    DebugSerial_print(dit);
  }

  m_state = PLAYING_DIT;
  m_ref_millis = millis();
  buzzer_on();
  return false;
}

bool
DotCodeBuzzer::done_playing()
{
  if (m_state == PLAYING_DONE) {
    return false;
  }

  // Compute time elapsed since our "ref_millis"
  unsigned elapsed = millis() - m_ref_millis;

  if (m_state == PLAYING_DIT) {
    // We are playing the buzz, is it time to turn off?
    if (elapsed >= dit_time) {
      // Time to turn off
      m_num_dits -= 1;
      buzzer_off();
      m_state = PLAYING_GAP;
      m_ref_millis = millis();
      if (m_num_dits > 0) {
        m_gap_time = gap_time;
      } else {
        m_gap_time = 4 * (dit_time + gap_time);
        if (*m_code == 0) {
          m_gap_time = 8 * (dit_time + gap_time);
          DebugSerial_print('.');
        }
      }
    }
    return false;
  }

  // We are in the gap time, are we completely done?
  if (elapsed < m_gap_time)
    return false;

  if (m_num_dits > 0) {
    m_ref_millis = millis();
    m_state = PLAYING_DIT;
    buzzer_on();
    return false;
  }

  // Time to move to next dit
  return next_code_digit();
}

