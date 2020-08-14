/*
 * IRremoteESP8266: IRServer - demonstrates sending IR codes controlled from a webserver
 * Version 0.3 May, 2019
 * Version 0.2 June, 2017
 * Copyright 2015 Mark Szabo
 * Copyright 2019 David Conran
 *
 * An IR LED circuit *MUST* be connected to the ESP on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif  // ESP8266
#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif  // ESP32
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFiClient.h>
#include <credentials.h>
#include "IR_Codes_Samsung.h"
#define SEND_SAMSUNG 1
#include <PubSubClient.h>
// OTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define ARDUINO_OTA_PW "YOUROTAPW"

const char* kSsid = WLAN_SSID;
const char* kPassword = WLAN_PASS;
MDNSResponder mdns;

#if defined(ESP8266)
ESP8266WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "GoogleRemote"
#endif  // ESP8266
#if defined(ESP32)
WebServer server(80);
#undef HOSTNAME
#define HOSTNAME "esp32"
#endif  // ESP32
const char* MQTT_BROKER = AIO_SERVER;
int MQTT_PORT = AIO_SERVERPORT;
const char* MQTT_CLIENT = HOSTNAME;
const char* MQTT_USER = AIO_USERNAME;
const char* MQTT_PW = AIO_KEY;
#define MQTT_CMDTOPIC "google_remote/cmd"

  WiFiClient espclient;
  PubSubClient client(espclient);
long lastMsg = 0;
char msg[50];
int value = 0;



const uint16_t kIrLed = 0;  // ESP GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed, true, true);  // Set the GPIO to be used to sending the message.

void handleRoot() {
  server.send(200, "text/html",
              "<html> "\
    "   <head><title>" HOSTNAME "</title> "\
    "  <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=yes\"> "\
    "  </head> "\ 
    "   <style> "\ 
    "   .button { "\ 
    "   background-color: #4CAF50; /* Green */ "\ 
    "    border: none;   "\ 
    "    color: white;  "\ 
    "     padding: 15px 32px; "\ 
    "     text-align: center; "\ 
    "     text-decoration: none; "\ 
    "     display: inline-block; "\ 
    "     font-size: 16px;"\ 
  "       "\ 
    "    padding: 14px 40px;"\ 
    "   }"\ 
  "     td a {"\ 
  "         display:table;"\ 
  "         background: orange;"\ 
  "         width:50%;"\ 
  "         border-spacing:10px/*border-spacing  instead of padding*/"\ 
  "     }"\ 
  "     td {"\ 
  "         padding:5;"\ 
  "         border-spacing:0"\ 
  "     }"\ 
    "  .button2 {background-color: #008CBA;} /* Blue */ "\ 
    "   .button3 {background-color: #f44336;} /* Red */  "\ 
    "   .button4 {background-color: #e7e7e7; color: black;} /* Gray */  "\ 
    "   .button5 {background-color: #555555;} /* Black */ "\ 
    "   .button6 {background-color: #FFFF00;} /* Yellow */  "\ 
    "   </style>"\ 
    "   <body> "\ 
    "     <h1>" HOSTNAME "</h1> "\ 
    "     <table style=\"width:60%\" cellpadding=5 cellspacing=30> "\ 
    "     <tr></tr> "\ 
    "       <tr> "\ 
    "<td colspan=3><a href=\"ir?code=POWERON\" class=\"button button3\">POWER ON</a></td> "\ 
    "       </tr>"\ 
  "       <tr></tr>"\ 
  "       <tr></tr>"\ 
  "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_UP\" class=\"button button4\">UP</a></td> "\ 
    "<td><a href=\"ir?code=SOURCE\" class=\"button button4\"><b>SOURCE</b></a></td> "\ 
    "<td><a href=\"ir?code=BTN_DOWN\" class=\"button button4\">DOWN</a></td> "\ 
    "       </tr> "\ 
  "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_VOLUP\" class=\"button button2\">VOL+</a></td> "\ 
    "<td><a href=\"ir?code=BTN_MUTE\" class=\"button button4\"><b>MUTE</b></a></td> "\ 
    "<td><a href=\"ir?code=BTN_VOLDN\" class=\"button button2\">VOL-</a></td> "\ 
    "       </tr>"\ 
  "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_PROGUP\" class=\"button button2\">PROG+</a></td> "\ 
    "<td><a href=\"ir?code=BTN_CHLIST\" class=\"button button4\"><b>CHLIST</b></a></td> "\ 
    "<td><a href=\"ir?code=BTN_PROGDN\" class=\"button button2\">PROG-</a></td> "\ 
    "       </tr>"\ 
  "       <tr></tr>"\ 
  "       <tr></tr>"\ 
    "       <tr> "\ 
    "<td></td> "\ 
    "<td><a href=\"ir?code=BTN_UP\" class=\"button button4\">UP</a></td> "\ 
    "<td></td> "\ 
    "       </tr> "\ 
  "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_LEFT\" class=\"button button4\">LEFT</a></td> "\ 
    "<td><a href=\"ir?code=BTN_MENU\" class=\"button button4\"><b>MENU</b></a></td> "\ 
    "<td><a href=\"ir?code=BTN_RIGHT\" class=\"button button4\">RIGHT</a></td> "\ 
    "       </tr> "\ 
  "       <tr> "\ 
    "<td></td> "\ 
    "<td><a href=\"ir?code=BTN_DOWN\" class=\"button button4\">DOWN</a></td> "\ 
    "<td></td> "\ 
    "       </tr>"\ 
  "       <tr></tr>"\ 
  "       <tr></tr>"\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_TOOLS\" class=\"button button4\">TOOLS</a></td> "\ 
    "<td><a href=\"ir?code=BTN_TTXMIX\" class=\"button button4\">TTXMIX</a></td> "\ 
    "<td><a href=\"ir?code=BTN_INFO\" class=\"button button4\">INFO</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_CONTENT\" class=\"button button4\">CONTENT</a></td> "\ 
    "<td><a href=\"ir?code=BTN_OKAY\" class=\"button button4\">ENTER TBD</a></td> "\ 
    "<td><a href=\"ir?code=BTN_GUIDE\" class=\"button button4\">GUIDE</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_RETURN\" class=\"button button4\">RETURN</a></td> "\ 
    "<td><a href=\"ir?code=BTN_PRECH\" class=\"button button4\">PRECH</a></td> "\ 
    "<td><a href=\"ir?code=BTN_EXIT\" class=\"button button4\">EXIT</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_A\" class=\"button button3\">A</a></td> "\ 
    "<td><a href=\"ir?code=BTN_B\" class=\"button\">B</a><a href=\"ir?code=BTN_C\" class=\"button button6\">C</a></td> "\ 
    "<td><a href=\"ir?code=BTN_D\" class=\"button button2\">D</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_EMANUAL\" class=\"button button4\">EMANUAL</a></td> "\ 
    "<td><a href=\"ir?code=BTN_PSIZE\" class=\"button button4\">PSIZE</a></td> "\ 
    "<td><a href=\"ir?code=BTN_ADSUBT\" class=\"button button4\">ADSUBT</a></td> "\ 
    "       </tr> "\ 
  "       <tr></tr>"\ 
  "       <tr></tr>"\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_1\" class=\"button button4\">1</a></td> "\ 
    "<td><a href=\"ir?code=BTN_2\" class=\"button button4\">2</a></td> "\ 
    "<td><a href=\"ir?code=BTN_3\" class=\"button button4\">3</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_4\" class=\"button button4\">4</a></td> "\ 
    "<td><a href=\"ir?code=BTN_5\" class=\"button button4\">5</a></td> "\ 
    "<td><a href=\"ir?code=BTN_6\" class=\"button button4\">6</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td><a href=\"ir?code=BTN_7\" class=\"button button4\">7</a></td> "\ 
    "<td><a href=\"ir?code=BTN_8\" class=\"button button4\">8</a></td> "\ 
    "<td><a href=\"ir?code=BTN_9\" class=\"button button4\">9</a></td> "\ 
    "       </tr> "\ 
    "       <tr> "\ 
    "<td></td> "\ 
    "<td><a href=\"ir?code=BTN_0\" class=\"button button4\">0</a></td> "\ 
    "<td></td> "\ 
    "       </tr> "\ 
    "     </table> "\ 
    "   </body> "\ 
    " </html>");
}

