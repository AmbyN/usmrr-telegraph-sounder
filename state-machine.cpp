// state-machine.cpp -- implements state machine logic for the USMRR telegraph sounder
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
#include "state-machine.hpp"
#include "debug-serial.h"
#include "global-objects.hpp"
#include <avr/pgmspace.h>

namespace hsm {

// Ambiance strings (stored in flash)
const char ambiance_0[] PROGMEM = "THERE IS AN EMPTY BOX CAR HERE FOR NEXT TRAIN TO PICK UP.";
const char ambiance_1[] PROGMEM = "CAN WE FOLLOW FURY TO FALMOUTH FOR WATER. WE HAVE BUT 8 INCHES.";
const char ambiance_2[] PROGMEM = "RUN AN EXTRA TRAIN TO PICK UP TROOPS AT FALMOUTH.";
const char ambiance_3[] PROGMEM = "TROOPS ARE LOADED AND READY TO MOVE.";
const char ambiance_4[] PROGMEM = "DID A CAR LEAVE AQUIA LOADED WITH A TUB OF BUTTER FOR COL G A MYERS?";
const char ambiance_5[] PROGMEM = "THE DISPATCHER WILL ARRANGE A GRAVEL TRAIN FROM POTOMAC CREEK TO STONEMANS";
const char ambiance_6[] PROGMEM = "THE SWITCH LEADING TO THE ENGINE TRACK IS BEING TAKEN UP. WHEN YOU COME SEE THAT ALL IS RIGHT BEFORE PASSING OVER.";

const char * const ambiance_strings[] PROGMEM = {
  ambiance_0, ambiance_1, ambiance_2, ambiance_3, ambiance_4, ambiance_5, ambiance_6,
};
const unsigned num_ambiance_strings = ARRAY_SIZE(ambiance_strings);

char gl_message[200];

enum signals {
  SIG_TRAIN = SIG_USER,  // Train selector has changed
  SIG_REGULAR,           // "REGULAR" button is pressed
  SIG_EXTRA,             // "EXTRA" button is pressed
  SIG_STOP,              // Dispatcher has pressed "STOP"
  SIG_BUSY,              // Another station is "BUSY"
  SIG_TIMEOUT,           // An internally generated timeout
  SIG_SOUNDER_DONE,      // The current sounder message is finished
};

simplehsm_t hsm;

/**/stnext on(int signal, void *param);
/***/stnext idle(int signal, void *param);
/***/stnext busy(int signal, void *param);
/****/stnext busy_lamp_on(int signal, void *param);
/****/stnext busy_lamp_off(int signal, void *param);
/***/stnext sounding(int signal, void *param);
/***/stnext setup(int signal, void *param);
/****/stnext setup_valid(int signal, void *param);
/****/stnext setup_error(int signal, void *param);
/***/stnext ambiance(int signal, void *param);

static bool ambiance_timer_expired = false;

stnext on(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      simplehsm_init_transition_state(&hsm, idle);
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println("on::entry");
      if (settings.m_station == 0) {
        // Arm the ambiance timer
        const long base_interval = 5L * 60L * 1000L;
        const long random_variance = 10L * 60L * 1000L;
        timer_events[0].arm(base_interval + random(0, random_variance));
        ambiance_timer_expired = false;
      }

      return stnone;

    case SIG_EXIT:
      DebugSerial_println("on::exit");
      return stnone;

    case SIG_TRAIN:
      DebugSerial_println("on::train");
      return stnone;

    case SIG_REGULAR:
      DebugSerial_println("on::regular");
      return stnone;

    case SIG_EXTRA:
      DebugSerial_println("on::extra");
      return stnone;

    case SIG_STOP:
      DebugSerial_println("on::stop");
      return stnone;

    case SIG_BUSY:
      DebugSerial_println("on::busy");
      return stnone;

    case SIG_TIMEOUT:
      DebugSerial_println("on::timeout");
      {
        // If the ambiance timer expires while we are not in state "idle", we record
        // that it has expired so that when we go back to "idle" we can play the ambiance
        // message after about 30 seconds.
        const unsigned which_timer = *reinterpret_cast<unsigned*>(param);
        if (which_timer == 0) {
          ambiance_timer_expired = true;
        }
      }
      return stnone;

    case SIG_SOUNDER_DONE:
      DebugSerial_println("on::sounder_done");
      return stnone;
  }
  return stnone;
}

