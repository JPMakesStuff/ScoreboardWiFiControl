#include "WiFiS3.h"

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

// --------------- WiFi CONFIG ---------------
// Fill these in to match your WiFi
// When testing without infrastructure WiFi set to match your phone's hotspot WiFi
char ssid[] = "[TYPE YOUR SSID HERE]";
char pass[] = "[TYPE YOUR PASSWORD HERE]";

// software version
char swver[] = "25.11.18r01";

// declair pins
int dataPin = 4;	// Data pin of 74HC595 is connected to Digital pin 4 (purple wire to digit ribbon cable)
int latchPin = 5;	// Latch pin of 74HC595 is connected to Digital pin 5 (yellow wire to digit ribbon cable)
int clockPin = 6;	// Clock pin of 74HC595 is connected to Digital pin 6 (blue wire to digit ribbon cable)
    // dont forget GND connection (green wire to digit ribbon cable)

// store value for each dot, but initially set to off = 0 (otherwise send 1 = on)
int dotA = 0;
int dotB = 0;
int dotC = 0;
int dotD = 0;
int dotE = 0;
int dotF = 0;
int dotG = 0;

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
  B00000001, // 11 = segment A only
  B00000010, // 12 = segment B only
  B00000100, // 13 = segment C only
  B00001000, // 14 = segment D only
  B00010000, // 15 = segment E only
  B00100000, // 16 = segment F only
  B01000000, // 17 = segment G only
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


void setup() {

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

  flag_update = false;
}


