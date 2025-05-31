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
char swver[] = "25.05.31r07";

// declair pins
int dataPin = 4;	// Data pin of 74HC595 is connected to Digital pin 4 (purple wire to digit ribbon cable)
int latchPin = 5;	// Latch pin of 74HC595 is connected to Digital pin 5 (yellow wire to digit ribbon cable)
int clockPin = 6;	// Clock pin of 74HC595 is connected to Digital pin 6 (blue wire to digit ribbon cable)
    // dont forget GND connection (green wire to digit ribbon cable)

// Digit index mapping based on wiring order from Arduino to shift registers
#define DIGIT_BALL         0
#define DIGIT_INN1_GUEST   1
#define DIGIT_INN1_HOME    2
#define DIGIT_INN2_HOME    3
#define DIGIT_INN2_GUEST   4
#define DIGIT_INN3_GUEST   5
#define DIGIT_INN3_HOME    6
#define DIGIT_INN4_HOME    7
#define DIGIT_INN4_GUEST   8
#define DIGIT_INN5_GUEST   9
#define DIGIT_INN5_HOME    10
#define DIGIT_INN6_HOME    11
#define DIGIT_INN6_GUEST   12
#define DIGIT_STRIKE       13
#define DIGIT_INN7_GUEST   14
#define DIGIT_INN7_HOME    15
#define DIGIT_INN8_HOME    16
#define DIGIT_INN8_GUEST   17
#define DIGIT_INN9_GUEST   18
#define DIGIT_INN9_HOME    19
#define DIGIT_HOME_TENS    20
#define DIGIT_HOME_ONES    21
#define DIGIT_GUEST_ONES   22
#define DIGIT_GUEST_TENS   23
#define DIGIT_OUT          24

int digits[25]; // Holds values 0–9 or 10 (for blank/off)

int digits_backup[25];  // Holds previous state for /restore

int guestInningScores[9] = {0};
int homeInningScores[9] = {0};

// --- Helper functions ---
int guestInningDigitIndex(int inning) {
  switch (inning) {
    case 1: return DIGIT_INN1_GUEST;
    case 2: return DIGIT_INN2_GUEST;
    case 3: return DIGIT_INN3_GUEST;
    case 4: return DIGIT_INN4_GUEST;
    case 5: return DIGIT_INN5_GUEST;
    case 6: return DIGIT_INN6_GUEST;
    case 7: return DIGIT_INN7_GUEST;
    case 8: return DIGIT_INN8_GUEST;
    case 9: return DIGIT_INN9_GUEST;
  }
  return -1;
}

int homeInningDigitIndex(int inning) {
  switch (inning) {
    case 1: return DIGIT_INN1_HOME;
    case 2: return DIGIT_INN2_HOME;
    case 3: return DIGIT_INN3_HOME;
    case 4: return DIGIT_INN4_HOME;
    case 5: return DIGIT_INN5_HOME;
    case 6: return DIGIT_INN6_HOME;
    case 7: return DIGIT_INN7_HOME;
    case 8: return DIGIT_INN8_HOME;
    case 9: return DIGIT_INN9_HOME;
  }
  return -1;
}

int lastGuestTotal = 0;
int lastHomeTotal  = 0;
int currentInning = 1;

// store values globally for actual score
int ballCount = 0;    // 0–3 (for dots A–C)
int strikeCount = 0;  // 0–2 (for dots D–E)
int outCount = 0;     // 0–2 (for dots F–G)

