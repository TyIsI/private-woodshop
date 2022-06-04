#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "modes.h"

WiFiClient net;
int wifiStatus = WL_IDLE_STATUS;

MQTTClient client;

Adafruit_NeoPixel led_strip = Adafruit_NeoPixel( NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800 );

// Variables
unsigned long lastMillis = 0;
unsigned int loop_counter = 0;
String ops_mode = "";
String new_ops_mode = DEFAULT_MODE;

void fill_strip(int r, int g, int b ) {
  for ( int p = 0 ; p < NUM_LEDS ; p++ ) {
    led_strip.setPixelColor( p, r, g, b );
  }
}

boolean phoneCycle = false;

void doPhoneAnim() {
  if ( loop_counter % 25 == 0 ) {
    phoneCycle = !phoneCycle;
  }

  if (phoneCycle) {
    fill_strip( 255, 255, 255 );
  } else {
    fill_strip( 0, 0, 0 );
  }

  led_strip.show();
}

boolean alarmCycle = false;

void doAlarmAnim() {
  if ( loop_counter % 25 == 0 ) {
    alarmCycle = !alarmCycle;
  }

  if (alarmCycle) {
    fill_strip( 255, 0, 0 );
  } else {
    fill_strip( 0, 0, 0 );
  }

  led_strip.show();
}

void doBlack() {
  if (loop_counter == 0)
    fill_strip( 0, 0, 0 );
  led_strip.show();
}

void doRed() {
  if (loop_counter == 0)
    fill_strip( 255, 0, 0 );
  led_strip.show();
}

void doOrange() {
  if (loop_counter == 0)
    fill_strip( 255, 128, 0 );
  led_strip.show();
}

void doGreen() {
  if (loop_counter == 0)
    fill_strip( 0, 255, 0 );
  led_strip.show();
}

void doBlue() {
  if (loop_counter == 0)
    fill_strip( 0, 0, 255 );
  led_strip.show();
}

boolean testCycle = false;

void doTest() {
  int modifier = (loop_counter % 255);
  fill_strip( modifier, modifier, modifier );
  led_strip.show();
}

void doUnknown() {
  int modifier = (loop_counter % (255 * 2));
  if (modifier > 255) modifier = (255 - (modifier % 255));
  fill_strip( modifier, 0, modifier );
  led_strip.show();
}

int discoPhase = 0;

void doDisco() {
  if (loop_counter % 25 != 0 )
    return;

  if ( discoPhase == 1 ) {
    discoPhase = 0;
  } else {
    discoPhase = 1;
  }

  for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
    if ( i % 2 == discoPhase ) {
      led_strip.setPixelColor( i, random(255), random(255), random(255) );
    } else {
      led_strip.setPixelColor( i, 0, 0, 0 );
    }
  }
  led_strip.show();
}

int knightRiderValues[NUM_LEDS];
int knightRiderPosition = 0;
int knightRiderDirection = 1;

void doKnightRider(int updateInterval, int colorMode) {
  // Reset to default at the start
  if (loop_counter == 0) {
    for (int i = 0 ; i < NUM_LEDS ; i++ ) {
      knightRiderValues[i] = 0;
    }
  }

  // Slow it down
  if (loop_counter % updateInterval != 0) return;

  // Dim LEDs
  for (int i = 0 ; i < NUM_LEDS ; i++ ) {
    if (knightRiderValues[i] > 0) knightRiderValues[i]--;
  }

  // Update position
  if ( (knightRiderPosition + knightRiderDirection) >= NUM_LEDS ) {
    knightRiderDirection = -1;
  } else if ((knightRiderPosition + knightRiderDirection) < 0) {
    knightRiderDirection = 1;
  }

  knightRiderPosition = knightRiderPosition + knightRiderDirection;

  // Light up current position
  knightRiderValues[knightRiderPosition] = 5;

  // Update LEDs
  for (int p = 0 ; p < NUM_LEDS ; p++ ) {
    int m = (knightRiderValues[p] * 51);
    switch(colorMode) {
      case MODE_RED:
      led_strip.setPixelColor( p, m, 0, 0 );
      break;
      case MODE_GREEN:
      led_strip.setPixelColor( p, 0, m, 0 );
      break;
      case MODE_BLUE:
      led_strip.setPixelColor( p, 0, 0, m );
      break;
    }
    
  }
  led_strip.show();
}

int connectWiFiStatus = 0;

