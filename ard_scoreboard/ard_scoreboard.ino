#include "WiFiS3.h"
#include "arduino_secrets.h"

#include "arduino_images.h"

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

char ssid[] = SECRET_SSID; // do not update SSID and PASS here, put it in arduino_secrets.h
char pass[] = SECRET_PASS;
int keyIndex = 0;

// store the software version
char swver[] = "25.05.01r23";

// declair pins
int dataPin = 4;	// Data pin of 74HC595 is connected to Digital pin 4 (purple wire to digit ribbon cable)
int latchPin = 5;	// Latch pin of 74HC595 is connected to Digital pin 5 (yellow wire to digit ribbon cable)
int clockPin = 6;	// Clock pin of 74HC595 is connected to Digital pin 6 (blue wire to digit ribbon cable)
    // dont forget GND connection (green wire to digit ribbon cable)

// store value for each digit, but initially set to off = 10 (otherwise send actual number you want displayed)
int u = 10;  //u is far left digit 1 (first) in chain (home 10s position)
int v = 0;
int w = 1;   //w is INNING
int x = 10;  //x is vane driver for DOTs (do not use this variable use dotX instead)
int y = 10;
int z = 0;   //z is far right digit 6 (last) in chain (guest 1s position)

// store value for each dot, but initially set to off = 0 (otherwise send 1 = on)
int dotA = 0;
int dotB = 0;
int dotC = 0;
int dotD = 0;
int dotE = 0;
int dotF = 0;
int dotG = 0;

// store values globally for use when restoring
int oldU = u, oldV = v, oldW = w, oldY = y, oldZ = z;
int oldDotA = dotA, oldDotB = dotB, oldDotC = dotC, oldDotD = dotD, oldDotE = dotE, oldDotF = dotF, oldDotG = dotG;

// store values globally for actual score
int homeScore = 0;    // 0-99 new games should start at score 0
int innScore = 1;     // 0-99 new games should start at inning 1
int guestScore = 0;   // 0-99 new games should start at score 0
int ballCount = 0;    // 0–3 (for dots A–C)
int strikeCount = 0;  // 0–2 (for dots D–E)
int outCount = 0;     // 0–2 (for dots F–G)

// to control each 7-segment digit
byte num[] = {
  B00111111, // Zero
  B00000110, // One
  B01011011, // Two
  B01001111, // Three
  B01100110, // Four
  B01101101, // Five
  B01111101, // Six
  B00000111, // Seven
  B01111111, // Eight
  B01101111, // Nine
  B00000000, // 10 = ALL SEGMENTS (or DOTs) OFF
};
// x        segment DP (not used so leave 0 for off)
//  x       segment position G on (also Out 2)
//   x      segment position F on (also Out 1)
//    x     segment position E on (also Strike 2)
//     x    segment position D on (also Strike 1)
//      x   segment position C on (also Ball 3)
//       x  segment position B on (also Ball 2)
//        x segment position A on (also Ball 1)

// store if new serial data should be sent to the HC595 shift registers or not
int flag_update = false;

byte mac[6];  // the MAC address of WiFi module

int status = WL_IDLE_STATUS;
WiFiServer server(80);


void setup() 
{

  // set all the pins of 74HC595 as OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);

  //Serial.begin(9600);      // initialize serial communication

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    //Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // check for the firmware upgrade:
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    //Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    //Serial.print("Attempting to connect to Network named: ");
    //Serial.println(ssid);  // print the network name (SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 15 seconds for connection:
    delay(15000);
  }

  server.begin();     // start the web server on port 80

  IPAddress localIP = WiFi.localIP();
  String ipStr = "." + String(localIP[2]) + "." + String(localIP[3]);  // show only 3rd and 4th octets (index 2 and 3)

  matrix.begin();
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(50);
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(ipStr);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();

  //quickly blink ball 1 LED to let user know we're done booting
  dotA = 1;  // ball 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(200);
  dotA = 0;  // ball 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(200);
  dotA = 1;  // ball 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(200);
  dotA = 0;  // ball 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(200);

  flag_update = false;
}


