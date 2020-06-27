#include <WS2812FX.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>

// Constants for WIFI setup
#define wifi_ssid "SSID"
#define wifi_password "PASSWORD"

// Setup for MQTT
#define mqtt_server "192.168.1.201"
#define INCREASE_MODE_TOPIC "/esp/increaseMode"
#define DECREASE_MODE_TOPIC "/esp/decreaseMode"
#define CHANGE_COLOR_TOPIC "/esp/changeColor"
#define INCREASE_BRIGHTNESS_TOPIC "/esp/increaseBrightness"
#define DECREASE_BRIGHTNESS_TOPIC "/esp/decreaseBrightness"
#define SET_BRIGHTNESS_TOPIC "/esp/setBrightness"
#define SPECIFIC_MODE_TOPIC "/esp/specific"
#define SET_SPEED_TOPIC "/esp/setSpeed"

#define CURRENT_MODE_TOPIC "/esp/mode"
#define CURRENT_MODE_STRING_TOPIC "/esp/modeString"
#define CURRENT_BRIGHTNESS_TOPIC "/esp/brightness"
//#define CURRENT_COLOR_TOPIC "/esp/color"
#define CURRENT_SPEED_TOPIC "/esp/speed"

// Setup for LED strips
#define LED_COUNT 288
#define LED_PIN 5
#define TIMER_MS 5000

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int BRIGHTNESS = 10;
int SPEED = 10;

int counter = 1;

int SEG_MULTIPLIER = 1;
int START_INDEX = 0;
int LEDS_PER_PENTAGON = LED_COUNT / MAX_NUM_SEGMENTS;

// Setup for MQTT
WiFiClient espClient;
PubSubClient client(espClient);

uint8_t lastMode = 0;
uint8_t lastBrightness = BRIGHTNESS;
uint8_t lastSpeed = SPEED;
uint32_t lastColor = DEFAULT_COLOR;

long lastMsg = 0;

void setup()
{
  Serial.begin(9600);

  setupWifi();
  setupLedStrips();

  client.setServer(mqtt_server, 1883);

  // Set callback function when recieving a MQTT message
  client.setCallback(callback);

  // Subscribe to MQTT Topics
  client.subscribe(INCREASE_MODE_TOPIC);
  client.subscribe(DECREASE_MODE_TOPIC);
  client.subscribe(CHANGE_COLOR_TOPIC);
  client.subscribe(INCREASE_BRIGHTNESS_TOPIC);
  client.subscribe(DECREASE_BRIGHTNESS_TOPIC);
  client.subscribe(SPECIFIC_MODE_TOPIC);
  client.subscribe(SET_SPEED_TOPIC);
  client.subscribe(SET_BRIGHTNESS_TOPIC);
}

void loop()
{

  ws2812fx.service();

  if (!client.connected())
  {
    reconnect();
  }

  checkIfUpdateAvailable();

  //delay(100);
  client.loop();
}

void changeColor(String newColor)

{

  int x = strtol(newColor.c_str(), NULL, 16);

  Serial.println(x);

  for (int i = 0; i < ws2812fx.getNumSegments(); i++)
  {
    ws2812fx.setColor(i, 0);
  }

  for (int i = 0; i < ws2812fx.getNumSegments(); i++)
  {
    ws2812fx.setColor(i, x);
  }
}

void increaseMode()
{

  //Serial.print(ws2812fx.getMode());
  if (ws2812fx.getMode() == 25)
  {
    ws2812fx.setMode(28);
  }

  if (ws2812fx.getMode() == 55)
  {
    ws2812fx.setMode(59);
  }

  int prevMode = ws2812fx.getMode();
  int nextMode = (prevMode + 1) % ws2812fx.getModeCount();

  for (int i = 0; i < ws2812fx.getNumSegments(); i++)
  {
    ws2812fx.setMode(i, nextMode);
  }
}

void setSpeed(int speed)
{

  for (int i = 0; i < ws2812fx.getNumSegments(); i++)
  {
    ws2812fx.setSpeed(i, speed);
  }
}

void decreaseMode()
{

  if (ws2812fx.getMode() == 0)
  {
    ws2812fx.setMode(56);
  }

  //Serial.print(ws2812fx.getMode());
  if (ws2812fx.getMode() == 29)
  {
    ws2812fx.setMode(26);
  }

  int prevMode = ws2812fx.getMode();
  int nextMode = (prevMode - 1) % ws2812fx.getModeCount();

  for (int i = 0; i < ws2812fx.getNumSegments(); i++)
  {
    ws2812fx.setMode(i, nextMode);
  }
}

void setSpecificMode(int mode)
{
  for (int i = 0; i < ws2812fx.getNumSegments(); i++)
  {
    ws2812fx.setMode(i, mode);
  }
}

void decreaseBrightness(int brightnessToLowerBy)
{

  BRIGHTNESS -= brightnessToLowerBy;

  ws2812fx.setBrightness(BRIGHTNESS);
}

