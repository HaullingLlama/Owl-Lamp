#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#include <ArduinoOTA.h>

#include "enums.h"

#define NUM_LEDS 3
#define DATA_PIN 13
#define BRITNESS 125

CRGB leds[NUM_LEDS];

const char* ssid = "GForce";
const char* pass = "Christmas1225";
int keyIndex = 0;
Animations selected = SOLID;
int led1Color = 0;
int led2Color = 0;
int led3Color = 0;
CRGB ledColors[3];
JsonDocument colorArray;

uint8_t gHue = 0;

int status = WL_IDLE_STATUS;
WebServer server(80);

void setSolidColor() {
  Serial.println("setSolidColor");
  int r, g, b;
  int i = 0;

  JsonObject color = colorArray.as<JsonObject>();
  for (JsonPair kv : color) {
    Serial.print(kv.key().c_str());
    Serial.print(": ");
    Serial.println(kv.value().as<const char*>());
    String thisColor = kv.value();
    sscanf(thisColor.c_str(), "#%02x%02x%02x", &r, &g, &b);
    leds[i] = CRGB(r, g, b);
    ledColors[i] = CRGB(r, g, b);
    i++;
  }
  
  FastLED.show();
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void setAnimation(String animation) {
  if (animation == "solid") {
    // solid animation code here
    selected = SOLID;
  } else if (animation == "flap") {
    // blink animation code here
    selected = FLAP;
  } else if (animation == "fade") {
    // fade animation code here
    selected = FADE;
  } else if (animation == "rainbow") {
    // rainbow animation code here
    selected = RAINBOW;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  FastLED.setBrightness(BRITNESS);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("Connected to: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.begin();

  server.on("/", HTTP_GET, []() {
      server.send(200, "text/html",
        "<html>"
          "<body>"
            "<h1>LED Control</h1>"
            "<button onclick=\"setOff()\">Turn Off</button>"
            "<br>"
            "<br>"
            "<input type=\"color\" id=\"led1\">"
            "<input type=\"color\" id=\"led2\">"
            "<input type=\"color\" id=\"led3\">"
            "<br>"
            "<br>"
            "<button onclick=\"setColor()\">Set Color</button>"
            "<br>"
            "<br>"
            "<button onclick=\"setWhite()\">Set White</button>"
            "<br>"
            "<br>"
            "<select id=\"animation\">"
              "<option value=\"solid\">Solid</option>"
              "<option value=\"flap\">Flap</option>"
              "<option value=\"fade\">Fade</option>"
              "<option value=\"rainbow\">Rainbow</option>"
            "</select>"
            "<br>"
            "<br>"
            "<button onclick=\"setAnimation()\">Set Animation</button>"
            "<br>"
            "<br>"
            "<input type=\"number\" id=\"brightness\" min=\"0\" max=\"255\" value=\"128\">"
            "<button onclick=\"setBrightness()\">Set Brightness</button>"
            "<script>"
              "function setOff(){"
                "fetch(\"/off\");"
              "}"
              "function setColor(){"
                "var led1Color = document.getElementById(\"led1\").value;"
                "var led2Color = document.getElementById(\"led2\").value;"
                "var led3Color = document.getElementById(\"led3\").value;"
                "fetch(\"/set-color\", {"
                  "method: \"POST\", body: JSON.stringify({"
                    "led1Color: led1Color, led2Color: led2Color, led3Color: led3Color"
                  "})"
                "});"
              "}"
              "function setWhite(){"
                "var led1Color = '#ffffff';"
                "var led2Color = '#ffffff';"
                "var led3Color = '#ffffff';"
                "fetch(\"/set-color\", {"
                  "method: \"POST\", body: JSON.stringify({"
                    "led1Color: led1Color, led2Color: led2Color, led3Color: led3Color"
                  "})"
                "});"
              "}"
              "function setAnimation(){"
                "var animation = document.getElementById(\"animation\").value;"
                "fetch(\"/set-animation\", {"
                  "method: \"POST\", body: JSON.stringify({"
                    "animation: animation"
                  "})"
                "});"
              "}"
              "function setBrightness(){"
                "var brightness = document.getElementById(\"brightness\").value;"
                "fetch(\"/set-brightness\", {"
                  "method: \"POST\", body: JSON.stringify({"
                    "brightness: brightness"
                  "})"
                "});"
              "}"
            "</script>"
          "</body>"
        "</html>");
  });

  server.on("/on", HTTP_GET, []() {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::White; // set the LEDs to white
    }
    FastLED.show();
    server.send(200, "text/plain", "LEDs turned on");
  });

  server.on("/off", HTTP_GET, []() {
    setAnimation("solid");
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black; // set the LEDs to black (off)
    }
    FastLED.show();
    server.send(200, "text/plain", "LEDs turned off");
  });
  
  server.on("/set-color", HTTP_POST, []() {
    Serial.println("set-color");
    String body = server.arg("plain");
    JsonDocument json;
    deserializeJson(json, body);

    serializeJson(json, Serial);
    colorArray = json;
    selected = SOLID;
    
    setSolidColor();

    server.send(200, "text/plain", "Colors set");
  });

  server.on("/set-animation", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument json;
    deserializeJson(json, body);
    String animation = json["animation"];
    setAnimation(animation);
    server.send(200, "text/plain", "Animation set");
  });

  server.on("/set-brightness", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument json;
    deserializeJson(json, body);
    uint8_t brightness = json["brightness"];
    FastLED.setBrightness(brightness);
    server.send(200, "text/plain", "Brightness set");
  });

  server.begin();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void rainbow_wave(uint8_t thisSpeed, uint8_t deltaHue) {     // The fill_rainbow call doesn't support brightness levels.
 
  uint8_t thisHue = beatsin8(thisSpeed,0,255);                // A simple rainbow wave.
  fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);            // Use FastLED's fill_rainbow routine.
 
}                                 // Initialize our LED array.

