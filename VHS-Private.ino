#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"

WiFiClient net;
int wifiStatus = WL_IDLE_STATUS;

MQTTClient client;


#define NUM_LEDS 42
#define LED_PIN 3
#define BRIGHTNESS 255

Adafruit_NeoPixel led_strip = Adafruit_NeoPixel( NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800 );

// Variables
unsigned long lastMillis = 0;
unsigned int loop_counter = 0;
String ops_mode = "";
String new_ops_mode = "";

boolean phoneCycle = false;

void doPhoneAnim() {
  if ( loop_counter % 25 == 0 ) {
    phoneCycle = !phoneCycle;
  }

  if (phoneCycle) {
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 255, 255, 255 );
    }
  } else {
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 0, 0, 0 );
    }
  }

  led_strip.show();
}

boolean alarmCycle = false;

void doAlarmAnim() {
  if ( loop_counter % 25 == 0 ) {
    alarmCycle = !alarmCycle;
  }

  if (alarmCycle) {
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 255, 0, 0 );
    }
  } else {
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 0, 0, 0 );
    }
  }

  led_strip.show();
}

void doBlack() {
  if (loop_counter == 0)
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 0, 0, 0 );
    }
  led_strip.show();
}

void doRed() {
  if (loop_counter == 0)
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 255, 0, 0 );
    }
  led_strip.show();
}

void doOrange() {
  if (loop_counter == 0)
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 255, 128, 0 );
    }
  led_strip.show();
}

void doGreen() {
  if (loop_counter == 0)
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 0, 255, 0 );
    }
  led_strip.show();
}

void doBlue() {
  if (loop_counter == 0)
    for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
      led_strip.setPixelColor( i, 0, 0, 255 );
    }
  led_strip.show();
}

boolean testCycle = false;
void doTest() {
  int modifier = (loop_counter % 255);
  for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
    led_strip.setPixelColor( i, modifier, modifier, modifier );
  }
  led_strip.show();
}

void doUnknown() {
  int modifier = (loop_counter % (255*2));
  if(modifier>255) modifier = (255-(modifier%255));
  for ( int i = 0 ; i < NUM_LEDS ; i++ ) {
    led_strip.setPixelColor( i, modifier, 0, modifier );
  }
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

void connectWiFi() {
  if ( WiFi.status() == WL_CONNECTED)
    return;

  Serial.print("checking wifi...");
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    wifiStatus = WiFi.status();
  }
  Serial.println("");
}

void connectMQTT() {
  if (client.connected())
    return;

  Serial.print("connecting to MQTT...");
  while (!client.connect(HOSTNAME)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");

  Serial.println("connected!");

  client.subscribe(MQTT_TOPIC);
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

void setup() {
  Serial.begin(115200);

  WiFi.hostname(HOSTNAME);
  wifiStatus = WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  client.begin(MQTT_HOST, MQTT_PORT, net);
  client.onMessage(messageReceived);

  connectWiFi();
  connectMQTT();

  led_strip.setBrightness(BRIGHTNESS);
  led_strip.begin();
  led_strip.show(); // Initialize all pixels to 'off'
  doUnknown();
}

int failCounter = 0;

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!WiFi.isConnected()) {
    connectWiFi();
    Serial.println("Bumping failCounter for WiFi connection problem");
    failCounter++;
  } else if (!client.connected()) {
    connectMQTT();
    Serial.println("Bumping failCounter for MQTT connection problem");
    failCounter++;
  } else {
    if(failCounter>0)
      failCounter--;
  }

  if (failCounter > 5)
    ESP.restart();

  if ( new_ops_mode != ops_mode ) {
    loop_counter = 0;
    ops_mode = new_ops_mode;
  }

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
  } else if (ops_mode == "unknown") {
    doUnknown();
  }

  loop_counter++;
}
