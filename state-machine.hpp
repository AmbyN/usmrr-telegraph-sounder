// state-machine.hpp -- interface to state machine for USMRR telegraph sounder
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
#ifndef INCLUDED_state_machine_hpp
#define INCLUDED_state_machine_hpp

extern "C" {
#include "simplehsm.h"
};

namespace hsm {
  class machine {
  public:
    machine();
    ~machine();

    void setup(bool valid_settings);
    
    // train() -- the train selector changed
    void train();

    // regular() -- the "REGULAR" train button has changed state
    void regular(bool pressed);

    // extra() -- the "EXTRA" train button has changed state
    void extra(bool pressed);

    // stop() -- the "STOP" input from the dispatcher has changed state
    void stop(bool pressed);

    // busy() -- the "BUSY_L" input has changed state
    void busy(bool busy);

    // timeout() -- the simple timer has expired. This is a pretty limited capability only the
    // leafmost state in the state stack can use this event
    void timeout(unsigned which_timer);

    // sounder_done() -- the dot code message has finished playing
    void sounder_done();
  };
  
} // namespace hsm

#endif
