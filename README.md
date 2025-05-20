# ScoreboardWiFiControl
Drop-in replacement for the Baseball 3314 Varsity/All-Star/BSN/Sportable scoreboard controller

Manufacturer's ancient remote was either a 5pin DIN connector that seemed to be RS-485 (but maybe not quite) or included the artaflex 2.4Ghz module (discontinued in 2018) which constantly disconnected; possibly because of a poorly designed heartbeat signal that can't ever be missed, or because it conflicted with every other 2.4Ghz WiFi network around.

Solution: Replace the brain-box with an Arduino WiFi R4 board, but keep the custom dual output 35v/12v power supply

Now anybody can use their phone/tablet to control the scoreboard!

Security should be accomplished by a dedicated SSID just for the scoreboard(s) and only allow actual coaches/announcers to connect.

![Webpage_Screenshot](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Webpage_Screenshot.png?raw=true)

Digits are connected by a 10pin ribbon cable, of which only 4 pins are useful but they doubled them up for redundancy against broken lines? I guess?
Arduino --> Leftmost home digit (10's) --> right home digit (1's) --> Inning --> Dot driver board --> Guest left digit (10's) --> Rightmost Guest digit (1's)

![10pin connector](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/10pin_ribbon_cable.jpg?raw=true)

This cable connects to HC595 8-Bit Shift Registers on each digit.

Arduino power via buck converter down to 7.5 to 9 VDC should NOT be sourced from the custom 35v/12v power supply as it doesn't seem to supply enough amperage for another load on the 12v rail, you will find that the Arduino randomly resets if many digits are 8 (all segments lit up)

Note on the power supply: MeanWell PD85-SP with dual output 35VDC 2.3A, 12VDC 0.25A (which seemingly can't be purchased anywhere today, perhaps was custom built just for Varsity)
 - Red wire is 35v rail - used to power the LED segments via several voltage regulators, one for each segment of the 7-segment digit
 - Brown wire is 12v rail - used to power a single voltage regulator on each digit which drops to 5v to power the HC595 and ULN2803A chips
 - Black wire is GND

Graphics are included as a sprites file so there's only a single .png request (prevents upsetting Arduino's little web server as opposed to 10+ image requests)

I spent days reverse engineering this, drop me a line if it helped you, or if you've adapted it to another model scoreboard!

Notes for cost comparison...

https://scoreboard-parts.com/1006647-Semtech-Sportable-Varsity-Scoreboard-Driver-101MOD2.html
$975 | SKU# 1006647-Semtech

https://scoreboard-parts.com/1006649-SEMTECH-Sportable-Varsity-Wireless-Controller.html
$950 | SKU# 1006649-SEMTECH

https://scoreboard-parts.com/101MOD2-Sportable-Varsity-Scoreboard-Driver.html
$1,562.95 | SKU# 101MOD2

https://scoreboard-parts.com/1001LCDW-Sportable-Varsity-Wireless-Controller.HTML
$1,824.95 | SKU# 1001LCDW

...I'm not paying that for something that disconnects and resets the score 95% of the time.

[End of Line]