void loop() 
{

WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                           // if you get a client
    String request = "";
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.length() == 1 && line[0] == '\r') break;
        if (request == "") request = line;
      }

        // HTTP headers always start with a response code
        if (request.indexOf("GET /favicon.ico") >= 0) {
          client.println("HTTP/1.1 404 Not Found");   // here is the error response code
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("404 Not Found");
          client.stop();  // close connection
          return;         // stop processing this loop()
        }
        else {
          client.println("HTTP/1.1 200 OK");          // here is a normal response code
        }

        // image first since header is different
        // using image sprite to save memory
        if (request.indexOf("GET /sprites.png") >= 0) {
          client.println("Content-Type: image/png");
          client.println("Content-Length: " + String(sprites_png_len));
          client.println("Cache-Control: max-age=86400");  // browser cache for 1 day
          client.println("Connection: close");
          client.println();
          client.write(sprites_png, sprites_png_len);
        }
        // otherwise its the normal text/html content below
        else {
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();
        }

        // otherwise the content of the HTTP response follows the header

        // check if the client request was "GET /placeholder" used in quick remote and main page iframes
        if (request.indexOf("GET /placeholder") >= 0) {
        }

        // HOME SCORE UP - request may come from either remote page
        else if (request.indexOf("GET /hup") >= 0) {
          homeScore = (homeScore + 1) % 100;
          v = homeScore % 10;
          u = (homeScore < 10) ? 10 : homeScore / 10;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(homeScore);
          }
        }

        // HOME SCORE DOWN - request may come from either remote page
        else if (request.indexOf("GET /hdn") >= 0) {
          homeScore = (homeScore == 0) ? 99 : homeScore - 1;
          v = homeScore % 10;
          u = (homeScore < 10) ? 10 : homeScore / 10;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(homeScore);
          }
        }

        // INNING UP - request may come from either remote page
        else if (request.indexOf("GET /iup") >= 0) {
          innScore = (innScore == 9) ? 1 : innScore + 1;  // wrap from 9 -> 1
          w = innScore;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(innScore);
          }
        }

        // INNING DOWN - request may come from either remote page
        else if (request.indexOf("GET /idn") >= 0) {
          innScore = (innScore == 1) ? 9 : innScore - 1;
          w = innScore;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(innScore);
          }
        }

        // GUEST SCORE UP - request may come from either remote page
        else if (request.indexOf("GET /gup") >= 0) {
          guestScore = (guestScore + 1) % 100;
          z = guestScore % 10;
          y = (guestScore < 10) ? 10 : guestScore / 10;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(guestScore);
          }
        }

        // GUEST SCORE DOWN - request may come from either remote page
        else if (request.indexOf("GET /gdn") >= 0) {
          guestScore = (guestScore == 0) ? 99 : guestScore - 1;
          z = guestScore % 10;
          y = (guestScore < 10) ? 10 : guestScore / 10;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(guestScore);
          }
        }

        // BALLs - request may come from either remote page
        else if (request.indexOf("GET /ball") >= 0) {
          ballCount = (ballCount + 1) % 4;
          dotA = (ballCount >= 1) ? 1 : 0;
          dotB = (ballCount >= 2) ? 1 : 0;
          dotC = (ballCount >= 3) ? 1 : 0;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(ballCount);
          }
        }

        // STRIKEs - request may come from either remote page
        else if (request.indexOf("GET /strike") >= 0) {
          strikeCount = (strikeCount + 1) % 3;
          dotD = (strikeCount >= 1) ? 1 : 0;
          dotE = (strikeCount >= 2) ? 1 : 0;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(strikeCount);
          }
        }

        // OUTs - request may come from either remote page
        else if (request.indexOf("GET /out") >= 0) {
          outCount = (outCount + 1) % 3;
          dotF = (outCount >= 1) ? 1 : 0;
          dotG = (outCount >= 2) ? 1 : 0;
          flag_update = false;
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(outCount);
          }
        }

        // check if the client request was "GET /new"
        else if (request.indexOf("GET /new") >= 0) {
          u = 10; v = 0;         // Home score = 00 (10 = blank tens digit)
          w = 1;                 // Inning = 1
          y = 10; z = 0;         // Guest score = 00 (10 = blank tens digit)
          dotA = dotB = dotC = dotD = dotE = dotF = dotG = 0; // All dots off
          homeScore = 0;
          innScore = 1;
          guestScore = 0;
          ballCount = 0;
          strikeCount = 0;
          outCount = 0;
          flag_update = false;
        }

        // check if the client request was "GET /blank"
        else if (request.indexOf("GET /blank") >= 0) {
          // Store the current score and dot state
          oldU = u, oldV = v, oldW = w, oldY = y, oldZ = z;
          oldDotA = dotA, oldDotB = dotB, oldDotC = dotC, oldDotD = dotD, oldDotE = dotE, oldDotF = dotF, oldDotG = dotG;
          // Set all digits and dots to blank
          u = v = w = y = z = 10;
          dotA = dotB = dotC = dotD = dotE = dotF = dotG = 0;
          flag_update = false;
        }

        // check if the client request was "GET /restore"
        else if (request.indexOf("GET /restore") >= 0) {
          // Restore previous values
          u = oldU; v = oldV; w = oldW; y = oldY; z = oldZ;
          dotA = oldDotA; dotB = oldDotB; dotC = oldDotC; dotD = oldDotD;
          dotE = oldDotE; dotF = oldDotF; dotG = oldDotG;
          flag_update = false;
        }

        // read the home score API FUNCTION check if the client request was "GET /rhome"
        else if (request.indexOf("GET /rhome") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }
          client.print(homeScore);
        }

        // read the home score API FUNCTION check if the client request was "GET /rinning"
        else if (request.indexOf("GET /rinning") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }
          client.print(innScore);
        }

        // read the home score API FUNCTION check if the client request was "GET /rguest"
        else if (request.indexOf("GET /rguest") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }
          client.print(guestScore);
        }

        // read the home score API FUNCTION check if the client request was "GET /rball"
        else if (request.indexOf("GET /rball") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }
          client.print(ballCount);
        }

        // read the home score API FUNCTION check if the client request was "GET /rstrike"
        else if (request.indexOf("GET /rstrike") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }
          client.print(strikeCount);
        }

        // read the home score API FUNCTION check if the client request was "GET /rout"
        else if (request.indexOf("GET /rout") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }
          client.print(outCount);
        }

        // set the home score API FUNCTION check if the client request was "GET /shome?##"
        else if (request.indexOf("GET /shome?") >= 0) {
          int start = request.indexOf("/shome?") + 7;  // move to first char after ?
          String numStr = "";
          // Collect up to 2 digits only, ignore the rest
          while (numStr.length() < 2 && isDigit(request.charAt(start))) {
            numStr += request.charAt(start++);
          }
          int newScore = numStr.toInt();
          if (numStr.length() > 0 && newScore >= 0 && newScore <= 99) {
            homeScore = newScore;
            v = homeScore % 10;
            u = (homeScore < 10) ? 10 : homeScore / 10;
          flag_update = false;
            client.println("OK");
          } else {
            client.println("ERR");
          }
        }

        // set the inning API FUNCTION check if the client request was "GET /sinn?##"
        else if (request.indexOf("GET /sinn?") >= 0) {
          int start = request.indexOf("/sinn?") + 6;
          String numStr = "";
          while (numStr.length() < 2 && isDigit(request.charAt(start))) {
            numStr += request.charAt(start++);
          }
          int newInning = numStr.toInt();
          if (numStr.length() > 0 && newInning >= 1 && newInning <= 9) {
            innScore = newInning;
            w = innScore;
            flag_update = false;
            client.println("OK");
          } else {
            client.println("ERR");
          }
        }

        // set the guest score API FUNCTION check if the client request was "GET /sguest?##"
        else if (request.indexOf("GET /sguest?") >= 0) {
          int start = request.indexOf("/sguest?") + 8;
          String numStr = "";
          while (numStr.length() < 2 && isDigit(request.charAt(start))) {
            numStr += request.charAt(start++);
          }
          int newScore = numStr.toInt();
          if (numStr.length() > 0 && newScore >= 0 && newScore <= 99) {
            guestScore = newScore;
            z = guestScore % 10;
            y = (guestScore < 10) ? 10 : guestScore / 10;
            flag_update = false;
            client.println("OK");
          } else {
            client.println("ERR");
          }
        }

        // set the ball API FUNCTION check if the client request was "GET /sball?#"
        else if (request.indexOf("GET /sball?") >= 0) {
          int start = request.indexOf("/sball?") + 7;
          char ch = request.charAt(start);

          if (isDigit(ch)) {
            int newCount = ch - '0';
            if (newCount >= 0 && newCount <= 3 && !isDigit(request.charAt(start + 1))) {
              ballCount = newCount;
              dotA = (ballCount >= 1) ? 1 : 0;
              dotB = (ballCount >= 2) ? 1 : 0;
              dotC = (ballCount >= 3) ? 1 : 0;
              flag_update = false;
              client.println("OK");
            } else {
              client.println("ERR");
            }
          } else {
            client.println("ERR");
          }
        }

        // set the strike API FUNCTION check if the client request was "GET /sstrike?#"
        else if (request.indexOf("GET /sstrike?") >= 0) {
          int start = request.indexOf("/sstrike?") + 9;
          char ch = request.charAt(start);

          if (isDigit(ch)) {
            int newCount = ch - '0';
            if (newCount >= 0 && newCount <= 2 && !isDigit(request.charAt(start + 1))) {
              strikeCount = newCount;
              dotD = (strikeCount >= 1) ? 1 : 0;
              dotE = (strikeCount >= 2) ? 1 : 0;
              flag_update = false;
              client.println("OK");
            } else {
              client.println("ERR");
            }
          } else {
            client.println("ERR");
          }
        }

        // set the out API FUNCTION check if the client request was "GET /sout?#"
        else if (request.indexOf("GET /sout?") >= 0) {
          int start = request.indexOf("/sout?") + 6;
          char ch = request.charAt(start);

          if (isDigit(ch)) {
            int newCount = ch - '0';
            if (newCount >= 0 && newCount <= 2 && !isDigit(request.charAt(start + 1))) {
              outCount = newCount;
              dotF = (outCount >= 1) ? 1 : 0;
              dotG = (outCount >= 2) ? 1 : 0;
              flag_update = false;
              client.println("OK");
            } else {
              client.println("ERR");
            }
          } else {
            client.println("ERR");
          }
        }

        // check if the client request was "GET /sysinfo"
        else if (request.indexOf("GET /sysinfo") >= 0) {
          client.print("<html>");
          client.print("<head>");
          client.print("<meta http-equiv=\"Content-Language\" content=\"en-us\">");
          client.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">");
          client.print("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no;\">");
          client.print("<title>Scoreboard Sys Info</title>");

          client.print("<style>");
          client.print(".icon {");
          client.print("height: 60px;");
          client.print("background-image: url('/sprites.png');");
          client.print("background-repeat: no-repeat;");
          client.print("display: inline-block;");
          client.print("}");

          client.print(".icon.logo {");
          client.print("width: 300px;");
          client.print("background-position: -180px 0;");
          client.print("}");
          client.print("</style>");

          client.print("</head>");
          client.print("<body>");
          client.print("<font face=\"Verdana\" style=\"font-size: 10pt\">");
          client.print("<div class=\"icon logo\"></div><br><br>");

          client.print("<a href=\"/\">Go back to main menu</a>");

          client.print("<br><br>");

          long rssi = WiFi.RSSI();
          delay(30);
          client.print("WiFi RSSI: ");
          client.print(rssi);
          client.print(" dBm  <br>&nbsp;&nbsp;(-70 excellent, -81 to -90 poor, -91 terrible)");
          client.print("<br><br>");

          IPAddress localIP = WiFi.localIP();
          String lipStr = localIP.toString();
          delay(30);
          client.print("Webserver IP: ");
          client.print(lipStr);
          client.print("&nbsp;&nbsp;");

          WiFi.macAddress(mac);
          char macStr[18];
          sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
          delay(30);
          client.print("MAC: ");
          client.print(macStr);
          client.print("<br><br>");

          IPAddress clientIP = client.remoteIP();
          String cipStr = clientIP.toString();
          delay(30);
          client.print("Your client IP: ");
          client.print(cipStr);
          client.print("<br><br>");

          String uptimeStr = formatUptime();
          client.print("System Uptime: ");
          client.print(uptimeStr);
          client.print("<br><br>");

          client.print("<a target=\"hidden\" href=\"/testdigits\">Test Digits</a><br>");
          client.print("&nbsp;&nbsp;&nbsp;Shows 8 left to right, then all dots, then shows data again");

          client.print("<br><br>");

          client.print("API endpoints...<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;<a href=\"/rhome\">/rhome</a> - <u>r</u>eturns homeScore as integer<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;<a href=\"/rhome?show=yes\">/rhome?show=yes</a> - <u>r</u>eturns homeScore as an HTML page<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;<a href=\"/shome?11\">/shome?11</a> - <u>s</u>ets the homeScore to 11<br><br>");
          client.print("other endpoint names...<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;inn, guest, ball, strike, out<br>");

          client.print("<br><br>");

          client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
          client.print("Scoreboard web interface v");
          client.print(swver);
          client.print("<br>");
          client.print("&copy; 2025 JPMakesStuff<br>");
          client.print("</font>");

          client.print("<iframe name=\"hidden\" width=\"50\" height=\"20\" align=\"top\" src=\"/placeholder\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");

          client.print("</font>");
          client.print("</body>");
          client.print("</html>");
        }

        // check if the client request was "GET /testdigits"
        else if (request.indexOf("GET /testdigits") >= 0) {
          // Store the current score and dot state
          oldU = u, oldV = v, oldW = w, oldY = y, oldZ = z;
          oldDotA = dotA, oldDotB = dotB, oldDotC = dotC, oldDotD = dotD, oldDotE = dotE, oldDotF = dotF, oldDotG = dotG;

          digit_test();

          // Restore previous values
          u = oldU; v = oldV; w = oldW; y = oldY; z = oldZ;
          dotA = oldDotA; dotB = oldDotB; dotC = oldDotC; dotD = oldDotD;
          dotE = oldDotE; dotF = oldDotF; dotG = oldDotG;
          delay(900);
          flag_update = false;
         }

        else if (request.indexOf("GET /full") >= 0) {
          //show the full remote version
          client.print("<html>");
          client.print("<head>");
          client.print("<meta http-equiv=\"Content-Language\" content=\"en-us\">");
          client.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">");
          client.print("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no;\">");
          client.print("<title>Scoreboard Full Remote</title>");

          client.print("<style>");
          client.print(".icon {");
          client.print("height: 60px;");
          client.print("background-image: url('/sprites.png');");
          client.print("background-repeat: no-repeat;");
          client.print("display: inline-block;");
          client.print("}");

          client.print(".icon.noarr {");
          client.print("width: 60px;");
          client.print("background-position: 0px 0;");
          client.print("}");

          client.print(".icon.dnarr {");
          client.print("width: 60px;");
          client.print("background-position: -60px 0;");
          client.print("}");

          client.print(".icon.uparr {");
          client.print("width: 60px;");
          client.print("background-position: -120px 0;");
          client.print("}");

          client.print(".icon.logo {");
          client.print("width: 300px;");
          client.print("background-position: -180px 0;");
          client.print("}");
          client.print("</style>");

          client.print("<style>");
          client.print("td {");
          client.print("text-align: center;");
          client.print("vertical-align: middle;");
          client.print("font-size: 32pt;");
          client.print("font-family: Verdana;");
          client.print("}");
          client.print("td a {");
          client.print("text-decoration: none;");
          client.print("}");
          client.print(".larger {");
          client.print("font-size: 52pt;");
          client.print("}");
          client.print("</style>");

          client.print("</head>");
          client.print("<body>");
          client.print("<font face=\"Verdana\" style=\"font-size: 10pt\">");
          client.print("<div class=\"icon logo\"></div><br>");

          client.print("<table>");
          client.print("<tbody>");
          client.print("<tr>");
          client.print("<td style=\"width: 100px\">Home</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\">Inn</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\">Guest</td>");
          client.print("</tr>");
          client.print("<tr>");
          client.print("<td class=\"larger\"><b><a target=\"home\" href=\"/hup?show=yes\"><div class=\"icon uparr\"></div></a><br>");
          client.print("<iframe name=\"home\" width=\"80\" height=\"60\" align=\"top\" src=\"/rhome?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");
          client.print("<br><a target=\"home\" href=\"/hdn?show=yes\"><div class=\"icon dnarr\"></div></a></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td class=\"larger\"><b><a target=\"inning\" href=\"/iup?show=yes\"><div class=\"icon uparr\"></div></a><br>");
          client.print("<iframe name=\"inning\" width=\"80\" height=\"60\" align=\"top\" src=\"/rinning?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");
          client.print("<br><a target=\"inning\" href=\"/idn?show=yes\"><div class=\"icon dnarr\"></div></a></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td class=\"larger\"><b><a target=\"guest\" href=\"/gup?show=yes\"><div class=\"icon uparr\"></div></a><br>");
          client.print("<iframe name=\"guest\" width=\"80\" height=\"60\" align=\"top\" src=\"/rguest?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");
          client.print("<br><a target=\"guest\" href=\"/gdn?show=yes\"><div class=\"icon dnarr\"></div></a></td>");
          client.print("</tr>");
          client.print("<tr>");
          client.print("<td style=\"width: 100px\"><br>Ball</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\"><br>Strike</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\"><br>Out</td>");
          client.print("</tr>");
          client.print("<tr>");
          client.print("<td><a target=\"ball\" href=\"/ball?show=yes\"><div class=\"icon noarr\"></div></a><br><iframe name=\"ball\" width=\"80\" height=\"60\" align=\"top\" src=\"/rball?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td><a target=\"strike\" href=\"/strike?show=yes\"><div class=\"icon noarr\"></div></a><br><iframe name=\"strike\" width=\"80\" height=\"60\" align=\"top\" src=\"/rstrike?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td><a target=\"out\" href=\"/out?show=yes\"><div class=\"icon noarr\"></div></a><br><iframe name=\"out\" width=\"80\" height=\"60\" align=\"top\" src=\"/rout?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe></td>");
          client.print("</tr>");
          client.print("</tbody>");
          client.print("</table><br>");

          client.print("<br><br><br>");

          client.print("&nbsp;&nbsp;&nbsp;<a target=\"hidden\" href=\"/blank\">OFF, but retain data</a>&nbsp;&nbsp;&nbsp; |&nbsp;&nbsp;&nbsp; ");
          client.print("<a target=\"hidden\" href=\"/restore\">ON, show data again</a><br><br><br>");
          client.print("&nbsp;&nbsp;&nbsp;Start a new game from the <a href=\"/\">main menu</a><br><br><br>");

          client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
          client.print("Scoreboard web interface v");
          client.print(swver);
          client.print("<br>");
          client.print("&copy; 2025 JPMakesStuff<br>");
          client.print("</font>");

          client.print("<iframe name=\"hidden\" width=\"50\" height=\"20\" align=\"top\" src=\"/placeholder\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");

          client.print("</font>");
          client.print("</body>");
          client.print("</html>");
        }

        else if (request.indexOf("GET /quick") >= 0) {
          //show the quick remote version
          client.print("<html>");
          client.print("<head>");
          client.print("<meta http-equiv=\"Content-Language\" content=\"en-us\">");
          client.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">");
          client.print("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no;\">");
          client.print("<title>Scoreboard Quick Remote</title>");

          client.print("<style>");
          client.print(".icon {");
          client.print("height: 60px;");
          client.print("background-image: url('/sprites.png');");
          client.print("background-repeat: no-repeat;");
          client.print("display: inline-block;");
          client.print("}");

          client.print(".icon.noarr {");
          client.print("width: 60px;");
          client.print("background-position: 0px 0;");
          client.print("}");

          client.print(".icon.dnarr {");
          client.print("width: 60px;");
          client.print("background-position: -60px 0;");
          client.print("}");

          client.print(".icon.uparr {");
          client.print("width: 60px;");
          client.print("background-position: -120px 0;");
          client.print("}");

          client.print(".icon.logo {");
          client.print("width: 300px;");
          client.print("background-position: -180px 0;");
          client.print("}");
          client.print("</style>");

          client.print("<style>");
          client.print("td {");
          client.print("text-align: center;");
          client.print("vertical-align: middle;");
          client.print("font-size: 32pt;");
          client.print("font-family: Verdana;");
          client.print("}");
          client.print("td a {");
          client.print("text-decoration: none;");
          client.print("}");
          client.print(".larger {");
          client.print("font-size: 52pt;");
          client.print("}");
          client.print("</style>");

          client.print("</head>");
          client.print("<body>");
          client.print("<font face=\"Verdana\" style=\"font-size: 10pt\">");
          client.print("<div class=\"icon logo\"></div><br>");

          client.print("<table>");
          client.print("<tbody>");
          client.print("<tr>");
          client.print("<td style=\"width: 100px\">Home</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\">Inn</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\">Guest</td>");
          client.print("</tr>");
          client.print("<tr>");
          client.print("<td class=\"larger\"><b><a target=\"hidden\" href=\"/hup\"><div class=\"icon uparr\"></div></a><br><br><a target=\"hidden\" href=\"/hdn\"><div class=\"icon dnarr\"></div></a></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td class=\"larger\"><b><a target=\"hidden\" href=\"/iup\"><div class=\"icon uparr\"></div></a><br><br><a target=\"hidden\" href=\"/idn\"><div class=\"icon dnarr\"></div></a></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td class=\"larger\"><b><a target=\"hidden\" href=\"/gup\"><div class=\"icon uparr\"></div></a><br><br><a target=\"hidden\" href=\"/gdn\"><div class=\"icon dnarr\"></div></a></td>");
          client.print("</tr>");
          client.print("<tr>");
          client.print("<td style=\"width: 100px\"><br>Ball</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\"><br>Strike</td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td style=\"width: 100px\"><br>Out</td>");
          client.print("</tr>");
          client.print("<tr>");
          client.print("<td><a target=\"hidden\" href=\"/ball\"><div class=\"icon noarr\"></div></a><br><br></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td><a target=\"hidden\" href=\"/strike\"><div class=\"icon noarr\"></div></a><br><br></td>");
          client.print("<td>&nbsp;</td>");
          client.print("<td><a target=\"hidden\" href=\"/out\"><div class=\"icon noarr\"></div></a><br><br></td>");
          client.print("</tr>");
          client.print("</tbody>");
          client.print("</table><br>");

          client.print("<br><br>");

          client.print("&nbsp;&nbsp;&nbsp;<a target=\"hidden\" href=\"/blank\">OFF, but retain data</a>&nbsp;&nbsp;&nbsp; |&nbsp;&nbsp;&nbsp; ");
          client.print("<a target=\"hidden\" href=\"/restore\">ON, show data again</a><br><br><br>");
          client.print("&nbsp;&nbsp;&nbsp;Start a new game from the <a href=\"/\">main menu</a><br><br><br>");

          client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
          client.print("Scoreboard web interface v");
          client.print(swver);
          client.print("<br>");
          client.print("&copy; 2025 JPMakesStuff<br>");
          client.print("</font>");

          client.print("<iframe name=\"hidden\" width=\"50\" height=\"20\" align=\"top\" src=\"/placeholder\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");

          client.print("</font>");
          client.print("</body>");
          client.print("</html>");
        }

        else {
          // show the main webpage
          client.print("<html>");
          client.print("<head>");
          client.print("<meta http-equiv=\"Content-Language\" content=\"en-us\">");
          client.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">");
          client.print("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no;\">");
          client.print("<title>Scoreboard</title>");

          client.print("<style>");
          client.print(".icon {");
          client.print("height: 60px;");
          client.print("background-image: url('/sprites.png');");
          client.print("background-repeat: no-repeat;");
          client.print("display: inline-block;");
          client.print("}");

          client.print(".icon.noarr {");
          client.print("width: 60px;");
          client.print("background-position: 0px 0;");
          client.print("}");

          client.print(".icon.dnarr {");
          client.print("width: 60px;");
          client.print("background-position: -60px 0;");
          client.print("}");

          client.print(".icon.uparr {");
          client.print("width: 60px;");
          client.print("background-position: -120px 0;");
          client.print("}");

          client.print(".icon.logo {");
          client.print("width: 300px;");
          client.print("background-position: -180px 0;");
          client.print("}");
          client.print("</style>");

          client.print("</head>");
          client.print("<body>");
          client.print("<font face=\"Verdana\" style=\"font-size: 14pt\">");
          client.print("<div class=\"icon logo\"></div><br>");

          client.print("Which remote would you like to use?<br><br><br><br>");

          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/quick\">Quick remote</a><br><br><br><br>");

          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/full\">Full remote</a><br><br><br><br><br><br><br>");

          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a target=\"hidden\" href=\"/new\">New game</a><br>");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(erases data)<br><br><br><br><br><br><br>");

          client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
          client.print("<a href=\"/sysinfo\">Show system info</a><br>");
          client.print("Scoreboard web interface v");
          client.print(swver);
          client.print("<br>");
          client.print("&copy; 2025 JPMakesStuff<br>");
          client.print("</font>");

          client.print("<iframe name=\"hidden\" width=\"50\" height=\"20\" align=\"top\" src=\"/placeholder\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");

          client.print("</font>");
          client.print("</body>");
          client.print("</html>");
        }

        // The HTTP response ends with another blank line:
        client.println();
      
    // close the connection:
    client.stop();
  }