void handleIr() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      //uint32_t code = strtoul(server.arg(i).c_str(), NULL, 10);
      String code = server.arg(i);
#if SEND_SAMSUNG
      if ( code == "POWERON") {
  irsend.sendSAMSUNG(POWERON,32);
  //Serial.println("Power ON send: " + code);
  }
      else if ( code == "SOURCE") {
        irsend.sendSAMSUNG(SOURCE,32);
        }
      else if ( code == "BTN_1") {
        irsend.sendSAMSUNG(BTN_1,32);
        }
      else if ( code == "BTN_2") {
        irsend.sendSAMSUNG(BTN_2,32);
        }
      else if ( code == "BTN_3") {
        irsend.sendSAMSUNG(BTN_3,32);
        }
      else if ( code == "BTN_4") {
        irsend.sendSAMSUNG(BTN_4,32);
        }
      else if ( code == "BTN_5") {
        irsend.sendSAMSUNG(BTN_5,32);
        }
      else if ( code == "BTN_6") {
        irsend.sendSAMSUNG(BTN_6,32);
        }
      else if ( code == "BTN_7") {
        irsend.sendSAMSUNG(BTN_7,32);
        }
      else if ( code == "BTN_8") {
        irsend.sendSAMSUNG(BTN_8,32);
        }
      else if ( code == "BTN_9") {
        irsend.sendSAMSUNG(BTN_9,32);
        }
      else if ( code == "BTN_0") {
        irsend.sendSAMSUNG(BTN_0,32);
        }
      else if ( code == "BTN_TTXMIX") {
        irsend.sendSAMSUNG(BTN_TTXMIX,32);
        }
      else if ( code == "BTN_PRECH") {
        irsend.sendSAMSUNG(BTN_PRECH,32);
        }
      else if ( code == "BTN_MUTE") {
        irsend.sendSAMSUNG(BTN_MUTE,32);
        }
      else if ( code == "BTN_CHLIST") {
        irsend.sendSAMSUNG(BTN_CHLIST,32);
        }
      else if ( code == "BTN_VOLUP") {
        irsend.sendSAMSUNG(BTN_VOLUP,32);
        }
      else if ( code == "BTN_VOLDN") {
        irsend.sendSAMSUNG(BTN_VOLDN,32);
        }
      else if ( code == "BTN_PROGUP") {
        irsend.sendSAMSUNG(BTN_PROGUP,32);
        }
      else if ( code == "BTN_PROGDN") {
        irsend.sendSAMSUNG(BTN_PROGDN,32);
        }
      else if ( code == "BTN_CONTENT") {
        irsend.sendSAMSUNG(BTN_CONTENT,32);
        }
      else if ( code == "BTN_MENU") {
        irsend.sendSAMSUNG(BTN_MENU,32);
        }
      else if ( code == "BTN_GUIDE") {
        irsend.sendSAMSUNG(BTN_GUIDE,32);
        }
      else if ( code == "BTN_TOOLS") {
        irsend.sendSAMSUNG(BTN_TOOLS,32);
        }
      else if ( code == "BTN_INFO") {
        irsend.sendSAMSUNG(BTN_INFO,32);
        }
      else if ( code == "BTN_RETURN") {
        irsend.sendSAMSUNG(BTN_RETURN,32);
        }
      else if ( code == "BTN_EXIT") {
        irsend.sendSAMSUNG(BTN_EXIT,32);
        }
      else if ( code == "BTN_OKAY") {
      irsend.sendSAMSUNG(BTN_OKAY,32);
      }
      else if ( code == "BTN_UP") {
        irsend.sendSAMSUNG(BTN_UP,32);
        }
      else if ( code == "BTN_LEFT") {
        irsend.sendSAMSUNG(BTN_LEFT,32);
        }
      else if ( code == "BTN_RIGHT") {
        irsend.sendSAMSUNG(BTN_RIGHT,32);
        }
      else if ( code == "BTN_DOWN") {
        irsend.sendSAMSUNG(BTN_DOWN,32);
        }
      else if ( code == "BTN_A") {
        irsend.sendSAMSUNG(BTN_A,32);
        }
      else if ( code == "BTN_B") {
        irsend.sendSAMSUNG(BTN_B,32);
        }
      else if ( code == "BTN_C") {
        irsend.sendSAMSUNG(BTN_C,32);
        }
      else if ( code == "BTN_D") {
        irsend.sendSAMSUNG(BTN_D,32);
        }
      else if ( code == "BTN_EMANUAL") {
        irsend.sendSAMSUNG(BTN_EMANUAL,32);
        }
      else if ( code == "BTN_PSIZE") {
        irsend.sendSAMSUNG(BTN_PSIZE,32);
        }
      else if ( code == "BTN_ADSUBT") {
        irsend.sendSAMSUNG(BTN_ADSUBT,32);
        };