boolean connectWiFi() {
  if ( WiFi.status() == WL_CONNECTED) {
    Serial.println("Wireless already connected!");
    connectWiFiStatus = 2;
    return true;
  }

  new_ops_mode = "network";

  Serial.println("Wifi.status() vs WL_CONNECTED: " + String(WiFi.status()) + " / " + String(WL_CONNECTED));

  if ( connectWiFiStatus == 0 ) {
    Serial.println("Connecting wifi...");
    WiFi.reconnect();
    delay(500);
    connectWiFiStatus = 1;
  }

  if ( connectWiFiStatus == 1 ) {
    for ( int i = 0 ; i < 5 ; i++ ) {
      delay(100);
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Wireless connected!");
        connectWiFiStatus = 2;
        return true;
      }
      Serial.println("Trying wifi...");
      wifiStatus = WiFi.status();
    }
  }

  return false;
}

int connectMQTTStatus = 0;

boolean connectMQTT() {
  if (client.connected()) {
    Serial.println("MQTT already connected!");
    connectMQTTStatus = 2;
    return true;
  }

  new_ops_mode = "spacebus";

  if ( connectMQTTStatus == 2)
    connectMQTTStatus = 0;

  if ( connectMQTTStatus < 1 ) {
    Serial.println("Connecting to MQTT...");
    client.connect(HOSTNAME);
    connectMQTTStatus = 1;
  }

  if (connectMQTTStatus == 1) {
    for ( int i = 0 ; i < 25 ; i++ ) {
      if (client.connected()) {
        connectMQTTStatus = 2;
        break;
      } else {
        Serial.println("Trying MQTT...");
      }
    }
  }

  if (client.connected()) {
    Serial.println("MQTT connected!");
    client.subscribe(MQTT_TOPIC);
    connectMQTTStatus = 2;
    new_ops_mode = "waiting";
    return true;
  }

  return false;
}

int watchdogCounter = 0;

void watchDog() {
  // Restart if we hit too many errors
  if (watchdogCounter > 25)
    ESP.restart();

  // Check WiFi
  if (connectWiFi() != true) {
    Serial.println("Bumping watchdogCounter for WiFi connection problem");
    watchdogCounter++;
    return;
  }

  // Check MQTT
  if (connectMQTT() != true) {
    Serial.println("Bumping watchdogCounter for MQTT connection problem");
    watchdogCounter++;
    return;
  }

  watchdogCounter = 0;
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  if ( payload == ops_mode )
    return;

  if ( payload == "off" ) {
    new_ops_mode = "off";
  } else if (payload == "alarm") {
    new_ops_mode = "alarm";
  } else if (payload == "phone") {
    new_ops_mode = "phone";
  } else if (payload == "red") {
    new_ops_mode = "red";
  } else if (payload == "orange") {
    new_ops_mode = "orange";
  } else if (payload == "green") {
    new_ops_mode = "green";
  } else if (payload == "test") {
    new_ops_mode = "test";
  } else if (payload == "disco") {
    new_ops_mode = "disco";
  } else {
    new_ops_mode = "unknown";
  }
}

void doUpdate() {
  if (ops_mode == "off" ) {
    doBlack();
  } else if (ops_mode == "alarm") {
    doAlarmAnim();
  } else if (ops_mode == "phone") {
    doPhoneAnim();
  } else if (ops_mode == "red") {
    doRed();
  } else if (ops_mode == "orange") {
    doOrange();
  } else if (ops_mode == "green") {
    doGreen();
  } else if (ops_mode == "test") {
    doTest();
  } else if (ops_mode == "disco") {
    doDisco();
  } else if (ops_mode == "network") {
    doKnightRider(12, MODE_RED);
  } else if (ops_mode == "spacebus") {
    doKnightRider(6, MODE_BLUE);
  } else if (ops_mode == "waiting") {
    doKnightRider(3, MODE_GREEN);
  } else if (ops_mode == "unknown") {
    doUnknown();
  }

  loop_counter++;
}

void setup() {
  Serial.begin(115200);

  WiFi.hostname(HOSTNAME);
  wifiStatus = WiFi.begin(WIFI_SSID, WIFI_PASS);
  //  WiFi.setAutoConnect(true);
  //  WiFi.setAutoReconnect(true);

  client.begin(MQTT_HOST, MQTT_PORT, net);
  client.onMessage(messageReceived);

  led_strip.setBrightness(BRIGHTNESS);
  led_strip.begin();
  led_strip.show(); // Initialize all pixels to 'off'
  doBlack();

  connectWiFi();
  //  connectMQTT();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if ( loop_counter % WATCHDOG_CYCLE == (int)random(1000) ) watchDog();

  if ( new_ops_mode != ops_mode ) {
    Serial.println("Changing mode from " + ops_mode + " to " + new_ops_mode);
    loop_counter = 0;
    ops_mode = new_ops_mode;
  }

  if ( loop_counter % REPORTING_CYCLE == 0 ) Serial.println("Current mode: " + ops_mode);

  doUpdate();
}
