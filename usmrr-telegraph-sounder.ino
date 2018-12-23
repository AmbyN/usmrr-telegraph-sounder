// usmrr-telegraph-sounder -- Arduino sketch to play USMRR "dot code" on Bernie Kempinski's
//   "Aquia Line" model railroad layout
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
#include <Arduino.h>

#include <avr/pgmspace.h>

#include "debug-serial.h"
#include "global-objects.hpp"
#include "state-machine.hpp"

// The array of station names.
const char * const station_names[] = {
  [0] = "AQ", // Aquia
  [1] = "BR", // Brooke
  [2] = "PO", // Potoma
  [3] = "ST", // Stanaford
  [4] = "FA", // Falmouth
};
const uint8_t station_names_num = ARRAY_SIZE(station_names);

// Each station saves a configuration record in the Arduino's EEPROM so that
// it knows what station name to play in the OS announcement.
struct EepromSettings settings;

// The DotCodeBuzzer object plays dot code messages
DotCodeBuzzer dot_code_buzzer;

// The SimpleTimer objects generate very simple one-deep timer events for the state machine
// We declare two of them here: [0] is the ambiance message timer, [1] is used in the "busy" state
SimpleTimer timer_events[2];

// My BBLeo board doesn't have an inverting driver on the pins that are used for the LOAD
// output from the Arduino Pro Mini on the MRCS Morse Code Buzzer Controller board. So these
// constants are used to fake the outputs associated with the train selector to the correct
// polarity.
#ifdef ARDUINO_AVR_LEONARDO
constexpr uint8_t OUTPUT_NORMAL = LOW;
constexpr uint8_t OUTPUT_INVERTED = LOW;
#else
constexpr uint8_t OUTPUT_NORMAL = LOW;
constexpr uint8_t OUTPUT_INVERTED = HIGH;
#endif

// The TrainSelector object scans through the output pins driving each
// one in turn to see which shows up at an input pin
TrainSelector train_selector;
static const TrainSelectorOutputPin output_pins[] = {
  {  LOAD_3, OUTPUT_INVERTED,  "1" },
  {  LOAD_4, OUTPUT_INVERTED,  "2" },
  {  LOAD_5, OUTPUT_INVERTED,  "3" },
  {  LOAD_6, OUTPUT_INVERTED,  "4" },
  {  LOAD_7, OUTPUT_INVERTED,  "5" },
  {  LOAD_8, OUTPUT_INVERTED,  "6" },
  { START_7,  OUTPUT_NORMAL,   "7" },
  { START_6,  OUTPUT_NORMAL,   "8" },
  { START_5,  OUTPUT_NORMAL,   "9" },
  { START_4,  OUTPUT_NORMAL,  "10" },
  { START_3,  OUTPUT_NORMAL,  "11" },
  { START_2,  OUTPUT_NORMAL,  "12" },
};
constexpr uint8_t TRAIN_IN_L = STOP_4;

Button stop_button;
Button busy_button;
Button regular_button;
Button extra_button;


// The state machine itself.
hsm::machine state_machine;

// In a couple of the states, we want the BUSY_LAMP to follow
// the SOUNDER output.
bool busy_follows_sounder = false;

void setup()
{
  // Read the unattached A7 input pin a few times to get a random seed.
  long seed = 0;
  while(millis() < 2000) {
    seed = seed << 4;
    seed |= analogRead(A7);
    delay(seed & 0x7f);
  }
  randomSeed(seed);
  DebugSerial_begin(9600);
  DebugSerial_println(F("USMRR Aquia Line Telegraph Sounder v1.0"));

  stop_button.setup(STOP_IN_L, LOW);
  busy_button.setup(BUSY_IN_L, LOW);
  regular_button.setup(REGULAR_IN_L, LOW);
  extra_button.setup(EXTRA_IN_L, LOW);

  pinMode(BUSY_LAMP_L, OUTPUT); digitalWrite(BUSY_LAMP_L, HIGH);
  pinMode(BUSY_OUT_H, OUTPUT); digitalWrite(BUSY_OUT_H, LOW);

  dot_code_buzzer.setup(SOUNDER_OUT_H, HIGH);

  train_selector.set_input(TRAIN_IN_L, LOW);
  train_selector.set_outputs(output_pins, ARRAY_SIZE(output_pins));

  bool valid_settings = settings.read();
  if (regular_button.pressed() || extra_button.pressed()) {
    valid_settings = false;
  }
  state_machine.setup(valid_settings);
}

void loop()
{
  // Has the sounder stopped playing?
  if (dot_code_buzzer.done_playing()) state_machine.sounder_done();
  
  // Do we want the BUSY_LAMP to show the sounder output?
  if (busy_follows_sounder) digitalWrite(BUSY_LAMP_L, dot_code_buzzer.buzzing() ? LOW : HIGH);

  // Has any of the timers expired?
  for (unsigned timer = 0; timer < ARRAY_SIZE(timer_events); timer++)
    if (timer_events[timer].loop()) state_machine.timeout(timer);

  // update the train selector
  if (train_selector.loop()) state_machine.train();

  // update the buttons
  if (busy_button.loop()) state_machine.busy(busy_button.pressed());
  if (regular_button.loop())state_machine.regular(regular_button.pressed());
  if (extra_button.loop()) state_machine.extra(extra_button.pressed());
  if (stop_button.loop()) state_machine.stop(stop_button.pressed());

}
