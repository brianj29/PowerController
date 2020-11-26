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
int RCTransmissionPin = D3; // output to transmitter
int WiFiResetPin = D1;      // input, pull low to reset WiFi parameters

RCSwitch mySwitch = RCSwitch();
ESP8266WebServer server(80);

const char *codes[][3] =
 {
  // SMART Electrician switches "(YTP) Model: JQ01TX-03 Date: 1926"
  // Protocol 1
  //         on                           off
  {"011101101111011100000011", "011101101111011100000010", "Kitchen amp"},     // 1
  {"011101101111011100000101", "011101101111011100000100", "Living room amp"}, // 2
  {"011101101111011100000111", "011101101111011100000110", "Switch 3"},        // 3
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

  server.begin();
  Serial.println("HTTP server started");

  mySwitch.enableTransmit( RCTransmissionPin );
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