// to control each 7-segment digit
const byte digitToSegmentMap[11] = {
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

  // displaying the IP below let's the end-user know we're done booting

  // reset all digits to off just incase
  for (int i = 0; i < 25; i++) {
    digits[i] = 10;
  }
  digit_update();

  //this scrolls the 3rd and 4th octets of the IP on the Arduino UNO R4 WiFi's built-in LED matrix
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

  // this displays the 3rd and 4th octets of the IP on the scoreboards per-inning home score digits
  String part3Str = String(localIP[2]);
  String part4Str = String(localIP[3]);

  // since there are no decimals on scoreboard digits a blank digit will represent the decimal between IP octets
  int displayDigits[7];  // up to 3 digits + blank digit + up to 3 digits
  int idx = 0;

  // Add digits for part 3 (octet 3)
  for (int i = 0; i < part3Str.length() && idx < 3; i++) {
    displayDigits[idx++] = part3Str.charAt(i) - '0';
  }

  // Add blank space separator
  displayDigits[idx++] = 10;  // blank

  // Add digits for part 4 (octet 4)
  for (int i = 0; i < part4Str.length() && idx < 6; i++) {
    displayDigits[idx++] = part4Str.charAt(i) - '0';
  }

  // Fill any unused positions with blank
  while (idx < 7) {
    displayDigits[idx++] = 10;
  }

  // Assign to per-inning home digits
  int digitPositions[7] = {2, 4, 6, 8, 10, 12, 14};
  for (int i = 0; i < 7; i++) {
    digits[digitPositions[i]] = displayDigits[i];
  }

  digit_update();
  delay(4000);  // Show for 4 seconds

  // start a new game

  // Reset inning scores and totals
  for (int i = 0; i < 9; i++) {
    guestInningScores[i] = 0;
    homeInningScores[i] = 0;
  }

  ballCount = 0;
  strikeCount = 0;
  outCount = 0;
  lastGuestTotal = 0;
  lastHomeTotal = 0;
  currentInning = 1;

  // Set B/S/O to 0
  digits[DIGIT_BALL] = 10;    //set 10 means digit is off
  digits[DIGIT_STRIKE] = 10;  //set 10 means digit is off
  digits[DIGIT_OUT] = 10;     //set 10 means digit is off

  // Show 0 on inning 1 digits to indicate it's the start of inning 1
  updateInningDigit(DIGIT_INN1_GUEST, 0, 1);
  updateInningDigit(DIGIT_INN1_HOME, 0, 1);

  // Show 0 for Guest and Home total scores
  digits[DIGIT_GUEST_TENS] = 10; //set 10 means digit is off
  digits[DIGIT_GUEST_ONES] = 0;
  digits[DIGIT_HOME_TENS] = 10;  //set 10 means digit is off
  digits[DIGIT_HOME_ONES] = 0;

digit_update();

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
          int total = lastHomeTotal + 1;
          if (total <= 99) {
            int idx = currentInning - 1;
            homeInningScores[idx] += 1;
            lastHomeTotal = total;

            // Update digits
            digits[DIGIT_HOME_TENS] = total / 10;
            digits[DIGIT_HOME_ONES] = total % 10;

            switch (currentInning) {
              case 1: updateInningDigit(DIGIT_INN1_HOME, homeInningScores[idx], 1); break;
              case 2: updateInningDigit(DIGIT_INN2_HOME, homeInningScores[idx], 2); break;
              case 3: updateInningDigit(DIGIT_INN3_HOME, homeInningScores[idx], 3); break;
              case 4: updateInningDigit(DIGIT_INN4_HOME, homeInningScores[idx], 4); break;
              case 5: updateInningDigit(DIGIT_INN5_HOME, homeInningScores[idx], 5); break;
              case 6: updateInningDigit(DIGIT_INN6_HOME, homeInningScores[idx], 6); break;
              case 7: updateInningDigit(DIGIT_INN7_HOME, homeInningScores[idx], 7); break;
              case 8: updateInningDigit(DIGIT_INN8_HOME, homeInningScores[idx], 8); break;
              case 9: updateInningDigit(DIGIT_INN9_HOME, homeInningScores[idx], 9); break;
            }

            digit_update();
            if (request.indexOf("show=yes") >= 0) {
              client.print("<html><head>");
              client.print("</head><body>");
              client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
              client.print("<center>");
              client.print(lastHomeTotal);
            }
          }
        }

        // HOME SCORE DOWN - request may come from either remote page
        else if (request.indexOf("GET /hdn") >= 0) {
          int total = lastHomeTotal - 1;
          if (total >= 0) {
            int idx = currentInning - 1;
            homeInningScores[idx] -= 1;
            lastHomeTotal = total;

            digits[DIGIT_HOME_TENS] = total / 10;
            digits[DIGIT_HOME_ONES] = total % 10;

            switch (currentInning) {
              case 1: updateInningDigit(DIGIT_INN1_HOME, homeInningScores[idx], 1); break;
              case 2: updateInningDigit(DIGIT_INN2_HOME, homeInningScores[idx], 2); break;
              case 3: updateInningDigit(DIGIT_INN3_HOME, homeInningScores[idx], 3); break;
              case 4: updateInningDigit(DIGIT_INN4_HOME, homeInningScores[idx], 4); break;
              case 5: updateInningDigit(DIGIT_INN5_HOME, homeInningScores[idx], 5); break;
              case 6: updateInningDigit(DIGIT_INN6_HOME, homeInningScores[idx], 6); break;
              case 7: updateInningDigit(DIGIT_INN7_HOME, homeInningScores[idx], 7); break;
              case 8: updateInningDigit(DIGIT_INN8_HOME, homeInningScores[idx], 8); break;
              case 9: updateInningDigit(DIGIT_INN9_HOME, homeInningScores[idx], 9); break;
            }

            digit_update();
            if (request.indexOf("show=yes") >= 0) {
              client.print("<html><head>");
              client.print("</head><body>");
              client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
              client.print("<center>");
              client.print(lastHomeTotal);
            }
          }
        }

        // INNING UP - request may come from either remote page
        else if (request.indexOf("GET /iup") >= 0) {
          // Advance to next inning, wrap from 9 → 1
          currentInning = (currentInning == 9) ? 1 : currentInning + 1;

          int idx = currentInning - 1;

          // If not yet initialized, make sure the inning score arrays are zeroed
          guestInningScores[idx] = 0;
          homeInningScores[idx] = 0;

          // Show a 0 on the digit displays for new inning
          switch (currentInning) {
            case 1:
              updateInningDigit(DIGIT_INN1_GUEST, 0, 1);
              updateInningDigit(DIGIT_INN1_HOME,  0, 1);
              break;
            case 2:
              updateInningDigit(DIGIT_INN2_GUEST, 0, 2);
              updateInningDigit(DIGIT_INN2_HOME,  0, 2);
              break;
            case 3:
              updateInningDigit(DIGIT_INN3_GUEST, 0, 3);
              updateInningDigit(DIGIT_INN3_HOME,  0, 3);
              break;
            case 4:
              updateInningDigit(DIGIT_INN4_GUEST, 0, 4);
              updateInningDigit(DIGIT_INN4_HOME,  0, 4);
              break;
            case 5:
              updateInningDigit(DIGIT_INN5_GUEST, 0, 5);
              updateInningDigit(DIGIT_INN5_HOME,  0, 5);
              break;
            case 6:
              updateInningDigit(DIGIT_INN6_GUEST, 0, 6);
              updateInningDigit(DIGIT_INN6_HOME,  0, 6);
              break;
            case 7:
              updateInningDigit(DIGIT_INN7_GUEST, 0, 7);
              updateInningDigit(DIGIT_INN7_HOME,  0, 7);
              break;
            case 8:
              updateInningDigit(DIGIT_INN8_GUEST, 0, 8);
              updateInningDigit(DIGIT_INN8_HOME,  0, 8);
              break;
            case 9:
              updateInningDigit(DIGIT_INN9_GUEST, 0, 9);
              updateInningDigit(DIGIT_INN9_HOME,  0, 9);
              break;
          }

          // Reset B/S/O logic and display
          ballCount = 0;
          strikeCount = 0;
          outCount = 0;
          digits[DIGIT_BALL]   = 10;
          digits[DIGIT_STRIKE] = 10;
          digits[DIGIT_OUT]    = 10;

          digit_update();

          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            // must refresh the whole page otherwise B/S/O could be showing old data
            client.print("<meta http-equiv=\"refresh\" content=\"1; url=/full\">");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print("One moment...");
          }
        }


        // INNING DOWN - request may come from either remote page
        else if (request.indexOf("GET /idn") >= 0) {
          int eraseInning = currentInning;
          int eraseIdx = eraseInning - 1;

          // Subtract scores from totals
          lastGuestTotal -= guestInningScores[eraseIdx];
          lastHomeTotal  -= homeInningScores[eraseIdx];

          // Zero out the inning scores
          guestInningScores[eraseIdx] = 0;
          homeInningScores[eraseIdx] = 0;

          // Blank the digit display for the inning being cleared
          switch (eraseInning) {
            case 1: digits[DIGIT_INN1_GUEST] = 10; digits[DIGIT_INN1_HOME] = 10; break;
            case 2: digits[DIGIT_INN2_GUEST] = 10; digits[DIGIT_INN2_HOME] = 10; break;
            case 3: digits[DIGIT_INN3_GUEST] = 10; digits[DIGIT_INN3_HOME] = 10; break;
            case 4: digits[DIGIT_INN4_GUEST] = 10; digits[DIGIT_INN4_HOME] = 10; break;
            case 5: digits[DIGIT_INN5_GUEST] = 10; digits[DIGIT_INN5_HOME] = 10; break;
            case 6: digits[DIGIT_INN6_GUEST] = 10; digits[DIGIT_INN6_HOME] = 10; break;
            case 7: digits[DIGIT_INN7_GUEST] = 10; digits[DIGIT_INN7_HOME] = 10; break;
            case 8: digits[DIGIT_INN8_GUEST] = 10; digits[DIGIT_INN8_HOME] = 10; break;
            case 9: digits[DIGIT_INN9_GUEST] = 10; digits[DIGIT_INN9_HOME] = 10; break;
          }

          // Update the total score display
          digits[DIGIT_GUEST_TENS] = lastGuestTotal / 10;
          digits[DIGIT_GUEST_ONES] = lastGuestTotal % 10;
          digits[DIGIT_HOME_TENS]  = lastHomeTotal  / 10;
          digits[DIGIT_HOME_ONES]  = lastHomeTotal  % 10;

          // Go back one inning
          currentInning = (currentInning == 1) ? 9 : currentInning - 1;
          int idx = currentInning - 1;

          // Re-display inning digits for the new current inning
          switch (currentInning) {
            case 1:
              updateInningDigit(DIGIT_INN1_GUEST, guestInningScores[idx], 1);
              updateInningDigit(DIGIT_INN1_HOME, homeInningScores[idx], 1);
              break;
            case 2:
              updateInningDigit(DIGIT_INN2_GUEST, guestInningScores[idx], 2);
              updateInningDigit(DIGIT_INN2_HOME, homeInningScores[idx], 2);
              break;
            case 3:
              updateInningDigit(DIGIT_INN3_GUEST, guestInningScores[idx], 3);
              updateInningDigit(DIGIT_INN3_HOME, homeInningScores[idx], 3);
              break;
            case 4:
              updateInningDigit(DIGIT_INN4_GUEST, guestInningScores[idx], 4);
              updateInningDigit(DIGIT_INN4_HOME, homeInningScores[idx], 4);
              break;
            case 5:
              updateInningDigit(DIGIT_INN5_GUEST, guestInningScores[idx], 5);
              updateInningDigit(DIGIT_INN5_HOME, homeInningScores[idx], 5);
              break;
            case 6:
              updateInningDigit(DIGIT_INN6_GUEST, guestInningScores[idx], 6);
              updateInningDigit(DIGIT_INN6_HOME, homeInningScores[idx], 6);
              break;
            case 7:
              updateInningDigit(DIGIT_INN7_GUEST, guestInningScores[idx], 7);
              updateInningDigit(DIGIT_INN7_HOME, homeInningScores[idx], 7);
              break;
            case 8:
              updateInningDigit(DIGIT_INN8_GUEST, guestInningScores[idx], 8);
              updateInningDigit(DIGIT_INN8_HOME, homeInningScores[idx], 8);
              break;
            case 9:
              updateInningDigit(DIGIT_INN9_GUEST, guestInningScores[idx], 9);
              updateInningDigit(DIGIT_INN9_HOME, homeInningScores[idx], 9);
              break;
          }

          // Clear B/S/O as part of going back an inning
          ballCount = 0;
          strikeCount = 0;
          outCount = 0;
          digits[DIGIT_BALL] = 10;
          digits[DIGIT_STRIKE] = 10;
          digits[DIGIT_OUT] = 10;

          digit_update();

          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            // must refresh the whole page otherwise B/S/O could be showing old data
            client.print("<meta http-equiv=\"refresh\" content=\"1; url=/full\">");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print("One moment...");
          }
        }


        // GUEST SCORE UP - request may come from either remote page
        else if (request.indexOf("GET /gup") >= 0) {
          int total = lastGuestTotal + 1;
          if (total <= 99) {
            int idx = currentInning - 1;
            guestInningScores[idx] += 1;
            lastGuestTotal = total;

            // Update digits
            digits[DIGIT_GUEST_TENS] = total / 10;
            digits[DIGIT_GUEST_ONES] = total % 10;

            switch (currentInning) {
              case 1: updateInningDigit(DIGIT_INN1_GUEST, guestInningScores[idx], 1); break;
              case 2: updateInningDigit(DIGIT_INN2_GUEST, guestInningScores[idx], 2); break;
              case 3: updateInningDigit(DIGIT_INN3_GUEST, guestInningScores[idx], 3); break;
              case 4: updateInningDigit(DIGIT_INN4_GUEST, guestInningScores[idx], 4); break;
              case 5: updateInningDigit(DIGIT_INN5_GUEST, guestInningScores[idx], 5); break;
              case 6: updateInningDigit(DIGIT_INN6_GUEST, guestInningScores[idx], 6); break;
              case 7: updateInningDigit(DIGIT_INN7_GUEST, guestInningScores[idx], 7); break;
              case 8: updateInningDigit(DIGIT_INN8_GUEST, guestInningScores[idx], 8); break;
              case 9: updateInningDigit(DIGIT_INN9_GUEST, guestInningScores[idx], 9); break;
            }

            digit_update();
            if (request.indexOf("show=yes") >= 0) {
              client.print("<html><head>");
              client.print("</head><body>");
              client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
              client.print("<center>");
              client.print(lastGuestTotal);
            }
          }
        }

        // GUEST SCORE DOWN - request may come from either remote page
        else if (request.indexOf("GET /gdn") >= 0) {
          int total = lastGuestTotal - 1;
          if (total >= 0) {
            int idx = currentInning - 1;
            guestInningScores[idx] -= 1;
            lastGuestTotal = total;

            digits[DIGIT_GUEST_TENS] = total / 10;
            digits[DIGIT_GUEST_ONES] = total % 10;

            switch (currentInning) {
              case 1: updateInningDigit(DIGIT_INN1_GUEST, guestInningScores[idx], 1); break;
              case 2: updateInningDigit(DIGIT_INN2_GUEST, guestInningScores[idx], 2); break;
              case 3: updateInningDigit(DIGIT_INN3_GUEST, guestInningScores[idx], 3); break;
              case 4: updateInningDigit(DIGIT_INN4_GUEST, guestInningScores[idx], 4); break;
              case 5: updateInningDigit(DIGIT_INN5_GUEST, guestInningScores[idx], 5); break;
              case 6: updateInningDigit(DIGIT_INN6_GUEST, guestInningScores[idx], 6); break;
              case 7: updateInningDigit(DIGIT_INN7_GUEST, guestInningScores[idx], 7); break;
              case 8: updateInningDigit(DIGIT_INN8_GUEST, guestInningScores[idx], 8); break;
              case 9: updateInningDigit(DIGIT_INN9_GUEST, guestInningScores[idx], 9); break;
            }

            digit_update();
            if (request.indexOf("show=yes") >= 0) {
              client.print("<html><head>");
              client.print("</head><body>");
              client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
              client.print("<center>");
              client.print(lastGuestTotal);
            }
          }
        }

        // BALLs - request may come from either remote page
        else if (request.indexOf("GET /ball") >= 0) {
          ballCount = (ballCount + 1) % 4;  // 0 to 3, loops back to 0

          // Show 1–3, blank digit if 0
          digits[DIGIT_BALL] = (ballCount == 0) ? 10 : ballCount;

          digit_update();  // update display immediately

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
          strikeCount = (strikeCount + 1) % 3;  // 0 to 2

          // Show 1–2, blank if 0
          digits[DIGIT_STRIKE] = (strikeCount == 0) ? 10 : strikeCount;

          digit_update();  // update display

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
          outCount = (outCount + 1) % 3;  // 0 to 2

          // Show 1–2, blank if 0
          digits[DIGIT_OUT] = (outCount == 0) ? 10 : outCount;

          digit_update();  // update display

          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
            client.print(outCount);
          }
        }

        else if (request.indexOf("GET /new") >= 0) {
          // Reset all digits to off
          for (int i = 0; i < 25; i++) {
            digits[i] = 10;
          }

          // Reset inning scores and totals
          for (int i = 0; i < 9; i++) {
            guestInningScores[i] = 0;
            homeInningScores[i] = 0;
          }

          ballCount = 0;
          strikeCount = 0;
          outCount = 0;
          lastGuestTotal = 0;
          lastHomeTotal = 0;
          currentInning = 1;

          // Set B/S/O to 0
          digits[DIGIT_BALL] = 10;    //set 10 means digit is off
          digits[DIGIT_STRIKE] = 10;  //set 10 means digit is off
          digits[DIGIT_OUT] = 10;     //set 10 means digit is off

          // Show 0 on inning 1 digits to indicate it's the start of inning 1
          updateInningDigit(DIGIT_INN1_GUEST, 0, 1);
          updateInningDigit(DIGIT_INN1_HOME, 0, 1);

          // Show 0 for Guest and Home total scores
          digits[DIGIT_GUEST_TENS] = 10; //set 10 means digit is off
          digits[DIGIT_GUEST_ONES] = 0;
          digits[DIGIT_HOME_TENS] = 10;  //set 10 means digit is off
          digits[DIGIT_HOME_ONES] = 0;

          digit_update();
        }

        else if (request.indexOf("GET /blank") >= 0) {
          // Backup current display state
          for (int i = 0; i < 25; i++) {
            digits_backup[i] = digits[i];
            digits[i] = 10;  // turn off all digits
          }

          digit_update();  // apply blanking
        }

        else if (request.indexOf("GET /restore") >= 0) {
          // Restore previous digit values from backup
          for (int i = 0; i < 25; i++) {
            digits[i] = digits_backup[i];
          }

          digit_update();  // re-apply previous state
        }

        // request home score API FUNCTION check if the client request was "GET /rhome"
        else if (request.indexOf("GET /rhome") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }

          client.print(lastHomeTotal);
        }

        // request inning API FUNCTION check if the client request was "GET /rinning"
        else if (request.indexOf("GET /rinning") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }

          client.print(currentInning);
        }

        // request guest score API FUNCTION check if the client request was "GET /rguest"
        else if (request.indexOf("GET /rguest") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }

          client.print(lastGuestTotal);
        }

        // request ball API FUNCTION check if the client request was "GET /rball"
        else if (request.indexOf("GET /rball") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }

          client.print(ballCount);
        }


        // request strike API FUNCTION check if the client request was "GET /rstrike"
        else if (request.indexOf("GET /rstrike") >= 0) {
          if (request.indexOf("show=yes") >= 0) {
            client.print("<html><head>");
            client.print("</head><body>");
            client.print("<font face=\"Verdana\" style=\"font-size: 20pt\">");
            client.print("<center>");
          }

          client.print(strikeCount);
        }


        // request out API FUNCTION check if the client request was "GET /rout"
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
          int start = request.indexOf("/shome?") + 7;
          String numStr = "";

          // Collect up to 2 digits
          while (numStr.length() < 2 && isDigit(request.charAt(start))) {
            numStr += request.charAt(start++);
          }

          int newScore = numStr.toInt();

          if (numStr.length() > 0 && newScore >= 0 && newScore <= 99) {
            int delta = newScore - lastHomeTotal;
            int idx = currentInning - 1;

            homeInningScores[idx] += delta;
            lastHomeTotal = newScore;

            // Update total score display
            digits[DIGIT_HOME_TENS] = newScore / 10;
            digits[DIGIT_HOME_ONES] = newScore % 10;

            // Update per-inning digit
            switch (currentInning) {
              case 1: updateInningDigit(DIGIT_INN1_HOME, homeInningScores[idx], 1); break;
              case 2: updateInningDigit(DIGIT_INN2_HOME, homeInningScores[idx], 2); break;
              case 3: updateInningDigit(DIGIT_INN3_HOME, homeInningScores[idx], 3); break;
              case 4: updateInningDigit(DIGIT_INN4_HOME, homeInningScores[idx], 4); break;
              case 5: updateInningDigit(DIGIT_INN5_HOME, homeInningScores[idx], 5); break;
              case 6: updateInningDigit(DIGIT_INN6_HOME, homeInningScores[idx], 6); break;
              case 7: updateInningDigit(DIGIT_INN7_HOME, homeInningScores[idx], 7); break;
              case 8: updateInningDigit(DIGIT_INN8_HOME, homeInningScores[idx], 8); break;
              case 9: updateInningDigit(DIGIT_INN9_HOME, homeInningScores[idx], 9); break;
            }

            digit_update();
            client.println("OK");
          } else {
            client.println("ERR");
          }
        }


        // set the inning API FUNCTION check if the client request was "GET /sinn?##"
        else if (request.indexOf("GET /sinn?") >= 0) {
          int start = request.indexOf("/sinn?") + 6;
          String numStr = "";

          // Extract up to 2 digits
          while (numStr.length() < 2 && isDigit(request.charAt(start))) {
            numStr += request.charAt(start++);
          }

          int newInning = numStr.toInt();

          if (numStr.length() > 0 && newInning == currentInning + 1 && newInning >= 1 && newInning <= 9) {
            currentInning = newInning;

            // Reset B/S/O logic and display
            ballCount = 0;
            strikeCount = 0;
            outCount = 0;
            digits[DIGIT_BALL] = 10;
            digits[DIGIT_STRIKE] = 10;
            digits[DIGIT_OUT] = 10;

            // Update inning digits (show 0 if score is 0)
            int idx = currentInning - 1;
            updateInningDigit(guestInningDigitIndex(currentInning), guestInningScores[idx], currentInning);
            updateInningDigit(homeInningDigitIndex(currentInning), homeInningScores[idx], currentInning);

            digit_update();
            client.println("OK");
          } else {
            client.println("ERR");
          }
        }

        // set the guest score API FUNCTION check if the client request was "GET /sguest?##"
        else if (request.indexOf("GET /sguest?") >= 0) {
          int start = request.indexOf("/sguest?") + 8;
          String numStr = "";

          // Collect up to 2 digits
          while (numStr.length() < 2 && isDigit(request.charAt(start))) {
            numStr += request.charAt(start++);
          }

          int newScore = numStr.toInt();

          if (numStr.length() > 0 && newScore >= 0 && newScore <= 99) {
            int delta = newScore - lastGuestTotal;
            int idx = currentInning - 1;

            guestInningScores[idx] += delta;
            lastGuestTotal = newScore;

            // Update total score digits
            digits[DIGIT_GUEST_TENS] = newScore / 10;
            digits[DIGIT_GUEST_ONES] = newScore % 10;

            // Update per-inning digit
            switch (currentInning) {
              case 1: updateInningDigit(DIGIT_INN1_GUEST, guestInningScores[idx], 1); break;
              case 2: updateInningDigit(DIGIT_INN2_GUEST, guestInningScores[idx], 2); break;
              case 3: updateInningDigit(DIGIT_INN3_GUEST, guestInningScores[idx], 3); break;
              case 4: updateInningDigit(DIGIT_INN4_GUEST, guestInningScores[idx], 4); break;
              case 5: updateInningDigit(DIGIT_INN5_GUEST, guestInningScores[idx], 5); break;
              case 6: updateInningDigit(DIGIT_INN6_GUEST, guestInningScores[idx], 6); break;
              case 7: updateInningDigit(DIGIT_INN7_GUEST, guestInningScores[idx], 7); break;
              case 8: updateInningDigit(DIGIT_INN8_GUEST, guestInningScores[idx], 8); break;
              case 9: updateInningDigit(DIGIT_INN9_GUEST, guestInningScores[idx], 9); break;
            }

            digit_update();
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
              digits[DIGIT_BALL] = (ballCount == 0) ? 10 : ballCount;

              digit_update();
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
              digits[DIGIT_STRIKE] = (strikeCount == 0) ? 10 : strikeCount;

              digit_update();
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
              digits[DIGIT_OUT] = (outCount == 0) ? 10 : outCount;

              digit_update();
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
          client.print("&nbsp;&nbsp;&nbsp;Shows 8 on each digit, then shows data again");

          client.print("<br><br>");

          client.print("API endpoints...<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;<a href=\"/rhome\">/rhome</a> - <u>r</u>eturns homeScore as integer<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;<a href=\"/rhome?show=yes\">/rhome?show=yes</a> - <u>r</u>eturns homeScore as an HTML page<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;<a href=\"/shome?11\">/shome?11</a> - <u>s</u>ets the homeScore to 11<br><br>");
          client.print("other endpoint names...<br><br>");
          client.print("&nbsp;&nbsp;&nbsp;inn, guest, ball, strike, out<br>");

          client.print("<br><br>");

          client.print("Current Game State...<br><br>");

          // Balls, Strikes, Outs
          client.print("Ball: ");
          client.print(ballCount);
          client.print("&nbsp;&nbsp;Strike: ");
          client.print(strikeCount);
          client.print("&nbsp;&nbsp;Out: ");
          client.print(outCount);
          client.print("&nbsp;&nbsp;Inning: ");
          client.print(currentInning);
          client.print("<br><br>");

          // Guest inning-by-inning scores
          client.print("Guest: ");
          for (int i = 0; i < 9; i++) {
            client.print(guestInningScores[i]);
            if (i < 8) client.print(", ");
          }
          client.print(" &nbsp;&nbsp;&nbsp;Total: ");
          client.print(lastGuestTotal);
          client.print("<br><br>");

          // Home inning-by-inning scores
          client.print("Home: ");
          for (int i = 0; i < 9; i++) {
            client.print(homeInningScores[i]);
            if (i < 8) client.print(", ");
          }
          client.print(" &nbsp;&nbsp;&nbsp;Total: ");
          client.print(lastHomeTotal);
          client.print("<br><br>");

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
          digit_test();  // this handles saving the display, flashing 8 on every digit, and restoring the display
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
          client.print("<td class=\"larger\"><b><a href=\"/iup?show=yes\"><div class=\"icon uparr\"></div></a><br>");
          client.print("<iframe name=\"inning\" width=\"80\" height=\"60\" align=\"top\" src=\"/rinning?show=yes\" frameborder=\"0\" scrolling=\"no\" marginwidth=\"0\" marginheight=\"0\"></iframe>");
          client.print("<br><a href=\"/idn?show=yes\"><div class=\"icon dnarr\"></div></a></td>");
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

}


void digit_update() {
  digitalWrite(latchPin, LOW);

  // Shift out all 25 digits in order — BallDigit is shifted out first (digit 0)
  for (int i = 0; i < 25; i++) {
    int val = digits[i];
    if (val < 0 || val > 10) val = 10; // fallback safety
    shiftOut(dataPin, clockPin, MSBFIRST, digitToSegmentMap[val]);
  }

  digitalWrite(latchPin, HIGH);
}


void updateInningDigit(int digitIndex, int score, int inningNumber) {
  if (score == 0 && inningNumber != currentInning) {
    digits[digitIndex] = 10; // off
  } else {
    digits[digitIndex] = score;
  }
}


void digit_test()
{
  // BEGIN test individual digits
  // Save current display state
  int savedDigits[25];
  for (int i = 0; i < 25; i++) {
    savedDigits[i] = digits[i];
  }

  // Flash each digit with '8' for 1 second, then off
  for (int i = 0; i < 25; i++) {
    digits[i] = 8;
    digit_update();
    delay(1000);

    digits[i] = 10; // turn off
    digit_update();
    delay(100);
  }

  // Restore previous display state
  for (int i = 0; i < 25; i++) {
    digits[i] = savedDigits[i];
  }
  digit_update();
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