#endif
// insert other Encodings here 
    }
  }
  handleRoot();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received message [");
    Serial.print(topic);
    Serial.print("] ");
    char msg[length+1];
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    Serial.println();
 
    msg[length] = '\0';
    Serial.println(msg);
 
    if(strcmp(msg,"POWERON")==0){
            //Serial.println("POWERON Command received");
            irsend.sendSAMSUNG(POWERON,32);
            client.publish(MQTT_CMDTOPIC,"");
    }
    else if(strcmp(msg,"SOURCE")==0){
            irsend.sendSAMSUNG(SOURCE,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_1")==0){
            irsend.sendSAMSUNG(BTN_1,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_2")==0){
            irsend.sendSAMSUNG(BTN_2,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_3")==0){
            irsend.sendSAMSUNG(BTN_3,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_4")==0){
            irsend.sendSAMSUNG(BTN_4,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_5")==0){
            irsend.sendSAMSUNG(BTN_5,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_6")==0){
            irsend.sendSAMSUNG(BTN_6,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_7")==0){
            irsend.sendSAMSUNG(BTN_7,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_8")==0){
            irsend.sendSAMSUNG(BTN_8,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_9")==0){
            irsend.sendSAMSUNG(BTN_9,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_0")==0){
            irsend.sendSAMSUNG(BTN_0,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_TTXMIX")==0){
            irsend.sendSAMSUNG(BTN_TTXMIX,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_PRECH")==0){
            irsend.sendSAMSUNG(BTN_PRECH,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_MUTE")==0){
            irsend.sendSAMSUNG(BTN_MUTE,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_CHLIST")==0){
            irsend.sendSAMSUNG(BTN_CHLIST,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_VOLUP")==0){
            irsend.sendSAMSUNG(BTN_VOLUP,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_VOLDN")==0){
            irsend.sendSAMSUNG(BTN_VOLDN,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_PROGUP")==0){
            irsend.sendSAMSUNG(BTN_PROGUP,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_PROGDN")==0){
            irsend.sendSAMSUNG(BTN_PROGDN,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_CONTENT")==0){
            irsend.sendSAMSUNG(BTN_CONTENT,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_MENU")==0){
            irsend.sendSAMSUNG(BTN_MENU,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_GUIDE")==0){
            irsend.sendSAMSUNG(BTN_GUIDE,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_TOOLS")==0){
            irsend.sendSAMSUNG(BTN_TOOLS,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_INFO")==0){
            irsend.sendSAMSUNG(BTN_INFO,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_RETURN")==0){
            irsend.sendSAMSUNG(BTN_RETURN,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_EXIT")==0){
            irsend.sendSAMSUNG(BTN_EXIT,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_OKAY")==0){
            irsend.sendSAMSUNG(BTN_OKAY,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_UP")==0){
            irsend.sendSAMSUNG(BTN_UP,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_LEFT")==0){
            irsend.sendSAMSUNG(BTN_LEFT,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_RIGHT")==0){
            irsend.sendSAMSUNG(BTN_RIGHT,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_DOWN")==0){
            irsend.sendSAMSUNG(BTN_DOWN,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_A")==0){
            irsend.sendSAMSUNG(BTN_A,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_B")==0){
            irsend.sendSAMSUNG(BTN_B,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_C")==0){
            irsend.sendSAMSUNG(BTN_C,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_D")==0){
            irsend.sendSAMSUNG(BTN_D,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_EMANUAL")==0){
            irsend.sendSAMSUNG(BTN_EMANUAL,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_PSIZE")==0){
            irsend.sendSAMSUNG(BTN_PSIZE,32);
            client.publish(MQTT_CMDTOPIC,"");
      }
    else if(strcmp(msg,"BTN_ADSUBT")==0){
            irsend.sendSAMSUNG(BTN_ADSUBT,32);
            client.publish(MQTT_CMDTOPIC,"");
    }
}

void init_OTA(void) {
    // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname("IR-Remote_ESP8266");

  // No authentication by default
   ArduinoOTA.setPassword(ARDUINO_OTA_PW);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void setup(void) {
  irsend.begin();

  Serial.begin(115200);
  Serial.println("");

// Init WIFI
  WiFi.begin(kSsid, kPassword);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(kSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());
// Init OTA
  init_OTA();
  
// Init MQTT
  client.setServer(MQTT_BROKER, MQTT_PORT);
  // set callback function for mqtt
  client.setCallback(callback);

// Init mDNS
#if defined(ESP8266)
  if (mdns.begin(HOSTNAME, WiFi.localIP())) {
#else  // ESP8266
  if (mdns.begin(HOSTNAME)) {
#endif  // ESP8266
    Serial.println("MDNS responder started");
  }

// Init HTTP Server
  server.on("/", handleRoot);
  server.on("/ir", handleIr);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}


void reconnect() {
  // Reconnect function if mqtt connections gets disconnected
    //while (!client.connected()) {
        Serial.print("Reconnecting MQTT...");
        if (!client.connect(MQTT_CLIENT,MQTT_USER,MQTT_PW)) {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    //}
    client.subscribe(MQTT_CMDTOPIC);
    Serial.println("MQTT Connected...");
}

void loop(void) {
  server.handleClient();

  if (!client.connected()) {
        reconnect();
    }
    // MQTT Loop
    client.loop();

    // OTA Loop
    ArduinoOTA.handle();
}
