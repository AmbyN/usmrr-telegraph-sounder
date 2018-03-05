# usmrr-telegraph-sounder
This project holds source code and documentation for the Telegraph Sounder for Bernie Kempinski's USMRR Aquia Line model railroad (http://usmrr.blogspot.com/). Bernie's layout models the US Military Railroad in 1863 during the height of the US Civil War. 

The telegraph sounder system for Bernie's layout consists of five train order stations connected to the dispatcher's office. At each station, there is a small control panel embedded in the layout fascia resembling the image below.

[insert control panel image here]

## User Interface

The user interface for this project is very simple. The train crew of a train arriving at a station waits for the BUSY lamp to be off, selects their train number with the rotary switch and then presses either the "REGUAR" or "EXTRA" train button, as appropriate for their train. When either of those buttons is pressed, the BUSY lamp will start to blink out the USMRR dot code for the OS message for the train, and simultaneously, the BUSY lamps on all of the other station platforms will illuminate solidly. 

In the dispatcher's office, the telegraph sounder will start thunking away, playing the dot code of the train OS report. As a visual indicator of which station is calling (and a cheat for the dispatcher),  the station's LED on the dispatcher's control panel will also blink out the sounder activity. 

The sounder will continue to play the OS message until one of the following events happens:

 * the dispatcher presses the "STOP" button for the station, or
 * the train crew rotates the train selector knob, or
 * the train crew presses and holds both the "REGULAR" and "EXTRA" train buttons for 2 seconds.
 
At this point, the station will go back to IDLE and the BUSY lamp will turn off.

## Hardware

The hardware at each station consists of the fascia control panel with the rotary switch, BUSY lamp (LED), and "REGULAR" and "EXTRA" push buttons, along with a Model Railroad Control Systems "Morse Code Buzzer Board" (http://www.modelrailroadcontrolsystems.com/morse-code-buzzer-board/) which uses an Arduino Pro-Mini embedded microcontroller. A schematic of the connection of the fascia panel to the Morse Code Buzzer Board is shown below:

[insert schematic here]

In the dispatcher's office, all of the "SOUNDER_OUT_L" lines from the stations are connected to the vintage telegraph sounder through a diode stack with an LED in parallel with each station so that the dispatcher can see at a glance which station is calling. The dispatcher also has a set of "STOP" buttons, one for each station, which connect back to the STOP_L inputs at the corresponding station.

The BUSY_OUT_L lines from all of the stations are wire-ORed in the dispather's office and pulled high through a 10 kOhm pull-up resistor to the sounder's positive supply voltage (V_SOUNDER). This allows any station to pull its BUSY_OUT_L line low and cause all of the other stations to see their BUSY_IN_L go low to signal that the line is busy.

[insert dispatcher schematic here]

## Software

The software has been designed around a simple state machine as pictured below. At power-on, each station will enter the ST_IDLE state, de-asserting the BUSY_OUT_L and SOUNDER_L and extinguishing the local BUSY lamp on the fascia panel. 

### OSing a Train

When a train arrives at the station, the crew will use the rotary selector knob to select their train number, and then wait for the BUSY lamp to go out. Then they will press either the "REGULAR" or "EXTRA" button accoriding to their train. This will move the station's state machine to the ST_SOUNDING state. While in this state, the station will pull the BUSY_OUT_L line low to signal to the other stations that the line is busy and will start to play the OS message for the train on the sounder. As the message plays, the BUSY lamp on the local panel will blink on and off with each "dit" in the OS message pattern.

### Canceling an in-progress OS

Once in the ST_SOUNDING state, the station's train OS message will continue to play until one of the following occurs:

 * the dispatcher presses the "STOP" button for the station
 * the train crew chenges the rotary train selector knob
 * the train crew presses both the "REGULAR" and "EXTRA" buttons simultaneously

While this happens, all of the other station panels will see their BUSY_IN_L signal asserted low and will prevent operators at those stations from starting another train report.