CRGB clr1;
CRGB clr2;
uint8_t speed;
uint8_t loc1;
uint8_t loc2;
uint8_t ran1;
uint8_t ran2;

void blendwave() {

  speed = beatsin8(6,0,255);

  clr1 = blend(CHSV(beatsin8(3,0,255),255,255), CHSV(beatsin8(4,0,255),255,255), speed);
  clr2 = blend(CHSV(beatsin8(4,0,255),255,255), CHSV(beatsin8(3,0,255),255,255), speed);

  loc1 = beatsin8(10,0,NUM_LEDS-1);
  
  fill_gradient_RGB(leds, 0, clr2, loc1, clr1);
  fill_gradient_RGB(leds, loc1, clr2, NUM_LEDS-1, clr1);

} // blendwave()

void cylon(CRGB c, int width, int speed){
  // First slide the leds in one direction
  for(int i = 0; i <= NUM_LEDS-width; i++) {
    for(int j=0; j<width; j++){
      leds[i+j] = ledColors[i];
    }

    FastLED.show();

    // now that we've shown the leds, reset to black for next loop
    for(int j=0; j<width; j++){
      leds[i+j] = CRGB::Black;
    }
    delay(speed);
  }

  // Now go in the other direction.  
  for(int i = NUM_LEDS-width; i >= 0; i--) {
    for(int j=0; j<width; j++){
      leds[i+j] = ledColors[i];
    }
    FastLED.show();
    for(int j=0; j<width; j++){
      leds[i+j] = CRGB::Black;
    }

    delay(speed);
  }
}


void loop() {

  ArduinoOTA.handle();
  server.handleClient();

  switch (selected) {
    case SOLID:
      // code for SOLID animation
      break;
    case FLAP:
      cylon(CRGB::White, 1, 300);
      break;
    case FADE:
      blendwave();
      FastLED.show();
      break;
    case RAINBOW:
      // code for RAINBOW animation
      // fill_rainbow( leds, NUM_LEDS, gHue, 7);
      rainbow_wave( 5, 7);
      FastLED.show();
      break;
    default:
      // code for default case (if none of the above match)
      break;
  }
}