# StandAloneSynth37key for v0.5

Goals 1) There is something turning notes off after a certain time
      2) Basic Synth has some of these library changes from later version of Marcel's master repository integrated. Bring this into line with that
      3) A voltage monitor for the battery of the stand alone keyboard prototype
      4) Band in the box like key shifts with number of bars eg Menu items like I: Set Key, 4 bars, II: +4th, 2 bars ... at first have 4 sections
      5) A pitch adgustment part interface that plays as you adgust items of song
      
This is for someone who wants to wire up from a toy keyboard and pots, buttons to play the synth in a self contained unit. That is why I forked from Marcel Licence 
who has developed a number of modular synth projects and shared.

I made a thread where I started to list new features.
https://github.com/marcel-licence/esp32_basic_synth/discussions/60


This project is moving along as a fork of basic synth. I've made my own knobs, banks of parameters physical interface in this self contained unit.
I am milking this code for all I can to develop something I have a good deal of control.

Ran into a lot of little electronics building issues with physically making it stable and wiring it. My projects are kind of semi-wired and semi-wires into temporary pin-connectors like you have on a test arduino where I soldered some connectors to the ESP32 dev module.
Getting it to be stable and using the right wire and lengths so I can keep opening it up and improving it without breaking some connection takes experience. 

Issues
I am starting to look more at the modules I was just using at how the work and learn from that.
Had some weird DAC issues when i had a floating local SCLK instead of earthing it. 
Sometimes I get a bit of clipping with the sinewave or the current use causes it to lose it's stability until I had the wiring done right. 
Started to use my on AWG22 solid core breadboard style wires cut from a spool so I could make things stable.

Feature upgrades
I like my display setup - i have 8 sectors and a routing for showing the set levels. I need a button I think to put it into edit or not edit so that I can change sets of parameters without overwriting things.

New Features to be started
I need to make a patch write/read or at least read out a list of numbers to save as presets.
I need to make a physical 5 pin DIN midi in/out so I can slave or master out the keys.