void loop() {
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
        // NOT USED in single digit tester pages
        if (request.indexOf("GET /sprites.png") >= 0) {
          client.println("Content-Type: image/png");
          //client.println("Content-Length: " + String(sprites_png_len));
          client.println("Cache-Control: max-age=86400");  // browser cache for 1 day
          client.println("Connection: close");
          client.println();
          //client.write(sprites_png, sprites_png_len);
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

        // check if the client request was "GET /sysinfo"
        else if (request.indexOf("GET /sysinfo") >= 0) {
          client.print("<html>");
          client.print("<head>");
          client.print("<meta http-equiv=\"Content-Language\" content=\"en-us\">");
          client.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">");
          client.print("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no;\">");
          client.print("<title>System info - Scoreboard digit tester</title>");

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

          client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
          client.print("Scoreboard digit tester v");
          client.print(swver);
          client.print("<br>");
          client.print("&copy; 2025 JPMakesStuff<br>");
          client.print("</font>");

          client.print("<iframe name=\"hidden\" width=\"50\" height=\"20\" align=\"top\" src=\"/placeholder\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");

          client.print("</font>");
          client.print("</body>");
          client.print("</html>");
        }

else if (request.indexOf("GET /test") >= 0) {

  // ----------------- PARSE WHICH SEGMENT TO SHOW -----------------
  // Use /test?seg=8, /test?seg=off, /test?seg=A, etc.

  int testVal = 10;             // default = OFF (index 10)
  const char* currentLabel = "Off";

  if (request.indexOf("seg=8") >= 0) {
    testVal = 8;
    currentLabel = "All 7 segments";
  } else if (request.indexOf("seg=A") >= 0) {
    testVal = 11;
    currentLabel = "Segment A only";
  } else if (request.indexOf("seg=B") >= 0) {
    testVal = 12;
    currentLabel = "Segment B only";
  } else if (request.indexOf("seg=C") >= 0) {
    testVal = 13;
    currentLabel = "Segment C only";
  } else if (request.indexOf("seg=D") >= 0) {
    testVal = 14;
    currentLabel = "Segment D only";
  } else if (request.indexOf("seg=E") >= 0) {
    testVal = 15;
    currentLabel = "Segment E only";
  } else if (request.indexOf("seg=F") >= 0) {
    testVal = 16;
    currentLabel = "Segment F only";
  } else if (request.indexOf("seg=G") >= 0) {
    testVal = 17;
    currentLabel = "Segment G only";
  } else if (request.indexOf("seg=off") >= 0) {
    testVal = 10;
    currentLabel = "All off";
  }

  // ----------------- PUSH TO SINGLE DIGIT -----------------
  // First digit in chain (HOME left in the full board),
  // but for this tester it's just "the only digit connected".
  int u_val = testVal;

  dotA = 0;
  dotB = 0;
  dotC = 0;
  dotD = 0;
  dotE = 0;
  dotF = 0;
  dotG = 0;

  digit_update(u_val, 10, 10, 10, 10, 10,
               dotA, dotB, dotC, dotD, dotE, dotF, dotG);

  // ----------------- TEST PAGE UI -----------------
  client.print("<html>");
  client.print("<head>");
  client.print("<meta http-equiv=\"Content-Language\" content=\"en-us\">");
  client.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">");
  client.print("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no;\">");
  client.print("<title>Test - Scoreboard digit tester</title>");

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

  client.print("body{font-family:Verdana, sans-serif;font-size:12pt;text-align:center;}");

  client.print("a.btn{display:inline-block;margin:4px 6px;padding:6px 10px;border:1px solid #ccc;border-radius:6px;text-decoration:none;color:black;}");
  client.print("a.btn:hover{background:#eee;}");

  client.print(".current{margin:10px 0;font-size:10pt;color:#555;}");

  /* 7-segment grid */
  client.print(".seg-grid{display:inline-block;margin-top:10px;}");
  client.print(".seg-grid td{padding:3px;}");

  /* base style for segment buttons */
  client.print(".seg{display:inline-block;text-decoration:none;border:1px solid #ccc;border-radius:4px;background:#f8f8f8;color:#000;font-size:10pt;}");

  /* horizontal segments (A, G, D) */
  client.print(".seg.h{width:60px;padding:4px 0;}");

  /* vertical segments (F, E, B, C) */
  client.print(".seg.v{width:20px;padding:18px 0;}");

  /* hover */
  client.print(".seg:hover{background:#eee;}");
  client.print("</style>");


  client.print("</head>");
  client.print("<body>");
  client.print("<font face=\"Verdana\" style=\"font-size: 10pt\">");

  client.print("Test digit segments<br>");
  client.print("<div class='current'>Current pattern: <b>");
  client.print(currentLabel);
  client.print("</b></div><br>");

  client.print("<div>");
  client.print("<a class='btn' href=\"/test?seg=off\">All off</a>");
  client.print("<a class='btn' href=\"/test?seg=8\">All 7 segments</a>");
  client.print("</div><br>");

  // 7-segment layout with G included:
  //        A
  //     F     B
  //        G
  //     E     C
  //        D

  client.print("<table class='seg-grid' cellspacing='0' cellpadding='0'>");

  // Row 1: top (A)
  client.print("<tr>");
  client.print("<td></td>");
  client.print("<td><a class='seg h' href=\"/test?seg=A\">A</a></td>");
  client.print("<td></td>");
  client.print("</tr>");

  // Row 2: upper verticals (F and B)
  client.print("<tr>");
  client.print("<td><a class='seg v' href=\"/test?seg=F\">F</a></td>");
  client.print("<td></td>");
  client.print("<td><a class='seg v' href=\"/test?seg=B\">B</a></td>");
  client.print("</tr>");

  // Row 3: middle (G)
  client.print("<tr>");
  client.print("<td></td>");
  client.print("<td><a class='seg h' href=\"/test?seg=G\">G</a></td>");
  client.print("<td></td>");
  client.print("</tr>");

  // Row 4: lower verticals (E and C)
  client.print("<tr>");
  client.print("<td><a class='seg v' href=\"/test?seg=E\">E</a></td>");
  client.print("<td></td>");
  client.print("<td><a class='seg v' href=\"/test?seg=C\">C</a></td>");
  client.print("</tr>");

  // Row 5: bottom (D)
  client.print("<tr>");
  client.print("<td></td>");
  client.print("<td><a class='seg h' href=\"/test?seg=D\">D</a></td>");
  client.print("<td></td>");
  client.print("</tr>");

  client.print("</table><br><br>");

  client.print("Note: When testing dot driver...<br>");
  client.print("A, B, C = Ball 1, 2, 3<br>");
  client.print("D, E = Strike 1, 2<br>");
  client.print("F, G = Out 1, 2<br><br>");

  client.print("<br><a href=\"/\">Back to main menu</a><br><br>");

  client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
  client.print("Scoreboard digit tester v");
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
          client.print("<title>Scoreboard digit tester</title>");

          client.print("<style>");

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

          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/test\">Test digit segments</a><br><br></font>");
          
          client.print("<font face=\"Verdana\" style=\"font-size: 10pt\">");
          client.print("Single digit connection only -- NOT entire scoreboard!<br><br>");
          client.print("Observe correct wiring:<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<u>Ribbon cable</u>&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<u>Function</u>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;<u>Arduino pin</u><br>");
          client.print("&nbsp;&nbsp;&nbsp;Red stripe 1+2 | Data of 74HC595 IC&nbsp;&nbsp;| digital pin 4<br>");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Wires 3+4 | Latch of 74HC595 IC | digital pin 5<br>");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Wires 5+6 | Clock of 74HC595 IC | digital pin 6<br>");
          client.print("&nbsp;&nbsp;&nbsp;&nbsp;Wires 7,8,9,10 | GND&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| GND<br><br></font>");

          client.print("<font face=\"Verdana\" style=\"font-size: 8pt\">");
          client.print("<a href=\"/sysinfo\">Show system info</a><br>");
          client.print("Scoreboard digit tester v");
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

}

void digit_update(int u, int v, int w, int x, int y, int z,
                  int dotA, int dotB, int dotC, int dotD, int dotE, int dotF, int dotG)
{
  // Single-digit tester version:
  // Only the first 74HC595 / digit is connected
  // ignore v,w,x,y,z and all dot flags
  // just show num[u] on that one digit

  byte pattern = num[u];

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, pattern);
  digitalWrite(latchPin, HIGH);

  flag_update = true;
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
