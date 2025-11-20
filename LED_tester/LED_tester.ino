#include "WiFiS3.h"

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

// Fill these in to match your scoreboard WiFi
// ---------- PRIMARY WIFI ----------
char ssid1[] = "[PUT_YOUR_SSID_HERE]";
char pass1[] = "[PUT_YOUR_PASSWORD_HERE]";

// Fill these in to match your phone hotspot or home network for testing
// ---------- FALLBACK WIFI ----------
char ssid2[] = "AlternateWiFi";
char pass2[] = "AlternatePass";

// which WiFi did we end up on 1 = primary, 2 = fallback
int activeNet = 0;

// store the software version
char swver[] = "25.11.20r03";

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

// ---- animation modes ----
const int MODE_MANUAL = 0;
const int MODE_CHASE  = 1;
const int MODE_MULTI  = 2;
const int MODE_COUNT  = 3;

int testMode = MODE_MANUAL;        // current mode
unsigned long stepInterval = 500;  // ms between steps (changed via web UI)
unsigned long lastStepTime = 0;    // last time we advanced
int stepIndex = 0;                 // current position in the sequence

// Chase sequence: A, F, G, C, D, E, G, B
// Using the bit layout from num[] (LSB=A, then B,C,D,E,F,G)
const byte chaseSeq[] = {
  B00000001, // A
  B00100000, // F
  B01000000, // G
  B00000100, // C
  B00001000, // D
  B00010000, // E
  B01000000, // G again
  B00000010  // B
};
const int chaseLen = sizeof(chaseSeq) / sizeof(chaseSeq[0]);

// Multiples sequence: F+B, A+G, E+C, G+D
const byte multiSeq[] = {
  (B00100000 | B00000010), // F + B
  (B00000001 | B01000000), // A + G
  (B00010000 | B00000100), // E + C
  (B01000000 | B00001000)  // G + D
};
const int multiLen = sizeof(multiSeq) / sizeof(multiSeq[0]);


// store if new serial data should be sent to the HC595 shift registers or not
int flag_update = false;

// WiFi connection setup
byte mac[6];  // the MAC address of WiFi module

bool connectWiFi(const char* ssid, const char* pass, unsigned long timeoutMs) {
  WiFi.disconnect();
  delay(200);

  WiFi.begin(ssid, pass);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeoutMs) {
    delay(500);
  }

  return WiFi.status() == WL_CONNECTED;
}

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

  // ---------- WIFI CONNECT WITH FALLBACK ----------
  bool connected = false;

  // try primary first
  connected = connectWiFi(ssid1, pass1, 12000);
  if (connected) {
    activeNet = 1;
  } else {
    // try fallback
    connected = connectWiFi(ssid2, pass2, 12000);
    if (connected) {
      activeNet = 2;
    }
  }

  // If neither worked, keep cycling forever (primary then fallback)
  while (!connected) {
    connected = connectWiFi(ssid1, pass1, 12000);
    if (connected) {
      activeNet = 1;
      break;
    }

    connected = connectWiFi(ssid2, pass2, 12000);
    if (connected) {
      activeNet = 2;
      break;
    }
  }


server.begin();     // start the web server on port 80

// ---------- WAIT FOR A REAL IP (not 0.0.0.0) ----------
IPAddress localIP;
unsigned long ipStart = millis();

// wait up to 10 seconds for DHCP
do {
  localIP = WiFi.localIP();
  if (localIP[0] != 0) break;   // got something real
  delay(200);
} while (millis() - ipStart < 10000);

// PRI or SEC depending on which network connected
String netLabel = (activeNet == 1) ? "PRI" : "SEC";

// If still no IP, we'll show 0.0 to make it obvious something's wrong
String label = netLabel + " ." + String(localIP[2]) + "." + String(localIP[3]);

matrix.begin();
matrix.beginDraw();
matrix.stroke(0xFFFFFFFF);
matrix.clear();
matrix.textFont(Font_4x6);
matrix.textScrollSpeed(80);   // adjust to taste

