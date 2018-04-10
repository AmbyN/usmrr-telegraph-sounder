// train-selector.hpp--Use a rotary switch as a train selector.
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

// This class implements a simple scanner for a rotary switch that has the "common" terminal of the switch
// connected to an Arduino INPUT_PULLUP pin and the position terminals connected to some combination of
// Arduion output pins. The code cycles through the list of output pins driving each pin "active" for a few
// milliseconds and looking to see which ouput shows up on the input.
//
// In the simplest hardware configuration, you have the output pins connected directly to the switch, and the
// outputs should be active LOW. In some cases, you may have an external inverting driver, and you can specify
// these outputs as active HIGH. Each selector input can have a separate value string.
#include "debug-serial.h"
struct TrainSelectorOutputPin {
  uint8_t pin;
  uint8_t active;
  const char *text;
};

class TrainSelector {
    uint8_t m_input_pin;
    uint8_t m_input_active;
    const TrainSelectorOutputPin *m_outputs;
    uint8_t m_num_outputs;
    long    m_ref_millis;
    uint8_t m_scanned;
    uint8_t m_selected;
    bool    m_changed;
  public:
    TrainSelector() {
    }

    ~TrainSelector() {
      for (uint8_t ii = 0; ii < m_num_outputs; ii++) {
        const TrainSelectorOutputPin &output = m_outputs[ii];
        pinMode(output.pin, INPUT);
      }
    }

    void set_input(const uint8_t input_pin, const uint8_t input_active) {
      m_input_pin = input_pin;
      m_input_active = input_active;
      pinMode(m_input_pin, INPUT_PULLUP);
    }

    void set_outputs(const TrainSelectorOutputPin *outputs, const uint8_t num_outputs) {
      m_outputs = outputs;
      m_num_outputs = num_outputs;
      m_ref_millis = millis();
      m_scanned = 0;
      m_selected = 0;
      for (uint8_t ii = 0; ii < m_num_outputs; ii++) {
        const TrainSelectorOutputPin &output = m_outputs[ii];
        pinMode(output.pin, OUTPUT);
        digitalWrite(output.pin, (output.active == LOW) ? HIGH : LOW);
      }
    }

    // loop() should be called every main loop iteration
    //
    // We perform the slow scan of the outputs to see if the selector has been changed
    bool loop() {
      const long elapsed = millis() - m_ref_millis;
      m_changed = false;
      if (elapsed >= 10) {
        if (digitalRead(m_input_pin) == m_input_active) {
          m_changed = (m_selected != m_scanned);
          m_selected = m_scanned;
        }

        // Turn off the currently scanned output
        const auto &curr_output = m_outputs[m_scanned];
        digitalWrite(curr_output.pin, (curr_output.active == LOW) ? HIGH : LOW);

        // Scan the next output
        m_scanned += 1;
        if (m_scanned >= m_num_outputs) m_scanned = 0;

        // Turn on the new output
        const auto &next_output = m_outputs[m_scanned];
        digitalWrite(next_output.pin, (next_output.active == LOW) ? LOW : HIGH);
        m_ref_millis = millis();
      }
      return m_changed;
    }

    // changed() returns whether the value changed on the last call to loop()
    bool changed() {
      return m_changed;
    }

    uint8_t selected() {
      return m_selected;
    }

    // value() returns the most recently selected value.
    const char *text() {
      return m_outputs[m_selected].text;
    }

};
