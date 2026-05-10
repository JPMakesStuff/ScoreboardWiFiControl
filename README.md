# ScoreboardWiFiControl
Replacement for Varsity/All-Star/BSN/Sportable Baseball scoreboard controllers

Manufacturer's ancient and expensive remote was either a 5pin DIN connector that seemed to be RS-485 (but maybe not quite) or included the artaflex 2.4Ghz module (EOL 2018) which constantly disconnected; possibly because of a poorly designed heartbeat signal that can't ever be missed, or because it conflicted with every other 2.4Ghz WiFi network around. This remote drained batteries at an astounding rate; due to a design flaw where the power switch doesn't actually disconnect the battery.

Solution: Replace the manufacturer's control box with an [Arduino UNO R4 WiFi board](https://www.amazon.com/dp/B0C8V88Z9D), but keep the manufacturer dual output 35v/12v power supply that runs the LEDs.

Now anybody can use a **WiFi connected phone/tablet/laptop to control the scoreboard!**

Security should be accomplished by a dedicated wireless SSID just for the scoreboard(s) and for your coaches/announcers/trusted family member to connect.

Example webpage... (Make it an "app" icon using browser options menu, "Add to Home screen")<br/>
![Webpage_Screenshot_On_Phone](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Webpage_Screenshot_On_Phone.png?raw=true)

## Software for Arduino

For the [3314](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3314.jpg) baseball scoreboard<br/>
...use the Arduino code "[ard_scoreboard](https://github.com/JPMakesStuff/ScoreboardWiFiControl/tree/main/ard_scoreboard)"

For the [3320](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3320.jpg) baseball scoreboard<br/>
...use the Arduino code "[ard_bigscoreboard](https://github.com/JPMakesStuff/ScoreboardWiFiControl/tree/main/ard_bigscoreboard)"

Graphics are a sprites file so there's only a single .png request (prevents upsetting Arduino's little web server as opposed to 10+ image requests)  If you want to insert your own logo and touch button graphics you could edit the [example .png graphic](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/sprites_example.png), keeping the same placement and dimensions, then ask ChatGPT to create the arduino_images.h file from your .png file.

Your WiFi details need to be added in the arduino_secrets.h file.