stnext idle(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println("idle::entry");
      busy_follows_sounder = false;
      digitalWrite(BUSY_OUT_H, LOW);
      digitalWrite(BUSY_LAMP_L, HIGH);
      if (ambiance_timer_expired) {
        ambiance_timer_expired = false;
        timer_events[0].arm(30000L);       
      }
      return stnone;

    case SIG_EXIT:
      DebugSerial_println("idle::exit");
      return stnone;

    case SIG_REGULAR:
    case SIG_EXTRA:
      {
        const bool pressed = *reinterpret_cast<bool *>(param);
        if (pressed) {
          // Setup the message here while we know which button was pressed.
          strcpy(gl_message, "WWW OS ~");
          strcat(gl_message, station_names[settings.m_station]);
          strcat(gl_message, " ");
          if (signal == SIG_EXTRA) {
            strcat(gl_message, "E");
          }
          strcat(gl_message, train_selector.text());
          strcat(gl_message, ".~~~~");
          simplehsm_transition_state(&hsm, sounding);
        }
      }
      return stnone;

    case SIG_BUSY:
      {
        const bool pressed = *reinterpret_cast<bool *>(param);
        if (pressed) {
          simplehsm_transition_state(&hsm, busy);
        }
      }
      return stnone;

    case SIG_TIMEOUT:
      {
        const unsigned which_timer = *reinterpret_cast<unsigned*>(param);
        if (which_timer == 0) {
          simplehsm_transition_state(&hsm, ambiance);
          return stnone;
        }
      }
      break;
  }
  return on;
}

stnext busy(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      simplehsm_init_transition_state(&hsm, busy_lamp_on);
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println("busy::entry");
      return stnone;

    case SIG_EXIT:
      DebugSerial_println("busy::exit");
      return stnone;

    case SIG_BUSY:
      {
        const bool busy = *reinterpret_cast<const bool *>(param);
        if (!busy) simplehsm_transition_state(&hsm, idle);
      }
      return stnone;
  }
  return on;
}

stnext busy_lamp_on(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println("busy_lamp_on::entry");
      digitalWrite(BUSY_LAMP_L, LOW);
      timer_events[1].arm(500);
      return stnone;

    case SIG_EXIT:
      DebugSerial_println("busy_lamp_on::exit");
      timer_events[1].cancel();
      digitalWrite(BUSY_LAMP_L, HIGH);
      return stnone;

    case SIG_TIMEOUT:
      {
        const unsigned which_timer = *reinterpret_cast<unsigned*>(param);
        if (which_timer == 1) {
          simplehsm_transition_state(&hsm, busy_lamp_off);
          return stnone;
        }
      }
      break;
  }
  return busy;
}

stnext busy_lamp_off(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println("busy_lamp_off::entry");
      timer_events[1].arm(500);
      return stnone;

    case SIG_EXIT:
      timer_events[1].cancel();
      DebugSerial_println("busy_lamp_off::exit");
      return stnone;

    case SIG_TIMEOUT:
      {
        const unsigned which_timer = *reinterpret_cast<unsigned*>(param);
        if (which_timer == 1) {
          simplehsm_transition_state(&hsm, busy_lamp_on);
          return stnone;
        }
      }
      break;
  }
  return busy;
}

stnext sounding(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      digitalWrite(BUSY_OUT_H, HIGH);
      DebugSerial_println("sounding::entry");
      dot_code_buzzer.start(gl_message);
      busy_follows_sounder = true;
      return stnone;

    case SIG_EXIT:
      DebugSerial_println("sounding::exit");
      dot_code_buzzer.cancel();
      return stnone;

    case SIG_TRAIN:
      simplehsm_transition_state(&hsm, idle);
      return stnone;

    case SIG_REGULAR:
    case SIG_EXTRA:
      {
        const bool pressed = *reinterpret_cast<bool *>(param);
        if (pressed) {
          simplehsm_transition_state(&hsm, idle);
        }
      }
      return stnone;

    case SIG_STOP:
      simplehsm_transition_state(&hsm, idle);
      return stnone;

    case SIG_SOUNDER_DONE:
      simplehsm_transition_state(&hsm, sounding);
      return stnone;
  }
  return on;
}

stnext setup(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      {
        // On entry to setup we transition to setup_error to force the user to twist
        // the train selector.
        simplehsm_init_transition_state(&hsm, setup_error);
      }
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println(F("setup::entry"));
      return stnone;

    case SIG_EXIT:
      // The only way out of setup is by releasing a button in setup_valid
      DebugSerial_println(F("setup::exit"));
      return stnone;

    case SIG_TRAIN:
      {
        const bool valid_station =  (train_selector.selected() < station_names_num);
        simplehsm_transition_state(&hsm, (valid_station ? setup_valid : setup_error));
      }
      return stnone;

  }
  return on;
}