if (!flag_update) digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);

}


void digit_update(int u, int v, int w, int x, int y, int z, int dotA, int dotB, int dotC, int dotD, int dotE, int dotF, int dotG)
{
  byte dotFlags =   // pack into a byte: B0ABCDEFG
      (dotA << 0) |
      (dotB << 1) |
      (dotC << 2) |
      (dotD << 3) |
      (dotE << 4) |
      (dotF << 5) |
      (dotG << 6);

   digitalWrite(latchPin, LOW);  //about to send this to the scoreboard

  //6th digit is GUEST right
  shiftOut(dataPin, clockPin, MSBFIRST, num[z]); 
  //delay(50);

  //5th digit is GUEST left (10s position)
  shiftOut(dataPin, clockPin, MSBFIRST, num[y]); 
  //delay(50);

  //4th digit vane driver for DOTS
  shiftOut(dataPin, clockPin, MSBFIRST, dotFlags); 
  //delay(50);

  //3rd digit INNING
  shiftOut(dataPin, clockPin, MSBFIRST, num[w]);
  //delay(50);

  //2nd digit HOME right
  shiftOut(dataPin, clockPin, MSBFIRST, num[v]); 
  //delay(50);

  //1st digit HOME left (10s position)
  shiftOut(dataPin, clockPin, MSBFIRST, num[u]); 
  //delay(50);

  digitalWrite(latchPin, HIGH);  //done sending

  flag_update = true;  //mark that we already did it
}