matrix.beginText(0, 1, 0xFFFFFF);
matrix.println(label);
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

  // ---------- parse speed parameter (speed=###) ----------
  int spIdx = request.indexOf("speed=");
  if (spIdx >= 0) {
    spIdx += 6;
    int endIdx = request.indexOf(' ', spIdx);
    int ampIdx = request.indexOf('&', spIdx);
    if (ampIdx >= 0 && ampIdx < endIdx) endIdx = ampIdx;
    if (endIdx < 0) endIdx = request.length();
    String sp = request.substring(spIdx, endIdx);
    sp.trim();
    long v = sp.toInt();
    if (v >= 50 && v <= 5000) {    // clamp 50–5000 ms
      stepInterval = (unsigned long)v;
    }
  }

  // ---------- parse mode parameter (mode=chase/multi/count/manual) ----------
  if (request.indexOf("mode=chase") >= 0) {
    testMode = MODE_CHASE;
    stepIndex = 0;
    lastStepTime = millis();
    stepInterval = 200;      // default 200 ms for chase

  } else if (request.indexOf("mode=multi") >= 0) {
    testMode = MODE_MULTI;
    stepIndex = 0;
    lastStepTime = millis();
    stepInterval = 400;      // default 400 ms for multiples

  } else if (request.indexOf("mode=count") >= 0) {
    testMode = MODE_COUNT;
    stepIndex = 0;
    lastStepTime = millis();
    stepInterval = 1000;     // default 1000 ms (1 second) for count

  } else if (request.indexOf("mode=manual") >= 0) {
    testMode = MODE_MANUAL;
  }

  // ---------- parse manual segment selection (seg=...) ----------
  // Use /test?seg=8, /test?seg=off, /test?seg=A, etc.
  int testVal = 10;             // default OFF
  const char* currentLabel = "Off";

  if (request.indexOf("seg=") >= 0) {
    // Clicking any segment forces MANUAL mode
    testMode = MODE_MANUAL;

    if (request.indexOf("seg=8") >= 0) {
      testVal = 8;
      currentLabel = "Digit 8 (all segments)";
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
      currentLabel = "Off";
    }

    // apply manual pattern immediately
    int u_val = testVal;
    dotA = dotB = dotC = dotD = dotE = dotF = dotG = 0;
    digit_update(u_val, 10, 10, 10, 10, 10,
                 dotA, dotB, dotC, dotD, dotE, dotF, dotG);
  }

  // ---------- compute mode name for display ----------
  const char* modeName = "Manual";
  if (testMode == MODE_CHASE) modeName = "Chase test";
  else if (testMode == MODE_MULTI) modeName = "Multiples test";
  else if (testMode == MODE_COUNT) modeName = "Count test";

  // ----------------- BUILD TEST PAGE UI -----------------
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
  client.print(".seg-panel{");
  client.print("padding:18px;");
  client.print("margin-top:8px;");
  client.print("display:inline-block;");
  client.print("border:1px solid #ddd;");
  client.print("border-radius:8px;");
  client.print("background:#fafafa;");
  client.print("}");


  client.print("body{font-family:Verdana, sans-serif;font-size:12pt;text-align:left;}");

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
  client.print("<div class='current'>Mode: <b>");
  client.print(modeName);
  client.print("</b> &nbsp;|&nbsp; Step: ");
  client.print(stepInterval);
  client.print(" ms</div>");

  if (testMode == MODE_MANUAL) {
    client.print("<div class='current'>Current pattern: <b>");
    client.print(currentLabel);
    client.print("</b></div>");
  }

  client.print("<br>");

  // Auto-mode buttons
  client.print("<div>");
  client.print("<a class='btn' href=\"/test?mode=chase\">Chase</a>");
  client.print("<a class='btn' href=\"/test?mode=multi\">Multiples</a>");
  client.print("<a class='btn' href=\"/test?mode=count\">Count</a>");
  client.print("</div><br>");

  // Speed control
  client.print("<form method='get' action='/test' style='margin-top:10px;'>");
  client.print("Speed (ms between steps): ");
  client.print("<input type='number' name='speed' min='50' max='5000' step='50' value='");
  client.print(stepInterval);
  client.print("'>");
  client.print("<input type='submit' value='Set speed'>");
  client.print("</form><br>");

  client.print("<div>");
  client.print("<a class='btn' href=\"/test?seg=off\">All OFF</a>");
  client.print("<a class='btn' href=\"/test?seg=8\">8 (all segments)</a>");
  client.print("</div><br>");

  // 7-segment layout
  client.print("<div class='seg-panel'>");
  client.print("<table class='seg-grid' cellspacing='0' cellpadding='0'>");

  // Row 1: A
  client.print("<tr><td></td><td><a class='seg h' href=\"/test?seg=A\">A</a></td><td></td></tr>");

  // Row 2: F, B
  client.print("<tr><td><a class='seg v' href=\"/test?seg=F\">F</a></td><td></td><td><a class='seg v' href=\"/test?seg=B\">B</a></td></tr>");

  // Row 3: G
  client.print("<tr><td></td><td><a class='seg h' href=\"/test?seg=G\">G</a></td><td></td></tr>");

  // Row 4: E, C
  client.print("<tr><td><a class='seg v' href=\"/test?seg=E\">E</a></td><td></td><td><a class='seg v' href=\"/test?seg=C\">C</a></td></tr>");

  // Row 5: D
  client.print("<tr><td></td><td><a class='seg h' href=\"/test?seg=D\">D</a></td><td></td></tr>");

  client.print("</table>");
  client.print("</div><br><br>");

  client.print("<br><a href=\"/\">Back to main menu</a><br><br>");

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

  // run any active animation
  handleAnimations();
}


  // Single-digit tester version:
  //   Only the first 74HC595 / digit is connected.
  //   We ignore v,w,x,y,z and all dot flags.
  //   We just show num[u] on that one digit.
void writeDigitPattern(byte pattern) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, pattern);
  digitalWrite(latchPin, HIGH);
}

// single-digit tester: only uses 'u' as an index into num[]
void digit_update(int u, int v, int w, int x, int y, int z,
                  int dotA, int dotB, int dotC, int dotD, int dotE, int dotF, int dotG)
{
  byte pattern = num[u];   // only first argument matters
  writeDigitPattern(pattern);
  flag_update = true;
}



void handleAnimations() {
  if (testMode == MODE_MANUAL) return;  // nothing to do

  unsigned long now = millis();
  if (now - lastStepTime < stepInterval) return;  // not yet time to step

  lastStepTime = now;

  byte pattern = B00000000;

  if (testMode == MODE_CHASE) {
    pattern = chaseSeq[stepIndex];
    stepIndex = (stepIndex + 1) % chaseLen;
    writeDigitPattern(pattern);

  } else if (testMode == MODE_MULTI) {
    pattern = multiSeq[stepIndex];
    stepIndex = (stepIndex + 1) % multiLen;
    writeDigitPattern(pattern);

  } else if (testMode == MODE_COUNT) {
    // stepIndex goes 0..9 and wraps, use num[0..9]
    int digit = stepIndex % 10;
    pattern = num[digit];
    stepIndex = (stepIndex + 1) % 10;
    writeDigitPattern(pattern);

  } else {
    return;
  }
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
