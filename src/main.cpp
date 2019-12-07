#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <CircularBuffer.h>
#include <ESPAsyncWiFiManager.h>
#include "ArduinoJson.h"

#define NUM_LEDS 241
#define LED_PIN 23

struct effect{
    int effect;
    int count;
    double intensity;
    double speed;
    double r;
    double g;
    double b;
};

CircularBuffer<effect*, 20> effect_queue;

CRGB leds[NUM_LEDS];
AsyncWebServer server(80);

AsyncWebServer setup_server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);

StaticJsonDocument<1024> json;

void effect_flash(effect* options);
void effect_strobe(effect *options);

void add_to_queue(JsonObject effect_to_add);

void post_effect(AsyncWebServerRequest *request);
void error_404(AsyncWebServerRequest *request);

void setup() {
  // setup serial
  Serial.begin(115200);

  //setup fastled
  FastLED.addLeds<WS2812B, LED_PIN>(leds, NUM_LEDS);

  //connect to wifi
  wifiManager.autoConnect("CageLamp");

  Serial.println("");
  Serial.print("Connected. IP=");
  Serial.println(WiFi.localIP());


  server.on("/effect", HTTP_POST, post_effect);
  server.onNotFound(error_404);

  server.begin();
}
double i;
void loop() {
  if(!effect_queue.isEmpty()){
    effect* next_effect = effect_queue.shift();
    Serial.printf("playing %d \n", next_effect->effect);
    switch (next_effect->effect){
      case(1):
        effect_flash(next_effect);
        break;
      case(2):
        effect_strobe(next_effect);
        break;
    }
    free(next_effect);
  }
  delay(10);
}

void post_effect(AsyncWebServerRequest * request){
  if(request->hasParam("body", true)){
    DeserializationError error = deserializeJson(json, request->getParam("body", true)->value());
    if (error) {
      request->send(400, "text/plain", "invalid json");
      return;
    }

    if(effect_queue.isFull()) {
      request->send(400, "text/plain", "queue full");
      return;
    }

    if(json.is<JsonArray>()){
      JsonArray arr = json.as<JsonArray>();
      for (JsonObject effect_to_add : arr) {
        add_to_queue(effect_to_add);
      }
    }else{
      add_to_queue(json.as<JsonObject>());
    }

    request->send(200, "text/plain", "success");
  }else{
    request->send(403, "text/plain", "no body");
  }
  
}

void add_to_queue(JsonObject effect_to_add){
  effect* new_effect = (effect*) malloc(sizeof (struct effect));
  
  new_effect->effect = effect_to_add["effect"].as<int>();
  effect_queue.push(new_effect);

  if(json.containsKey("count")){
    new_effect->count = json["count"].as<int>();
  }else    {
    new_effect->count = 1;
  }

  if(json.containsKey("intensity")){
    new_effect->intensity = json["intensity"].as<double>();
  }else    {
    new_effect->intensity = 1.0;
  }

  if(json.containsKey("speed")){
    new_effect->speed = json["speed"].as<double>();
  }else    {
    new_effect->speed = 1.0;
  }

  if(json.containsKey("color")){
    JsonObject color = json["color"].as<JsonObject>();
    new_effect->r = color["r"].as<double>();
    new_effect->g = color["g"].as<double>();
    new_effect->b = color["b"].as<double>();
  }else    {
    new_effect->r = 1.0;
    new_effect->g = 0;
    new_effect->b = 0;
  }

  // Serial.printf("adding effect %d with: \n", new_effect->effect);
  // Serial.printf(" count: %d\n", new_effect->count);
  // Serial.printf(" intensity: %f\n", new_effect->intensity);
  // Serial.printf(" speed: %f\n", new_effect->speed);
  // Serial.printf(" r: %f\n", new_effect->r);
  // Serial.printf(" g: %f\n", new_effect->g);
  // Serial.printf(" b: %f\n", new_effect->b);
  // Serial.printf("\n");
}

void effect_flash(effect *options){
  for (int i=0;i<options->count;i++){
    for (double j=0;j<3.1415; j+=0.1 * options->speed ){
      double brightness = sin(j);
      double red = options->r  * brightness;
      double green = options->g * brightness;
      double blue =  options->b * brightness;

      double normalize = max(red, max(green, blue));
      if(normalize > 1.0){
        red /= normalize;
        green /= normalize;
        blue /= normalize;
      }

      red *= options->intensity;
      green *= options->intensity;
      blue *= options->intensity;

      double compensation = red + green + blue;
      if(compensation > 1.0){
        red /= compensation;
        green /= compensation;
        blue /= compensation;
      }

      FastLED.showColor(CRGB((int) (green * 255.0), (int) (red * 255.0), (int) (blue * 255.0)));
      delay(10);
    } 
  FastLED.showColor(CRGB(0, 0, 0));
  delay(50.0 / options->speed );
  }
  FastLED.showColor(CRGB(0, 0, 0));
}

void effect_strobe(effect *options){
  double red = options->r;
  double green = options->g;
  double blue =  options->b;

  double normalize = max(red, max(green, blue));
  if(normalize > 1.0){
    red /= normalize;
    green /= normalize;
    blue /= normalize;
  }

  red *= options->intensity;
  green *= options->intensity;
  blue *= options->intensity;

  double compensation = red + green + blue;
  if(compensation > 1.0){
    red /= compensation;
    green /= compensation;
    blue /= compensation;
  }

  for (int i=0;i<options->count;i++){
      FastLED.showColor(CRGB((int) (green * 255.0), (int) (red * 255.0), (int) (blue * 255.0)));
      delay(100 / options->speed);
      FastLED.showColor(CRGB(0, 0, 0));
      delay(100 / options->speed); 
  }
  FastLED.showColor(CRGB(0, 0, 0));
}

void error_404(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
