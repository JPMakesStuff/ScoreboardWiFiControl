# ScoreboardWiFiControl
Replacement for Varsity/All-Star/BSN/Sportable Baseball scoreboard controllers

Manufacturer's ancient/expensive remote was either a 5pin DIN connector that seemed to be RS-485 (but maybe not quite) or included the artaflex 2.4Ghz module (discontinued in 2018) which constantly disconnected; possibly because of a poorly designed heartbeat signal that can't ever be missed, or because it conflicted with every other 2.4Ghz WiFi network around.

Solution: Replace the manufacturer's control box with an [Arduino UNO R4 WiFi board](https://www.amazon.com/dp/B0C8V88Z9D), but keep the existing dual output 35v/12v power supply.

Now anybody can use a **WiFi connected phone/tablet/laptop to control the scoreboard!**

Security should be accomplished by a dedicated SSID just for the scoreboard(s) and for actual coaches/announcers to connect.

As currently implemented both the Arduino and your end-users should connect to the same wireless SSID, the Arduino should be given a DHCP reservation (so the IP doesn't change) on the network, and optionally a DNS A record pointing to that IP for an easy to remember name.  Arduino code could be adapted so the Arduino broadcasts it's own SSID (AP mode) but this is considerably less reliable in terms of WiFi channel saturation and end-user connection simplicity, so I strongly suggest against this.

Example webpage... (Make it an "app" icon using browser options menu, "Add to Home screen")<br/>
![Webpage_Screenshot_On_Phone](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Webpage_Screenshot_On_Phone.png?raw=true)

## Software for Arduino

On the [3314](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3314.jpg) baseball scoreboard this cable is routed like this...<br/>
Arduino --> Left home digit (10's) --> right home digit (1's) --> Inning --> Dot driver board --> Left guest digit --> Right guest digit<br/>
...remove the ribbon cable from control box found on the front side, hatch below the inning digit<br/>
...use the Arduino code "ard_scoreboard"

On the [3320](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3320.jpg) baseball scoreboard they routed this cable in what I suppose was efficient for saving on copper wire but full on silly for programming...<br/>
Arduino --> BallDigit, Inn1guest, Inn1home, Inn2home, Inn2guest, Inn3guest, Inn3home, Inn4home, Inn4guest, Inn5guest, Inn5home, Inn6home, Inn6guest, StrikeDigit, Inn7guest, Inn7home, Inn8home, Inn8guest, Inn9guest, Inn9home, LeftHomeDigit, RightHomeDigit, RightGuestDigit, LeftGuestDigit, OutDigit<br/>
...remove the ribbon cable from control box found on the front side, hatch above the 2nd inning guest score digit<br/>
...use the Arduino code "ard_bigscoreboard"

To test individual digit boards, or the vane driver board (for ball/strike/out dots)<br/>
Arduino --> only one digit at a time -- not the entire scoreboard!<br/>
...no other digits should be daisy-chained via additional ribbon cables connected to "output"<br/>
...use the Arduino code "LED_tester" [example of test page](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Webpage_Screenshot_LED_tester.png)

Graphics are included as a sprites file so there's only a single .png request (prevents upsetting Arduino's little web server as opposed to 10+ image requests)  If you want to insert your own logo and touch button graphics you could edit the [example .png graphic](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/sprites_example.png), keeping the same placement and dimensions, then ask ChatGPT to create the arduino_images.h file from your own .png file.

