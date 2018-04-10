// global-objects.hpp -- global objects manipulated by the stqte machine.
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

#include "dot-code.hpp"
#include "mrcs-mcbc-pins.hpp"
#include "settings.hpp"
#include "train-selector.hpp"

// The saved setings for the station as read from EEPROM
extern struct EepromSettings settings;

// In the sounding states (sounding and setup) we want the BUSY_LAMP to blink in concert with the
// SOUNDER output.
extern bool busy_follows_sounder;

// These are the other GPIO pins used -- here I am mapping them to the names on the MRCS Morse Code Buzzer Controller
// board. These names are defined in mrcs-mcbc-pins.hpp, where they are mapped to the Arduino pins.
constexpr uint8_t SOUNDER_OUT_H = LOAD_2;
constexpr uint8_t BUSY_OUT_H = LOAD_1;
constexpr uint8_t BUSY_IN_L = STOP_6;
constexpr uint8_t STOP_IN_L = STOP_5;
constexpr uint8_t BUSY_LAMP_L = STOP_3;
constexpr uint8_t EXTRA_IN_L = STOP_2;
constexpr uint8_t REGULAR_IN_L = STOP_1;

// compute size of an array
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

// defined in usmrr-telegraph-sounder.ino
extern const char * const station_names[];
extern const uint8_t station_names_num;

// dot_code_buzzer -- plays our dot code messages
extern DotCodeBuzzer dot_code_buzzer;

// The train selector scans a bunch of the Arduino output pins to see which position the rotary swithc is set.
extern TrainSelector train_selector;

// SimpleTimer implements a one-deep timer event for the state machine.
class SimpleTimer {
    unsigned m_duration;
    unsigned long m_ref_millis;
  public:
    SimpleTimer() : m_duration(0) {}
    bool loop() {
      if (m_duration == 0) {
        return false;
      }
      if ((millis() - m_ref_millis) > m_duration) {
        m_duration = 0;
        return true;
      }
      return false;
    }
    void arm(unsigned duration) {
      m_duration = duration;
      m_ref_millis = millis();
    }
    void cancel() {
      m_duration = 0;
    }
};

// We declare two timers: [0] for the random ambiance timer and [1] for the flashing in
// the busy states.
extern SimpleTimer timer_events[2];

// Button -- this class wraps a an Arduino input pin and its polarity so that
// we can think in the rest of the application in terms of the input being "active".
// On every pass through the Arduino "loop()" routing, each button's loop() method
// is invoked and returns true if the button has changed state, which is then turned
// into a state machine event.
//
// We use a setup() method here rather than the constructor because these are global
// objects and their constructors are called before the Arduino runtime has been initialized
// and I'm not sure that pinMode() and digitalWrite() are available.
class Button {
  private:
    uint8_t m_pin;
    uint8_t m_active;
    bool m_pressed;
    bool m_last_pressed;
    unsigned long m_bounce_millis;

  public:
    void setup(uint8_t pin, uint8_t active) {
      m_pin = pin;
      m_active = active;
      pinMode(pin, (active == LOW) ? INPUT_PULLUP : INPUT);
      m_pressed = m_last_pressed = (digitalRead(m_pin) == m_active);
      m_bounce_millis = millis();
    }
    bool loop() {
      // Read the pin to see if it is 'pressed'
      const bool pressed = (digitalRead(m_pin) == m_active);
      const unsigned long now = millis();
      if (pressed != m_last_pressed) {
        m_bounce_millis = now;
        m_last_pressed = pressed;
        return false;
      }
      // Has it been steady for 10 ms?
      if (now - m_bounce_millis < 10) {
        return false;
      }

      const bool changed = (pressed != m_pressed);
      m_pressed = pressed;
      return changed;
    }
    bool pressed() {
      return m_pressed;
    }
};

extern Button stop_button;
extern Button busy_button;
extern Button regular_button;
extern Button extra_button;
