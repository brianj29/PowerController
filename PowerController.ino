/*
 * ESP8266 web control for RCSwitch
 * Based on ESP8266 PathArgServer demo and RCSwitch Webserver demo
 *
 * by Brian J. Johnson 11/22/20
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <RCSwitch.h>

//#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

#ifndef STASSID
#define STASSID "your-wifi-id"
#define STAPSK  "your-password"
#endif

const char *hostname = "power";
const char *ssid = STASSID;
const char *password = STAPSK;

// RCSwitch configuration
RCSwitch mySwitch = RCSwitch();
int RCTransmissionPin = D3;

ESP8266WebServer server(80);

/**
 * HTTP Response with homepage
 * TODO:  generate this dynamically based on codes array
 */
String homepage = 
    "<html>\n"
    "<head>\n"
        "<title>Power Switch Control</title>\n"
        "<style>\n"
            "body { font-family: Arial, sans-serif; font-size:12px; }\n"
        "</style>\n"
    "</head>\n"
    "<body>\n"
        "<h1>Power Switch Control</h1>\n"
        "<ul>\n"
            "<li><a href=\"switch/1/on\">Switch #1 on</a></li>\n"
            "<li><a href=\"switch/1/off\">Switch #1 off</a></li>\n"
        "</ul>\n"
        "<ul>\n"
            "<li><a href=\"switch/2/on\">Switch #2 on</a></li>\n"
            "<li><a href=\"switch/2/off\">Switch #2 off</a></li>\n"
        "</ul>\n"
        "<ul>\n"
            "<li><a href=\"switch/3/on\">Switch #3 on</a></li>\n"
            "<li><a href=\"switch/3/off\">Switch #3 off</a></li>\n"
        "</ul>\n"
        "<hr>\n"
        "<a href=\"https://github.com/sui77/rc-switch/\">https://github.com/sui77/rc-switch/</a>\n"
    "</body>\n"
    "</html>\n";

const char *codes[][2] =
 {
  // SMART Electrician switches "(YTP) Model: JQ01TX-03 Date: 1926"
  // Protocol 1
  //         on                           off
  {"011101101111011100000011", "011101101111011100000010"}, // 1
  {"011101101111011100000101", "011101101111011100000100"}, // 2
  {"011101101111011100000111", "011101101111011100000110"}, // 3
};

void handleSwitch() {
  String name = server.pathArg(0);
  String state = server.pathArg(1);
  Serial.println("Got switch: " + name + " state: " + state);

  unsigned num = name.toInt() - 1; // names start at 1, but indices start at 0
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

void setup(void) {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(hostname)) {
    Serial.println("MDNS responder started");
  }

  server.on(F("/"), []() {
    server.send(200, "text/html", homepage);
  });

  //server.on(UriBraces("/users/{}"), []() {
  //  String user = server.pathArg(0);
  //  server.send(200, "text/plain", "User: '" + user + "'");
  //});

  server.on(UriRegex("^\\/switch\\/([0-9]+)\\/(on|off)$"), handleSwitch);

  server.begin();
  Serial.println("HTTP server started");

  mySwitch.enableTransmit( RCTransmissionPin );
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}

#if 0
/*
  A simple RCSwitch/Ethernet/Webserver demo
  
  https://github.com/sui77/rc-switch/
*/

#include <SPI.h>
#include <Ethernet.h>
#include <RCSwitch.h>

// Ethernet configuration
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC Address
uint8_t ip[] = { 192,168,0, 2 };                        // IP Address
EthernetServer server(80);                           // Server Port 80

// RCSwitch configuration
RCSwitch mySwitch = RCSwitch();
int RCTransmissionPin = D3;

// More to do...
// You should also modify the processCommand() and 
// httpResponseHome() functions to fit your needs.



/**
 * Setup
 */
void setup() {
  Serial.begin(9600); // for debugging
  Serial.println("Starting newtork");
  Ethernet.begin(mac, ip);
  Serial.println("Starting server");
  server.begin();
  Serial.println("Enabling transmit");
  mySwitch.enableTransmit( RCTransmissionPin );
  Serial.println("Starting main loop");
}

/**
 * Loop
 */
void loop() {
  char* command = httpServer();
}

/**
 * Command dispatcher
 */
void processCommand(char* command) {
  Serial.println("%s", command);
  if        (strcmp(command, "1-on") == 0) {
    mySwitch.send("011101101111011100000011");
  } else if (strcmp(command, "1-off") == 0) {
    mySwitch.send("011101101111011100000010");
  } else if (strcmp(command, "2-on") == 0) {
    mySwitch.send("011101101111011100000101");
  } else if (strcmp(command, "2-off") == 0) {
    mySwitch.send("011101101111011100000100");
  }
}

/**
 * HTTP Response with homepage
 */
void httpResponseHome(EthernetClient c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: text/html");
  c.println();
  c.println("<html>");
  c.println("<head>");
  c.println(    "<title>RCSwitch Webserver Demo</title>");
  c.println(    "<style>");
  c.println(        "body { font-family: Arial, sans-serif; font-size:12px; }");
  c.println(    "</style>");
  c.println("</head>");
  c.println("<body>");
  c.println(    "<h1>RCSwitch Webserver Demo</h1>");
  c.println(    "<ul>");
  c.println(        "<li><a href=\"./?1-on\">Switch #1 on</a></li>");
  c.println(        "<li><a href=\"./?1-off\">Switch #1 off</a></li>");
  c.println(    "</ul>");
  c.println(    "<ul>");
  c.println(        "<li><a href=\"./?2-on\">Switch #2 on</a></li>");
  c.println(        "<li><a href=\"./?2-off\">Switch #2 off</a></li>");
  c.println(    "</ul>");
  c.println(    "<hr>");
  c.println(    "<a href=\"https://github.com/sui77/rc-switch/\">https://github.com/sui77/rc-switch/</a>");
  c.println("</body>");
  c.println("</html>");
}

/**
 * HTTP Redirect to homepage
 */
void httpResponseRedirect(EthernetClient c) {
  c.println("HTTP/1.1 301 Found");
  c.println("Location: /");
  c.println();
}

/**
 * HTTP Response 414 error
 * Command must not be longer than 30 characters
 **/
void httpResponse414(EthernetClient c) {
  c.println("HTTP/1.1 414 Request URI too long");
  c.println("Content-Type: text/plain");
  c.println();
  c.println("414 Request URI too long");
}

/**
 * Process HTTP requests, parse first request header line and 
 * call processCommand with GET query string (everything after
 * the ? question mark in the URL).
 */
char*  httpServer() {
  EthernetClient client = server.available();
  if (client) {
    char sReturnCommand[32];
    int nCommandPos=-1;
    sReturnCommand[0] = '\0';
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if ((c == '\n') || (c == ' ' && nCommandPos>-1)) {
          sReturnCommand[nCommandPos] = '\0';
          if (strcmp(sReturnCommand, "\0") == 0) {
            httpResponseHome(client);
          } else {
            processCommand(sReturnCommand);
            httpResponseRedirect(client);
          }
          break;
        }
        if (nCommandPos>-1) {
          sReturnCommand[nCommandPos++] = c;
        }
        if (c == '?' && nCommandPos == -1) {
          nCommandPos = 0;
        }
      }
      if (nCommandPos > 30) {
        httpResponse414(client);
        sReturnCommand[0] = '\0';
        break;
      }
    }
    if (nCommandPos!=-1) {
      sReturnCommand[nCommandPos] = '\0';
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    
    return sReturnCommand;
  }
  return '\0';
}
#endif