void increaseBrightness(int brightnessToLowerBy)
{

  BRIGHTNESS -= brightnessToLowerBy;

  ws2812fx.setBrightness(BRIGHTNESS);
}

// Send updates to other MQTT client about current settings
void checkIfUpdateAvailable()
{

  if (lastMode != ws2812fx.getMode())
  {

    client.publish(CURRENT_MODE_TOPIC, String(ws2812fx.getMode()).c_str());
    client.publish(CURRENT_MODE_STRING_TOPIC, String(ws2812fx.getModeName(ws2812fx.getMode())).c_str());

    lastMode = ws2812fx.getMode();
  }

  if (lastBrightness != ws2812fx.getBrightness())
  {

    client.publish(CURRENT_BRIGHTNESS_TOPIC, String(ws2812fx.getBrightness()).c_str()); //TODO this has been changed, true has been removed
    lastBrightness = ws2812fx.getBrightness();
  }
 /*
  if (lastSpeed != ws2812fx.getSpeed())
  {

    client.publish(CURRENT_SPEED_TOPIC, String(ws2812fx.getSpeed()).c_str()); //TODO this has been changed, true has been removed
    lastSpeed = ws2812fx.getSpeed();
  }

 
  if (lastColor != ws2812fx.getColor())
  {

    client.publish(CURRENT_COLOR_TOPIC, String(ws2812fx.getColor()).c_str(), true);
    lastColor = ws2812fx.getColor();
  }
  */
}
// Callback for when recieving a MQTT message
void callback(char *topic, byte *message, unsigned int length)
{

  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  Serial.print("Message arrived in topic: ");

  String topicString = String(topic);
  Serial.println(topicString);

  if (topicString.equals(INCREASE_MODE_TOPIC))
  {
    increaseMode();
  }
  else if (topicString.equals(DECREASE_MODE_TOPIC))
  {
    decreaseMode();
  }
  else if (topicString.equals(SPECIFIC_MODE_TOPIC))
  {
    setSpecificMode(messageTemp.toInt());
  }
  else if (topicString.equals(INCREASE_BRIGHTNESS_TOPIC))
  {
    increaseBrightness(10);
  }
  else if (topicString.equals(DECREASE_BRIGHTNESS_TOPIC))
  {
    decreaseBrightness(10);
  }
  else if (topicString.equals(SET_BRIGHTNESS_TOPIC))
  {
    BRIGHTNESS = messageTemp.toInt();
    ws2812fx.setBrightness(BRIGHTNESS);
    Serial.print("BRIGHTNESS ");
    Serial.println(BRIGHTNESS);
  }
  else if (topicString.equals(CHANGE_COLOR_TOPIC))
  {

    changeColor(messageTemp);
  }
  else if (topicString.equals(SET_SPEED_TOPIC))
  {

    setSpeed(messageTemp.toInt());
  }

  Serial.print("Message:");

  Serial.println(messageTemp);
  Serial.println("-----------------------");
}

void setupLedStrips()
{
  ws2812fx.init();
  ws2812fx.setBrightness(BRIGHTNESS);
  ws2812fx.setSpeed(SPEED);
  ws2812fx.start();

  for (int i = 0; i < MAX_NUM_SEGMENTS; i++)
  {
    int END_INDEX = START_INDEX + LEDS_PER_PENTAGON;

    ws2812fx.setSegment(i, START_INDEX, END_INDEX, FX_MODE_BREATH, DEFAULT_COLOR, 1000, false);

    START_INDEX += LEDS_PER_PENTAGON;

    Serial.println(START_INDEX);
  }
}

void setupWifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  client.subscribe(INCREASE_MODE_TOPIC);
  client.subscribe(DECREASE_MODE_TOPIC);
  client.subscribe(CHANGE_COLOR_TOPIC);
  client.subscribe(INCREASE_BRIGHTNESS_TOPIC);
  client.subscribe(DECREASE_BRIGHTNESS_TOPIC);
  client.subscribe(SPECIFIC_MODE_TOPIC);
  client.subscribe(SET_SPEED_TOPIC);
  client.subscribe(SET_BRIGHTNESS_TOPIC);
}

// Helper methods
unsigned long colorIntsToUnsignedLong(uint32_t red, uint32_t green, uint32_t blue)
{

  // as a null-terminated char array (string)
  char rgbTxt[7];
  byte_to_str(&rgbTxt[0], red);
  byte_to_str(&rgbTxt[2], green);
  byte_to_str(&rgbTxt[4], blue);
  rgbTxt[6] = '\0';

  String rgbTxtResult = rgbTxt;

  return strtoul(rgbTxt, nullptr, HEX);
}

void byte_to_str(char *buff, uint8_t val)
{ // convert an 8-bit byte to a string of 2 hexadecimal characters
  buff[0] = nibble_to_hex(val >> 4);
  buff[1] = nibble_to_hex(val);
}

char nibble_to_hex(uint8_t nibble)
{ // convert a 4-bit nibble to a hexadecimal character
  nibble &= 0xF;
  return nibble > 9 ? nibble - 10 + 'A' : nibble + '0';
}