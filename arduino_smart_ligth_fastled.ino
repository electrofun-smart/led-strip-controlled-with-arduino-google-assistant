#include <SPI.h>
#include <Ethernet.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <FastLED.h>

const char *thehostname = "postman.cloudmqtt.com"; // your mqtt broker
const char *user = "";                     // your mqtt user
const char *user_password = "";        // your mqtt password
const char *id = "Arduino-SmartLigth";

#define LED_TYPE WS2811
#define COLOR_ORDER BRG
#define MACADDRESS 0x00, 0x01, 0x02, 0x03, 0x04, 0x05
#define MYIPADDR 192, 168, 0, 177
#define MYIPMASK 255, 255, 255, 0
#define MYDNS 189, 7, 48, 61
#define MYGW 192, 168, 0, 1
#define UPDATES_PER_SECOND 100
#define NUM_LEDS 30

CRGB leds[30];
MQTTClient client;
EthernetClient ethClient;

boolean connected = false;
boolean myinit = true;
boolean effect;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

unsigned long lastMillis = 0;
int brightness;
long spectrumRgb;

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void setup()
{
    // setup serial communication
    Serial.begin(115200);
    connected = false;

    // try to congifure using IP address instead of DHCP:
    uint8_t mac[6] = {MACADDRESS};
    uint8_t myIP[4] = {MYIPADDR};
    uint8_t myMASK[4] = {MYIPMASK};
    uint8_t myDNS[4] = {MYDNS};
    uint8_t myGW[4] = {MYGW};
    Ethernet.begin(mac, myIP, myDNS, myGW, myMASK);

    // setup mqtt client
    delay(1000);
    client.begin(thehostname, 16157, ethClient);

    client.onMessage(messageReceived);
    connect();
    brightness = 64;
    spectrumRgb = 16777200;
    FastLED.addLeds<LED_TYPE, 3, COLOR_ORDER>(leds, 30).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(64);

    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
    effect = false;
    Serial.println(("MQTT client configured"));
}

void connect()
{

    Serial.print("\nconnecting…");
    while (!client.connect(id, user, user_password))
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nconnected!");
    client.subscribe("1wae-client"); //change the value 1wae to your device id, check the value in Cloud Firestore database
}

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(payload);
    String deviceOn = json["on"];
    int rec_brightness = json["brightness"];
    long rec_spectrumRgb = json["spectrumRgb"];
    long my_spectrumRgb;

    if (deviceOn == "true")
    {
        Serial.println("onOff = true");
        brightness = 64;
        Serial.println(spectrumRgb);
        effect = false;
    }

    if (rec_brightness)
    {
        brightness = map(rec_brightness, 0, 100, 0, 255);
        Serial.println("brightness");
        Serial.println(brightness);
        effect = false;
    }

    if (rec_spectrumRgb)
    {
        spectrumRgb = rec_spectrumRgb;
        if (spectrumRgb == 9498256)
        {
            effect = true;
        }
        else
        {
            effect = false;
        }
    }

    if (deviceOn == "false")
    {
        Serial.println("onOff = false");
        my_spectrumRgb = CRGB::Black;
        effect = false;
    }
    else
    {
        my_spectrumRgb = spectrumRgb;
    }

    for (int i = 0; i < 30; i++)
    {
        leds[i] = my_spectrumRgb;
    }
}

void loop()
{
    client.loop();
    if (!client.connected())
    {
        connect();
    }

    if (effect)
    {
        ChangePalettePeriodically();

        static uint8_t startIndex = 0;
        startIndex = startIndex + 1; /* motion speed */

        FillLEDsFromPaletteColors(startIndex);

        FastLED.show();
        FastLED.delay(1000 / UPDATES_PER_SECOND);
    }
    else
    {
        FastLED.show();
        FastLED.setBrightness(brightness);
        FastLED.delay(500);
    }
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
    uint8_t brightness = 255;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;

    if (lastSecond != secondHand)
    {
        lastSecond = secondHand;
        if (secondHand == 0)
        {
            currentPalette = RainbowColors_p;
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 10)
        {
            currentPalette = RainbowStripeColors_p;
            currentBlending = NOBLEND;
        }
        if (secondHand == 15)
        {
            currentPalette = RainbowStripeColors_p;
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 20)
        {
            SetupPurpleAndGreenPalette();
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 25)
        {
            SetupTotallyRandomPalette();
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 30)
        {
            SetupBlackAndWhiteStripedPalette();
            currentBlending = NOBLEND;
        }
        if (secondHand == 35)
        {
            SetupBlackAndWhiteStripedPalette();
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 40)
        {
            currentPalette = CloudColors_p;
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 45)
        {
            currentPalette = PartyColors_p;
            currentBlending = LINEARBLEND;
        }
        if (secondHand == 50)
        {
            currentPalette = myRedWhiteBluePalette_p;
            currentBlending = NOBLEND;
        }
        if (secondHand == 55)
        {
            currentPalette = myRedWhiteBluePalette_p;
            currentBlending = LINEARBLEND;
        }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for (int i = 0; i < 16; i++)
    {
        currentPalette[i] = CHSV(random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV(HUE_PURPLE, 255, 255);
    CRGB green = CHSV(HUE_GREEN, 255, 255);
    CRGB black = CRGB::Black;

    currentPalette = CRGBPalette16(
        green, green, black, black,
        purple, purple, black, black,
        green, green, black, black,
        purple, purple, black, black);
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
    {
        CRGB::Red,
        CRGB::Gray, // 'white' is too bright compared to red and blue
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Red,
        CRGB::Gray,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Blue,
        CRGB::Black,
        CRGB::Black

};
