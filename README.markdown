# Saitek X-52 Pro Joystick X-Plane Plugin (64 bit)

## About

Extended sothis plugin to my custom X-Plane joystick mapping.
As little support was found to Saitek X52 Pro joystick in Linux operating system, 
I made a custom plugin to take advantage of all joystick features.

## Features

Multi Function Display *MFD* used to display:

* COM1 frequencies (active and standby)
* FLAPS
* HSI source

LEDs associated with specific buttons are used to increase pilot awareness 
of aircraft configuration using:

* *E* and *i* buttons, gear up and gear down buttons respectively, with:
    * no light - gear retracted
    * green    - extended
    * orange   - trasitioning
* *D* button, reverse toogle red when activated, green otherwise
* *POV 1*, nosewheel steering indicator: green - off, orange - on
* *T3* and *T4* switch, taxi light switch and indicator
* *T5* and *T6* switch, landing light switch and indicator


## Build 

### Dependencies
* [X-Plane SDK 2.1.2](http://www.xsquawkbox.net/xpsdk)

### Compile
1. Extract X-Plane SDK to project root folder.
2. Run make command in project root folder.
3. Copy build/release/x52control.xpl to X-Plane plugins folder.
4. Happy flying!