To upload the sketch to your Arduino you need...
 - [Arduino IDE](https://www.arduino.cc/en/software/) (v2.3.8 as of today)
 - Install all the USB drivers when prompted
 - Tools > Board > Boards manager ... search "R4" ... Arduino UNO R4 Boards ... Install (v1.5.3 as of today)
 - Tools > Manage libraries ... search "graphics" ... ArduinoGraphics ... Install (v1.1.5 as of today)
 - Tools > Board > Arduino UNO R4 Boards > pick Arduino UNO R4
 - Tools > Port > pick COM # (the correct COM port number will be the one that shows up after plugging the Arduino into your computer)
 - Sketch > Upload

After applying power the Arduino's LED matrix briefly scrolls the last two octets of the IP address it got assigned. For example if your network DHCP server hands out 192.168.1.8 to the Arduino, the display would scroll 1.8
If this never happens then either your WiFi details are incorrect, out of range, you have some other network problem, or the Arduino is never fully booting.

## Hardware connections

The ribbon cable must be unplugged from the manufacturer's control box (usually located behind the large hatch below or behind the inning digit)
Example pinout...<br/>
![10pin connector](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/10pin_ribbon_cable.jpg?raw=true)

On the [cable I sell](https://www.etsy.com/listing/4490992543/arduino-to-baseball-scoreboard-control) pay attention to the colors plugged into the Arduino...<br/>
 - GND on Arduino is $\color{Green}{\textsf{Green}}$ wire</br>
 - Pin 6 on Arduino is $\color{Blue}{\textsf{Blue}}$ wire<br/>
 - Pin 5 on Arduino is $\color{Yellow}{\textsf{Yellow}}$ wire<br/>
 - Pin 4 on Arcuino is White or $\color{Purple}{\textsf{Purple}}$ wire<br/>
...see the [picture here](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Example_Arduino_Connection.jpg) of Arduino connection</br>

Valid ways of connecting to the ribbon cable without butchering it are...<br/>
 - [2X5 10P Dual Rows 2.54mm 0.1" Pitch Shrouded IDC Male Header](https://www.amazon.com/dp/B00B3PI02G)
 - [Male IDC Socket 10 Pin 2x5 Pin 2.54mm Pitch Box Header Connector](https://www.amazon.com/dp/B08T1YDG8M)
 - [Male to Male Breadboard Jumper Ribbon Cables](https://www.amazon.com/dp/B01EV70C78)
 - [Pre-made cable from me on Etsy](https://www.etsy.com/listing/4490992543/arduino-to-baseball-scoreboard-control)

I spent days reverse engineering this, drop me a line if it helped you, or if you've adapted it to another model scoreboard!  If you'd like assistance in repair of your scoreboard or implementation of this new WiFi control method please contact me.  As of today I sell the [Arduino interface cable](https://www.etsy.com/listing/4490992543/arduino-to-baseball-scoreboard-control) on my Etsy shop and hope to offer full hardware conversion kits in the future if the demand is there.

## WiFi technical info

In my implementation we are using the following harware...
 - TP-Link Omada firewall ER605v2 https://www.amazon.com/dp/B08QTXNWZ1
   ...also allows for content filtering to keep limited wireless bandwidth available for the GameChanger app
      (such as no YouTube, Hulu, Disney+ or adult sites)
 - TP-Link Omada wireless controller OC200 https://www.amazon.com/dp/B08SW3GFHH
   ...newer versions are also OK, but not nessesary for 8 or less WiFi APs
      this is required to put the scoreboard mounted APs into mesh mode
 - TP-Link EAP610 Outdoor AP https://www.amazon.com/dp/B0B231J81C
   ...one goes on the concessions building, one goes on the scoreboard
 - Universal Wireless mount 2pk https://www.amazon.com/dp/B0BCNZFNTD
   ...makes it easy to mount the APs so they face eachother
 - RasPi 4 model B with 4GB RAM https://www.amazon.com/dp/B07TD42S27
   ...this acts as a [Tailscale subnet router](https://tailscale.com/docs/features/subnet-routers) so I can control the scoreboards from anywhere, even when I'm home and not on the field WiFi

I intentionally decided to use an Arduino with WiFi as opposed to an Arduino with Ethernet; since each scoreboard has it's own AP mounted on it the signal is great and some level of lightning protection is achived by not having a direct Ethernet connection to the AP.

As currently implemented both the Arduino and your end-users should connect to the same wireless SSID, the Arduino should be given a DHCP reservation (so the IP doesn't change) on the network, and optionally a DNS A record pointing to that IP for an easy to remember name.  Arduino code could be modified so the Arduino broadcasts it's own SSID (AP mode) but this is considerably less reliable in terms of WiFi channel saturation and end-user connection simplicity, so I strongly suggest against this. If the wireless doesn't actually provide Internet, users will have to pick "stay connected" on their device, some devices don't handle this well and won't continue to route Internet out the cellular connection resulting in loss of calls, text messages, or other app data.

## More technical info

Scoreboard digits are connected internally by a 10pin ribbon cable, of which only 4 pins are useful (doubled up for redundancy against broken lines, I guess?) This cable connects to a HC595 IC (8-Bit Shift Register), which turns "on" a channel of the ULN2803A (NPN Darlington Transistor Array) which allows 35v power to flow through a voltage regulator to 2 sets of 13 LEDs in series, which makes up a segment of the digit, or a dot.

On the [3314](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3314.jpg) baseball scoreboard this cable is routed like this...<br/>
Arduino --> Left home digit (10's) --> right home digit (1's) --> Inning --> Dot driver board --> Left guest digit --> Right guest digit<br/>
...remove the ribbon cable from control box found on the front side, hatch below the inning digit
See [3314 wiring diagram](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3314-Wiring-Diagram.png)

On the [3320](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/3320.jpg) baseball scoreboard they routed this cable in what I suppose was efficient for saving on copper wire but full on silly for programming...<br/>
Arduino --> BallDigit, Inn1guest, Inn1home, Inn2home, Inn2guest, Inn3guest, Inn3home, Inn4home, Inn4guest, Inn5guest, Inn5home, Inn6home, Inn6guest, StrikeDigit, Inn7guest, Inn7home, Inn8home, Inn8guest, Inn9guest, Inn9home, LeftHomeDigit, RightHomeDigit, RightGuestDigit, LeftGuestDigit, OutDigit<br/>
...remove the ribbon cable from control box found on the front side, hatch above the 2nd inning guest score digit

Do NOT connect the ribbon cable to both your Arduino and the manufacture's control box at the same time, doing so will impede the signal to control any of the digits.

Arduino power should NOT be sourced from the custom 12v/35v power supply within the manufacture's control box as it doesn't seem to supply enough amperage for another load on the 12v rail, you will find that the Arduino randomly resets if many digits are 8 (all segments lit up)

I power the Arduino using a [5v 2amp USB power supply](https://www.amazon.com/dp/B07DCR29GN) feeding into either the USB-C port or the 5v pin on the Arduino.  Optionally you could also use a 7.5v or 9v power supply feeding into the barrel jack, realizing this is dropped down to 5v via Arduino's on-board voltage regulator.

120VAC can be obtained from the small hatch on the back of the scoreboard near the power entry point, or by using an [IEC splitter](https://www.amazon.com/dp/B0BGPXDYSM) before it goes into the manufacture's control box.

Note on the manufacture's power supply located inside the control box...<br/>
MeanWell PD85-SP with dual output 12VDC 0.25A + 35VDC 2.3A (which I can't find anywhere today, perhaps was custom built just for Varsity)<br/>
Someday this will die and I expect having to source a more common 36v supply and using the internal trim pot found on most MeanWell supply's to make it output 35v, having a seperate 12v power supply, then connecting both GNDs together.<br/>
 - Red wire is 35v rail - used to power the LED segments via several voltage regulators, one for each segment of the 7-segment digit<br/>
 - Brown wire is 12v rail - used to power a single voltage regulator on each digit which drops to 5v to power the HC595 and ULN2803A chips<br/>
 - Black wire is GND<br/>

In my case the scoreboards were all outdoors and all seemed to be getting wet inside, plus the fact that it's a big metal box means putting the Arduino inside the scoreboard isn't the best for WiFi connectivity, so I elected to move the power supply and control circuitry to a [weatherproof box](https://www.amazon.com/dp/B0BFPXDN8M) and split the 120VAC using [this terminal block](https://www.amazon.com/dp/B0DNJRQP32).  Each scoreboard has a wireless AP in a mesh configuration that connects them all back to the source of the WiFi at the concessions building.  We also use an Omada wireless controller and Omada firewall to allow GameChanger, but not streaming or YouTube or other high-bandwidth things.

See [example of my new control box](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Example_control_box.jpg)

## LED digit or dot testing aid

To test individual digit boards, or the "vane driver" board for (ball/strike/out) dots<br/>
Arduino --> 10pin ribbon cable --> only one digit at a time, not the entire scoreboard!<br/>
 - no other digits should be daisy-chained via additional ribbon cables connected to "output"<br/>
 - it's OK to keep the 3pin power cable connected to all, unless your trying to isolate a damaged/corroded/shorted digit or dot<br/>
 - **you MUST connect all GND together: the Arduino GND to the ribbon cable GND is then connected to the power supply GND via the digit**<br/>
 - use the Arduino code "[LED_tester](https://github.com/JPMakesStuff/ScoreboardWiFiControl/tree/main/LED_tester)"<br/>
 - modify the SSID section near the top to match your WiFi on the field, or your home WiFi, or your phone's hotspot when testing anywhere

See [example of test page](https://github.com/JPMakesStuff/ScoreboardWiFiControl/blob/main/Webpage_Screenshot_LED_tester.png)

## LED repair info

Digit<br/>
PCB - 1600129<br/>
PCA - 1500129<br/>
REV C<br/>

Dots<br/>
PCB - 1600131<br/>
PCA - 1500131<br/>
REV A<br/>

35v in → LM317 + 30 Ω → ~42 mA constant current → two parallel strings of 13 × 1.8v LEDs (voltage of drop ~23.4v) → ~20–21 mA per LED

LED size = T-1 3/4 5mm = DigiKey part 350-5218765F-ND, Mfg part 5218765F by Dialight

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
