# ScoreboardWiFiControl
Drop-in replacement for the Baseball 3314 Varsity/All-Star/BSN/Sportable scoreboard controller

We got tired of the manufacturer's ancient remote that included an artaflex 2.4Ghz module that (always disconnected) and was discontinued by the mfg. in 2018

Replace the brain-box with an Arduino WiFi R4 board, but keep the custom dual output power supply: MeanWell PD85-SP with dual output 35VDC 2.3A, 12VDC 0.25A

Digits are connected by a 10pin ribbon cable, of which only 4 pins are useful but they doubled them up for redundancy against broken lines? I guess?
Arduino --> Leftmost home digit (10's) --> right home digit (1's) --> Inning --> Dot driver board --> Guest left digit (10's) --> Rightmost Guest digit (1's)

Arduino power should NOT be sourced from the custom 35v/12v power supply as it doesn't seem to supply enough amperage for another load on either the 35v or 12v rail.

Note on the power supply...
Red wire is 35v rail - used to power the LED segments via a voltage regulator for each segment of the 7-segment digit
Brown wire is 12v rail - seems to only power a voltage regulator which drops to 5v to power the HC595 and ULN2803A chips
Black wire is GND

Graphics are included in a sprites file so there's only a single .png request (prevents upsetting Arduino's little webserver as opposed to 10+ image requests)

[END]