void digit_test()
{
  // BEGIN test individual digits

  u = 8;
  digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);    // let all 8s sit on the scoreboard for awhile

  u = 10;
  v = 8;
  digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);    // let all 8s sit on the scoreboard for awhile

  v = 10;
  w = 8;
  digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);    // let all 8s sit on the scoreboard for awhile

  w = 10;
  y = 8;
  digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);    // let all 8s sit on the scoreboard for awhile

  y = 10;
  z = 8;
  digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);    // let all 8s sit on the scoreboard for awhile

  z = 10;
  digit_update(u, v, w, x, y, z, dotA, dotB, dotC, dotD, dotE, dotF, dotG);

  // END test individual digits

  // BEGIN test individual DOTs

  dotA = 1;  // ball 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotB = 1;  // ball 2
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotC = 1;  // ball 3
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotD = 1;  // strike 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotE = 1;  // strike 2
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotF = 1;  // out 1
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotG = 1;  // out 2
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  delay(900);

  dotA = 0;  // turn all DOTs off
  dotB = 0;
  dotC = 0;
  dotD = 0;
  dotE = 0;
  dotF = 0;
  dotG = 0;
  digit_update(10, 10, 10, 10, 10, 10, dotA, dotB, dotC, dotD, dotE, dotF, dotG);

  // END test individual DOTs
}


String formatUptime() {
  unsigned long ms = millis();
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  char buf[50];
  sprintf(buf, "%lu day%s %02lu:%02lu:%02lu", days, (days == 1 ? "" : "s"), hours, minutes, seconds);
  return String(buf);
}
