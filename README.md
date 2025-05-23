# ScoreboardWiFiControl
Drop-in replacement for the Baseball 3314 Varsity/All-Star/BSN/Sportable scoreboard controller

Manufacturer's ancient remote was either a 5pin DIN connector that seemed to be RS-485 (but maybe not quite) or included the artaflex 2.4Ghz module (discontinued in 2018) which constantly disconnected; possibly because of a poorly designed heartbeat signal that can't ever be missed, or because it conflicted with every other 2.4Ghz WiFi network around.

Solution: Replace the brain-box with an [Arduino WiFi R4 board](https://store.arduino.cc/products/uno-r4-wifi), but keep the custom dual output 35v/12v power supply

Now anybody can use a WiFi connected phone/tablet/laptop to control the scoreboard!

Security should be accomplished by a dedicated SSID just for the scoreboard(s) and only allow actual coaches/announcers to connect.  As currently implemented both the Arduino and your end-users should connect to the same wireless SSID, the Arduino should be given a DHCP reservation (so the IP doesn't change) on the network, and optionally a DNS A record pointing to that IP.  Arduino code could be adapted to broadcast it's own SSID (AP mode) but this is considerably less reliable in terms of WiFi channel saturation and end-user connection simplicity, I strongly suggest against this.

Example webpage... (Make it an "app" icon using browser options menu, "Add to Home screen")

![Webpage_Screenshot_On_Phone](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Webpage_Screenshot_On_Phone.png?raw=true)

Digits are connected by a 10pin ribbon cable, of which only 4 pins are useful (doubled up for redundancy against broken lines, I guess?)

This cable connects to HC595 8-Bit Shift Registers on each digit.

Arduino --> Left home digit (10's) --> right home digit (1's) --> Inning --> Dot driver board --> Left guest digit --> Right guest digit

On a bigger scoreboard (3320) they routed this cable in what I suppose was efficent for saving on copper wire but full on silly for programming...

Arduino --> BallDigit, Inn1guest, Inn1home, Inn2home, Inn2guest, Inn3guest, Inn3home, Inn4home, Inn4guest, Inn5guest, Inn5home, Inn6home, Inn6guest, StrikeDigit, Inn7guest, Inn7home, Inn8home, Inn8guest, Inn9guest, Inn9home, LeftHomeDigit, RightHomeDigit, RightGuestDigit, LeftGuestDigit, OutDigit

Example pinout...

![10pin connector](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/10pin_ribbon_cable.jpg?raw=true)

Arduino power via buck converter down to 7.5 to 9 VDC should NOT be sourced from the custom 35v/12v power supply as it doesn't seem to supply enough amperage for another load on the 12v rail, you will find that the Arduino randomly resets if many digits are 8 (all segments lit up)

Note on the power supply: MeanWell PD85-SP with dual output 35VDC 2.3A, 12VDC 0.25A (which seemingly can't be purchased anywhere today, perhaps was custom built just for Varsity)
 - Red wire is 35v rail - used to power the LED segments via several voltage regulators, one for each segment of the 7-segment digit
 - Brown wire is 12v rail - used to power a single voltage regulator on each digit which drops to 5v to power the HC595 and ULN2803A chips
 - Black wire is GND

Graphics are included as a sprites file so there's only a single .png request (prevents upsetting Arduino's little web server as opposed to 10+ image requests)  If you want to insert your own logo and touch button graphics you could edit the example .png graphic, keeping the same placement and dimentions, then ask ChatGPT to create the arduino_images.h file from your own .png file.

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
