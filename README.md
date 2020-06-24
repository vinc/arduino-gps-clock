# Arduino GPS Clock

This a little personal project to assemble various components laying around
into a very precise GPS clock, accurate up to 50 nanoseconds in theory.

I used an old Arduino Leonardo board with an even older Motoral UT+ Oncore GPS
receiver to display the time on a 7-segments led display.

## Install

    $ git clone arduino-gps-clock
    $ cd arduino-gps-clock
    $ arduino-cli compile -b arduino:avr:leonardo
    $ arduino-cli upload -p /dev/ttyACM0 -b arduino:avr:leonardo

## External Libraries

- https://github.com/arduino-libraries/Arduino_DebugUtils
- https://github.com/RobTillaart/Arduino/tree/master/libraries/HT16K33