stnext setup_valid(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println("setup_valid::entry");
      settings.m_station = train_selector.selected();
      strcpy(gl_message, "I AM ");
      strcat(gl_message, station_names[settings.m_station]);
      strcat(gl_message, ".~~");
      dot_code_buzzer.start(gl_message);
      busy_follows_sounder = true;
      return stnone;

    case SIG_EXIT:
      DebugSerial_println("setup_valid::exit");
      dot_code_buzzer.cancel();
      busy_follows_sounder = false;
      return stnone;

    case SIG_REGULAR:
    case SIG_EXTRA:
      {
        const bool pressed = *reinterpret_cast<bool*>(param);
        if (!pressed) {
          DebugSerial_print("saving settings for station "); DebugSerial_println(station_names[settings.m_station]);
          settings.write();
          simplehsm_transition_state(&hsm, idle);
        }
      }
      return stnone;

    case SIG_SOUNDER_DONE:
      simplehsm_transition_state(&hsm, setup_valid);
      return stnone;
  }
  return setup;
}

stnext setup_error(int signal, void *param) {
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      DebugSerial_println(F("setup_error::entry"));
      dot_code_buzzer.start(".~~");
      busy_follows_sounder = true;
      return stnone;

    case SIG_EXIT:
      DebugSerial_println(F("setup_error::exit"));
      dot_code_buzzer.cancel();
      return stnone;

    case SIG_SOUNDER_DONE:
      simplehsm_transition_state(&hsm, setup_error);
      return stnone;
  }
  return setup;
}

stnext ambiance(int signal, void *param)
{
  switch (signal) {
    case SIG_INIT:
      return stnone;

    case SIG_ENTRY:
      {
        DebugSerial_println("ambiance::entry");

        // Pick a random message from the ambiance table
        unsigned ambiance_idx = random(num_ambiance_strings);
        strcpy_P(gl_message, pgm_read_word(&ambiance_strings[ambiance_idx]));
        DebugSerial_println(gl_message);

        digitalWrite(BUSY_OUT_H, HIGH);
        dot_code_buzzer.start(gl_message);
        busy_follows_sounder = true;
      }
      return stnone;

    case SIG_EXIT:
      DebugSerial_println("ambiance::entry");
      busy_follows_sounder = false;
      digitalWrite(BUSY_OUT_H, LOW);
      return stnone;

    case SIG_SOUNDER_DONE:
      // Exit through "on" so we pick a new ambiance timer.
      simplehsm_transition_state(&hsm, on);
      return stnone;
  }
  return on;
}


machine::machine() {
}

machine::~machine() {
}

void machine::setup(bool valid_settings) {
  simplehsm_initialize(&hsm, on);
  if (!valid_settings) {
    simplehsm_transition_state(&hsm, hsm::setup);
  }
}

// train() -- the train selector changed
void machine::train() {
  DebugSerial_print(F("train ")); DebugSerial_println(train_selector.text());
  simplehsm_signal_current_state(&hsm, SIG_TRAIN, nullptr);
}

// regular() -- the "REGULAR" train button has changed state
void machine::regular(bool pressed) {
  DebugSerial_print(F("regular ")) ; DebugSerial_println(pressed);
  simplehsm_signal_current_state(&hsm, SIG_REGULAR, &pressed);
}

// extra() -- the "EXTRA" train button has changed state
void machine::extra(bool pressed) {
  DebugSerial_print(F("extra ")) ; DebugSerial_println(pressed);
  simplehsm_signal_current_state(&hsm, SIG_EXTRA, &pressed);
}

// stop() -- the "STOP" input from the dispatcher has changed state
void machine::stop(bool pressed) {
  DebugSerial_print(F("stop ")) ; DebugSerial_println(pressed);
  simplehsm_signal_current_state(&hsm, SIG_STOP, &pressed);
}

// busy() -- the "BUSY_L" input has changed state
void machine::busy(bool busy) {
  DebugSerial_print(F("busy ")) ; DebugSerial_println(busy);
  simplehsm_signal_current_state(&hsm, SIG_BUSY, &busy);
}

// timeout() -- the simple timer has expired. This is a pretty limited capability only the
// leafmost state in the state stack can use this event
void machine::timeout(unsigned which_timer) {
  simplehsm_signal_current_state(&hsm, SIG_TIMEOUT, &which_timer);
}

// sounding_done() -- the sounder has finished playing the message.
void machine::sounder_done() {
  DebugSerial_println(F("sounder_done"));
  simplehsm_signal_current_state(&hsm, SIG_SOUNDER_DONE, nullptr);
}


} // namespace hsm