I spent days reverse engineering this, drop me a line if it helped you, or if you've adapted it to another model scoreboard!  If you'd like assistance in repair of your scoreboard or implementation of this new WiFi control method please contact me, hardware kits are available on my Etsy shop: [etsy.com/shop/JPElectron](https://www.etsy.com/shop/JPElectron)

## Hardware connections

Digits are connected by a 10pin ribbon cable, of which only 4 pins are useful (doubled up for redundancy against broken lines, I guess?)<br/>
This cable connects to a HC595 IC (8-Bit Shift Register), which turns "on" a channel of the ULN2803A (NPN Darlington Transistor Array) which allows 35v power to flow through a voltage regulator to 2 sets of 13 LEDs in series, which makes up a segment of the digit, or a dot.

Example pinout...<br/>
![10pin connector](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/10pin_ribbon_cable.jpg?raw=true)<br/>
You MUST also connect GND on the Arduino to GND on the ribbon cable

Valid ways of connecting to this cable without butchering it (so you could connect it back to the manufacture's control box) are...<br/>
 - [2X5 10P Dual Rows 2.54mm 0.1" Pitch Shrouded IDC Male Header](https://www.amazon.com/dp/B00B3PI02G)
 - [Male IDC Socket 10 Pin 2x5 Pin 2.54mm Pitch Box Header Connector](https://www.amazon.com/dp/B08T1YDG8M)
 - [Male to Male Breadboard Jumper Ribbon Cables](https://www.amazon.com/dp/B01EV70C78)

Do NOT connect the ribbon cable to both your Arduino and the manufacture's control box at the same time, doing so will impede the signal to control any of the digits.

Arduino power should NOT be sourced from the custom 35v/12v power supply as it doesn't seem to supply enough amperage for another load on the 12v rail, you will find that the Arduino randomly resets if many digits are 8 (all segments lit up)

In my example I chose to power the Arduino using a [5v 2amp USB power supply](https://www.amazon.com/dp/B07DCR29GN) feeding into (either the USB port or) the 5v pin on the Arduino.  Optionally you could also use a 7.5v or 9v power supply feeding into the barrel jack, realizing this is dropped down to 5v via Arduino's on-board voltage regulator.

120VAC can be obtained from the small hatch on the back of the scoreboard near the power entry point, or by using an [IEC splitter](https://www.amazon.com/dp/B0BGPXDYSM) before it goes into the manufacture's control box.

Note on the manufacture's power supply located inside the control box...<br/>
MeanWell PD85-SP with dual output 35VDC 2.3A, 12VDC 0.25A (which I can't find anywhere today, perhaps was custom built just for Varsity)<br/>
Someday this will die and I expect having to source a more common 36v supply and using the internal trim pot found on most MeanWell supply's to make it output 35v, having a seperate 12v power supply, then connecting both GNDs together.
 - Red wire is 35v rail - used to power the LED segments via several voltage regulators, one for each segment of the 7-segment digit
 - Brown wire is 12v rail - used to power a single voltage regulator on each digit which drops to 5v to power the HC595 and ULN2803A chips
 - Black wire is GND

In my case the scoreboards were all outdoors and all seemed to be getting wet inside, plus the fact that it's a big metal box means putting the Arduino inside the scoreboard isn't the best for WiFi connectivity, so I elected to move the power supply and control circuitry to a [weatherproof box](https://www.amazon.com/dp/B0BFPXDN8M) and split the 120VAC using [this terminal block](https://www.amazon.com/dp/B0DNJRQP32).  Each scoreboard has a [wireless AP](https://www.amazon.com/dp/B0B231J81C) in a mesh configuration that connects them all back to the source of the WiFi at the concessions building.  We also use an Omada wireless controller and Omada firewall to allow GameChanger, but not streaming or YouTube or other high-bandwidth things.

See [example of my new control box](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Example_control_box.jpg)

## LED repair info

Digit
PCB - 1600129
PCA - 1500129
REV C

Dots
PCB - 1600131
PCA - 1500131
REV A

35 V in → LM317 + 30 Ω → ~42 mA constant current → two parallel strings of 13 × 1.8 V LEDs (voltage of drop ~23.4v) → ~20–21 mA per LED

LED size = T-1 3/4 = 5mm

## Notes for cost comparison

~$263 for my control box (including conformal coating the PCBs, caulk, weatherproof box, screws, Arduino, 5v power supply, and wireless AP)

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
