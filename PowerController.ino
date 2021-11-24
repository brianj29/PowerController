/*
 * ESP8266 web control for RCSwitch
 * Based on ESP8266 PathArgServer demo and RCSwitch Webserver demo
 *
 * by Brian J. Johnson 11/22/20
 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <RCSwitch.h>

#include <uri/UriRegex.h>

const char *hostname = "power"; // mDNS hostname (xxx.local)

// Pin configuration
int RCTransmissionPin = D8; // output to transmitter
int RCReceivePin = D5;      // input from receiver
int WiFiResetPin = D1;      // input, pull low to reset WiFi parameters

RCSwitch mySwitch = RCSwitch();
ESP8266WebServer server(80);

const char *codes[][3] =
 {
  // SMART Electrician switches "(YTP) Model: JQ01TX-03 Date: 1926", transmitter "A"
  // Protocol 1
  //         on                           off
  {"011101101111011100000011", "011101101111011100000010", "Kitchen amp"},     // A1
  {"011101101111011100000101", "011101101111011100000100", "Living room amp"}, // A2
  {"011101101111011100000111", "011101101111011100000110", "Switch A3"},       // A3

  // SMART Electrician switches "(YTP) Model: JQ01TX-03 Date: 1926", transmitter "B"
  // Protocol 1
  //         on                           off
  {"010000101010101100000011", "010000101010101100000010", "Switch B1"},       // B1
  {"010000101010101100000101", "010000101010101100000100", "Switch B2"},       // B2
  {"010000101010101100000111", "010000101010101100000110", "Switch B3"},       // B3
};

//
// Handle a request to turn a switch on or off
//

void handleSwitch() {
  String name = server.pathArg(0);
  String state = server.pathArg(1);
  Serial.println("Got switch: " + name + " state: " + state);

  unsigned num = name.toInt();
  unsigned col = 999;
  if (state == "on") {
    col = 0;
  }
  else if (state == "off") {
    col = 1;
  }

  if (col > 1 || num >= (sizeof(codes) / sizeof(codes[0]))) {
    // bad args
    Serial.println("  Bad args");
    server.send(200, "text/plain", "Unknown args:  Switch: '" + name + "' State: '" + state + "'");
    return;
  }

  // Send the RC command from the table
  Serial.println("  row " + String(num) + (col == 0 ? " ON  " : " OFF ") + codes[num][col]);
  mySwitch.send(codes[num][col]);

  // Redirect back to home page
  server.sendHeader("Location","/"); // Redirection URI
  server.send(303);                  // HTTP status 303 (See Other)
}

// Calculate a binary code.  From RCSwitch ReceiveDemo_Advanced demo
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[64]; 
  unsigned int i=0;

  while (Dec > 0) {
    bin[32+i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j< bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
}

//
// Handle a request to sniff for codes
//
unsigned long rcvValue = 0;
unsigned int rcvLength;
unsigned int rcvProtocol;

void handleSniff() {
  // Generate the header and boilerplate
  String title = "RF Code Sniffer";
  String page =
    "<html>\n"
    "<head>\n"
        "<title>" + title + "</title>\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<style>\n"
            "body { font-family: Arial, sans-serif; font-size:12px; }\n"
            "table, th, td { border: none; }\n"
        "</style>\n"
        "<meta http-equiv=\"refresh\" content=\"1\">\n"
    "</head>\n"
    "<body>\n"
        "<h1>" + title + "</h1>\n";

  if (rcvValue != 0) {
    page += "<table>";
    page += "<tr><td><b>Code:</b></td><td>"  + String(rcvValue) + "</td></tr>";
    page += "<tr><td><b>Length:</b></td><td>"  + String(rcvLength) + "</td></tr>";
    page += "<tr><td><b>Binary:</b></td><td>"  + String(dec2binWzerofill(rcvValue, rcvLength)) + "</td></tr>";
    page += "<tr><td><b>Protocol:</b></td><td>"  + String(rcvProtocol) + "</td></tr>";
    page += "</table><p>";
  }
  else {
    page += "<i>Waiting for code...</i><p>";
  }

  // Add the footer
  page +=
    "<a href=\"/\">Power Switch Control</a>\n"
        "<hr>\n"
        "<a href=\"https://github.com/brianj29/PowerController/\">https://github.com/brianj29/PowerController/</a>\n"
    "</body>\n"
    "</html>\n";

  server.send(200, "text/html", page);
}

//
// Handle a request for the home page
//

void handleHomepage() {

  // Generate the header and boilerplate
  String title = "Power Switch Control";
  String homepage =
    "<html>\n"
    "<head>\n"
        "<title>" + title + "</title>\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<style>\n"
            "body { font-family: Arial, sans-serif; font-size:12px; }\n"
        "</style>\n"
    "</head>\n"
    "<body>\n"
        "<h1>" + title + "</h1>\n";

  // Add on/off entries for each switch
  for (int i = 0; i < (sizeof(codes) / sizeof(codes[0])); i++) {
    homepage +=
        "<ul>\n"
            "<li><a href=\"switch/" + String(i) + "/on\">" + codes[i][2] + " on</a></li>\n"
            "<li><a href=\"switch/" + String(i) + "/off\">" + codes[i][2] + " off</a></li>\n"
        "</ul>\n";
  }

  // Add the footer
  homepage +=
        "<a href=\"sniff\">Sniff for codes</a>\n"
        "<hr>\n"
        "<a href=\"https://github.com/brianj29/PowerController/\">https://github.com/brianj29/PowerController/</a>\n"
    "</body>\n"
    "</html>\n";

  server.send(200, "text/html", homepage);
};

void setup(void) {
  Serial.begin(9600);
  Serial.println("");

  WiFiManager wifiManager;

  pinMode(WiFiResetPin, INPUT_PULLUP);
  if (digitalRead(WiFiResetPin) == 0) {
    // Pin is pulled low.  Reset the WiFi parameters so the config portal is used.
    Serial.println("Resetting WiFi parameters");
    wifiManager.resetSettings();
  }

  // Connect to WiFi.  If network isn't found, put up an AP where user can configure it
  wifiManager.autoConnect("PowerManager");

  if (MDNS.begin(hostname)) {
    Serial.println("MDNS responder started");
  }

  server.on(F("/"), handleHomepage);

  server.on(UriRegex("^\\/switch\\/([0-9]+)\\/(on|off)$"), handleSwitch);
  server.on(UriRegex("^\\/sniff$"), handleSniff);

  server.begin();
  Serial.println("HTTP server started");

  mySwitch.setRepeatTransmit(20);
  mySwitch.enableTransmit(RCTransmissionPin);
  mySwitch.enableReceive(digitalPinToInterrupt(RCReceivePin));
}

void loop(void) {
  server.handleClient();
  MDNS.update();

  if (mySwitch.available()) {
    // Save values for snoop page
    rcvValue = mySwitch.getReceivedValue();
    rcvLength = mySwitch.getReceivedBitlength();
    rcvProtocol = mySwitch.getReceivedProtocol();

    Serial.print("Received ");
    Serial.print( rcvValue );
    Serial.print(" / ");
    Serial.print( rcvLength );
    Serial.print("bit ");
    Serial.print("Protocol: ");
    Serial.println( rcvProtocol );

    mySwitch.resetAvailable();
  }
}
