/****************************************************************************
 * NeoPixel on ESP8266 + ArduinoOTA + WiFiManager
 *  Project for Kevin's "busy board" for Christmas: ESP8266 running a
 *   NeoPixel strip.
 * 
 *  Mike Lance (c) November, 2017
 *  MIT License
 * 
 * This project is based on several NeoPixel effects published on:
 *  https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 * 
 * It uses the standard ArduinoOTA library included with the ESP8266 libraries,
 *  with a few, more current updates from the github master:
 *  https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
 * 
 * It also uses a combination of the ArduinoOTA library and Tzapu's WiFiManager
 *  library (https://github.com/tzapu/WiFiManager) which are nicely (and 
 *  thankfully) combined by Erkobg:
 *  https://github.com/erkobg/WiFiManager-OTA.
 * 
 * Random library by Marvingroger: https://github.com/marvinroger/ESP8266TrueRandom
 * 
 *****************************************************************************/
/* All comments, references and change log moved to notes.h file. */
#include "notes.h"      // Non-functional, but gets saved to new version this way!

/*  Currently running on Wemos D1 Mini V2, Arduino 1.8.5   12/10/2017  */
/*   Wemos D1 Mini pins in use: D1, D2, D5; D4 via BUILTIN_LED.        */

  //  STATUS:  Fully functional, still in TEST MODE tho.
  //
  //  To do:
  //    - Maybe add `if (interrupt2State) break;` in some functions.
  //    - WiFiManager timeout - not sure I need or want this? TBI...
  //    - Do someting with case 5: fade R+W+B.  It's booring.
  //    - Make more patterns.


// WiFiManager-OTA section
#include <ESP8266WiFi.h>
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

//#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>    // https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
                           // Modified ArduinoOTA and WStrings library to more current Dec 2017 versions.

#include <ESP8266TrueRandom.h>    // https://github.com/marvinroger/ESP8266TrueRandom


#define MYSSID    "ML-WD1M"
//#define MYPASS    "welcome1"    // Only for WiFiManager initialization.
#define HOSTNAME  "ML-WD1M-1"
//#define LOADPWD   "welcome2"    // Had issues, intermittent. Troubleshoot some day...

// Interrupt pins
#define INTR1PIN   D1  // Any digital pin >0 for ESP8266...except D3,D4,D8
#define INTR2PIN   D2  // 

// TEST MODE DIAGNOSTICS...
#define LEDPIN     BUILTIN_LED  // 
#define LEDLOGIC   "LOW"          // HIGH|LOW|OFF string

// NeoPixel
#include <Adafruit_NeoPixel.h>

#define NEOPIN      D5    // GPIO pin for NeoPixel; REMEMBER resistor
#define NUM_LEDS    29    // 30 LED Strip that was cut too short argh
#define BRIGHTNESS  50    // Max brightness setting
#define DURATION    30    // seconds between changes

// Instantiate the strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIN, NEO_GRB + NEO_KHZ800);

// Globals
unsigned long loopTime = DURATION * 1000;
unsigned long startTime = 0;
byte neoMode = 0;
byte e, f, g;
int i, j, k;

// Interrupt status
volatile bool interrupt2State = false;  // Break from loops


// ############################################################################# //
// ##### Setup
// ############################################################################# //

void setup() {
                              // I've taken liberty indenting for clarity (!python)
  Serial.begin(115200);
  Serial.println("***Setup Begin...");

  // TEST MODE:  For Diagnostics.
    pinMode(LEDPIN, OUTPUT);

  // NeoPixel section.  Moved before WiFi Manager to use LEDs to communicate stats.
    Serial.println("Starting NeoPixel...");
    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show(); // Initialize all pixels to 'off'

  //WiFiManager
    Serial.println("Starting WiFiManager...");

    // TEST MODE  --  may leave this in ???
    // Show 3 Red LEDs for WiFiManager mode
    for (i = 0; i < 3; i++) setPixel(i, 32, 0, 0);
    strip.show();

    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // Timeout after 3 minutes if no network found
    wifiManager.setConfigPortalTimeout(180);

    //reset saved settings
    //wifiManager.resetSettings();

    //set custom ip for portal.  Default is 192.168.4.1
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    //if you like you can create AP with password
    //wifiManager.autoConnect("APNAME", "password");
    wifiManager.autoConnect(MYSSID);
  #ifdef MYPASS
    wifiManager.autoConnect(MYSSID, MYPASS);
  #endif
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();
    
    //if you get here you have connected to the WiFi
    Serial.println("WiFiManager connected.");
  // End of WiFiManager

  // ArduinoOTA    
    // The following directly from libraries/ArduinoOTA/examples/BasicOTA
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");
    ArduinoOTA.setHostname(HOSTNAME);

    // No authentication by default
    // ArduinoOTA.setPassword("admin");   // default if used
  #ifdef LOADPWD
    ArduinoOTA.setPassword(LOADPWD);     // <- Had issues authentiating, intermittent.
  #endif
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    // ML - note that getCommand() requires a newer version of ArduinoOTA
    //  library than what gets downloaded in the ESP2866 board definitions!
    //  Pulled down ArduinoODA, also dependency Wstring.  Argh!  Besides that,
    //  the rest directly from libraries/ArduinoOTA/examples/BasicOTA.
    ArduinoOTA.onStart([]() {
      Serial.println("\nArduinoOTA Start");
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nArduinoOTA End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("Ready OTA8");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  // End of ArduinoOTA

  // TEST MODE
    // Change LEDs to 3 Blue
    for (i = 0; i < 3; i++) setPixel(i, 0, 0, 32);
    strip.show();

  // Let's get a bit more random  
    // Now using ESP8266TrueRandom library,
    randomSeed(analogRead(A0));        // leave this in just in case.

  // Initialize the pushbutton pins as an input:
    pinMode(INTR1PIN, INPUT_PULLUP);    // Intent: WiFi Manager reset
    pinMode(INTR2PIN, INPUT_PULLUP);    // Intent: loop break
    // Attach interrupt
    attachInterrupt(digitalPinToInterrupt(INTR1PIN), interrupt1_ISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(INTR2PIN), interrupt2_ISR, CHANGE);
    // Some ESP8266 docs state RISING/FALLING are supported.  CHANGE works for me.

  // TEST MODE:  Wait for OTA in case of runnaway loop!
    for (i=0; i<50; i++) { 
      ArduinoOTA.handle();
      blinkLed(1, 50);
      }
  
  // Done with startup!  Turn off Neo's.
  setAllOff();
  Serial.println("***Setup Complete.");

}

// ############################################################################# //
// ##### Loop
// ############################################################################# //

void loop() {
  // Listen to load...
  ArduinoOTA.handle();
  
  // TEST MODE: REMOVE THIS FOR PRODUCTION:
  //blinkLed(3, 50);

  // Pick your pattern:
  //neoMode = ESP8266TrueRandom.random(1,19);  // (1 to n-1)
  // TEST MODES
  neoMode++;  if (neoMode == 19) neoMode = 1;   // sequentially loop
  //neoMode = 18;    // individual tests.  Timing section at bottom of loop()

  interrupt2State = false;      // Reset "loop break" interrupt

  startTime = millis();  
  while(millis() - startTime < loopTime) {
  
    ArduinoOTA.handle();          // Listen to load...more often
    if (interrupt2State) break;   // break out to loop(), pick next pattern
                                  // may want to put this in some of the function sub-loops...

    switch (neoMode) {

      case 1:        //Cylon - Original
        //CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
        CylonBounce(128, 0, 0, 5, 100, 100);
        break;

      case 2:        //Bouncing Balls
        //BouncingBalls(byte red, byte green, byte blue, int BallCount, int SpeedDelay)
        e = randByte(); f = randByte(); g = randByte(); 
        if ((e + f + g) < 64) break;  // skip if all off (black dots!)
        //if ((e + f + g) > 600) break;
        BouncingBalls(e, f, g, 6, 100);
        break;

      case 3:        // Color Wipe
        // colorWipe(byte red, byte green, byte blue, int SpeedDelay)
        e = randByte(); f = randByte(); g = randByte(); 
        if ((e + f + g) < 64) break;  // skip if all off (black dots!)
        colorWipe(e, f, g, 50);
        break;

      case 4:        //Cylon - Multi-color
        //CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
        e = randFifths(); f = randFifths(); g = randFifths(); 
        if ( e == 0 && f == 0 && g == 0 ) break;  // skip if all off (black dots!)
        CylonBounce(e, f, g, 4, 60, 60);
        break;

   //// Rewrite this to somthing better or toss it.
      case 5:        //Fade In and Fade Out Red, White and Blue
        //FadeInOut(byte red, byte green, byte blue, int SpeedDelay)
        FadeInOut(196,  0 ,  0 , 8 ); // red
        FadeInOut(160, 160, 160, 8 ); // white 
        FadeInOut( 0 ,  0 , 196, 8 ); // blue
        // OTA Caution: Don't make speedDelay too big else OTA on Blue!
        break;

      case 6:        //Fade In and Fade Out   - randmo colors
        //FadeInOut(byte red, byte green, byte blue, int SpeedDelay)
        e = randByte(); f = randByte(); g = randByte(); 
        if ((e + f + g) < 32) break;  // skip if all off (black dots!)
        if ((e + f + g) > 512) break;  // too bright
        FadeInOut(e, f, g, 5);
        break;

      case 7:        //Fire
        //Fire(int Cooling, int Sparking, int SpeedDelay)
        Fire(55,120,75);
        break;

      case 8:        //Halloween Eyes
        //HalloweenEyes(byte red, byte green, byte blue, 
        //             int EyeWidth, int EyeSpace, 
        //             boolean Fade, int Steps, int FadeDelay, int EndPause)
        // Fixed:
        // HalloweenEyes(255, 0, 0, 1,4, true, 10, 80, 3000);
        // or Random:
        HalloweenEyes(255, 0, 0, 
                  1, 3, 
                  true, ESP8266TrueRandom.random(5,13), 
                  ESP8266TrueRandom.random(150,300), ESP8266TrueRandom.random(500,3000));
        break;

      case 9:        //New KITT.ino
        //NewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
        NewKITT(255, 0, 0, 3, 75, 75);
        break;

      case 10:        //Rainbow Cycle - High to Low
        //rainbowCycle1(int SpeedDelay)
        rainbowCycle1(10);
        break;

      case 11:        //Rainbow Cycle - Low to High
        //rainbowCycle2(int SpeedDelay)
        rainbowCycle2(10);
        break;

      case 12:        //Rainbow Twinkle
        //void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne)
        TwinkleRandom(ESP8266TrueRandom.random(6,18), 250, false);
        // ArduinoOTA.handle() inside loop.
        break;

      case 13:        //Running Lights      // Same as theatre chase???
        //RunningLights(byte red, byte green, byte blue, int WaveDelay) 
        e = randFifths(); f = randFifths(); g = randFifths(); 
        if ( e == 0 && f == 0 && g == 0 ) break;  // skip if all off (black dots!)
        RunningLights(e, f, g, 100);
        break;

      case 14:        //Snow Sparkle        // Leaves leds on, don't want to clear every time tho?
        //SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay)
        SnowSparkle(16, 16, 16, 30, ESP8266TrueRandom.random(100,2000));
        break;

      case 15:        //Sparkle
        //Sparkle(byte red, byte green, byte blue, int SpeedDelay)
        e = randByte(); f = randByte(); g = randByte();
        if ( e == 0 && f == 0 && g == 0 ) break;  // skip if all off (black dots!)
        Sparkle(e, f, g, 250);
        break;

      case 16:        //Theatre Chase Rainbow
        //theaterChaseRainbow(int SpeedDelay)
        theaterChaseRainbow(150);
        // ArduinoOTA.handle() inside loop.
        break;

      case 17:        //Theatre Chase
        //theaterChase(byte red, byte green, byte blue, byte loopCount, int SpeedDelay)
        e = randFifths(); f = randFifths(); g = randFifths();
        if ( e == 0 && f == 0 && g == 0 ) break;  // skip if all off (black dots!)
        if ( (e + f + g) > 500 ) break;
        theaterChase(e, f, g, 20, 100);
        break;

      case 18:        //Twinkle
        //Twinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne)
        e = randFifths(); f = randFifths(); g = randFifths();
        Twinkle(e, f, g, 10, 250, false);
        break;

    }
       
  }
  
  // TEST MODE: Checking timings
  //Serial.print(neoMode);  Serial.print(",");
  //Serial.print(millis()-startTime);  Serial.print(",");
  //Serial.println(millis());
}

// ############################################################################# //
// ##### NeoPixel Functions
// ############################################################################# //

//Bouncing Balls.ino     modified
void BouncingBalls(byte red, byte green, byte blue, int BallCount, int SpeedDelay) {
  float Gravity = -9.81;
  int StartHeight = 1;
  
  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];
  
  for (int i = 0 ; i < BallCount ; i++) {   
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = StartHeight;
    Position[i] = 0; 
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i)/pow(BallCount,2); 
  }
  
  //while (millis() - startTime < loopTime) {  // interfers with outer while loop
  j = 0;
  while (j < 50) {                             // instead just do it 50 times.
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + 
                  ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
      if ( Height[i] < 0 ) {                      
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();
  
        if ( ImpactVelocity[i] < 0.01 ) {
          ImpactVelocity[i] = ImpactVelocityStart;
        }
      }
      Position[i] = round( Height[i] * (NUM_LEDS - 1) / StartHeight);
    }  
    for (int i = 0 ; i < BallCount ; i++) {
      setPixel(Position[i],red,green,blue);
    }
    strip.show();
    setAllOff();
    delay(SpeedDelay);
    j++;
  }
}

// Color Wipe.ino
void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  for(uint16_t i=0; i<NUM_LEDS; i++) {
      setPixel(i, red, green, blue);
      strip.show();
      delay(SpeedDelay);
  }
}

// Cylon.ino
void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAllOff();
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    strip.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAllOff();
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    strip.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

//Fade In and Fade Out Red, White and Blue.ino
//Fade In and Fade Out Your own Color(s).ino
void FadeInOut(byte red, byte green, byte blue, int SpeedDelay){
  float r, g, b;
      
  for(int k = 0; k < 256; k=k+1) { 
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    strip.show();
    delay(SpeedDelay);
  }
     
  for(int k = 255; k >= 12; k=k-2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    setAll(r,g,b);
    strip.show();
    delay(SpeedDelay);
  }
}


// Fire.ino
void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    cooldown = ESP8266TrueRandom.random(0, ((Cooling * 10) / NUM_LEDS) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( ESP8266TrueRandom.random(255) < Sparking ) {
    int y = ESP8266TrueRandom.random(7);
    heat[y] = heat[y] + ESP8266TrueRandom.random(160,255);
    //heat[y] = ESP8266TrueRandom.random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  strip.show();
  delay(SpeedDelay);
}

void setPixelHeatColor(int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

// Halloween Eyes.ino
void HalloweenEyes(byte red, byte green, byte blue, 
                   int EyeWidth, int EyeSpace, 
                   boolean Fade, int Steps, int FadeDelay, int EndPause){
  int StartPoint  = ESP8266TrueRandom.random( 0, NUM_LEDS - (2*EyeWidth) - EyeSpace );
  int Start2ndEye = StartPoint + EyeWidth + EyeSpace;
  
  for(i = 0; i < EyeWidth; i++) {
    setPixel(StartPoint + i, red, green, blue);
    setPixel(Start2ndEye + i, red, green, blue);
  }
  strip.show();
  
  if(Fade==true) {
    float r, g, b;
  
    for(int j = Steps; j >= 0; j--) {
      r = j*(red/Steps);
      g = j*(green/Steps);
      b = j*(blue/Steps);
      
      for(i = 0; i < EyeWidth; i++) {
        setPixel(StartPoint + i, r, g, b);
        setPixel(Start2ndEye + i, r, g, b);
      }
      strip.show();
      delay(FadeDelay);
    }
  }
  setAllOff(); // Set all black  
  delay(EndPause);
}

// New KITT.ino
void NewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
}

void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i =((NUM_LEDS-EyeSize)/2); i>=0; i--) {
    setAllOff();
    
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    
    setPixel(NUM_LEDS-i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(NUM_LEDS-i-j, red, green, blue); 
    }
    setPixel(NUM_LEDS-i-EyeSize-1, red/10, green/10, blue/10);
    
    strip.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i<=((NUM_LEDS-EyeSize)/2); i++) {
    setAllOff();
    
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    
    setPixel(NUM_LEDS-i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(NUM_LEDS-i-j, red, green, blue); 
    }
    setPixel(NUM_LEDS-i-EyeSize-1, red/10, green/10, blue/10);
    
    strip.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAllOff();
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    strip.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

void RightToLeft(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) {
  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAllOff();
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue); 
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    strip.show();
    delay(SpeedDelay);
  }
  delay(ReturnDelay);
}

// Rainbow Cycle.ino     Modified
void rainbowCycle1(int SpeedDelay) {
  // Color change from high led to low direction
  byte *c;

  //for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
  for(j=0; j<256; j++) {   // 1 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++) {
      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    strip.show();
    delay(SpeedDelay);
  }
}

// Rainbow Cycle.ino     Revised to reverse direction
void rainbowCycle2(int SpeedDelay) {
  // Color change from low led to high direction
  byte *c;

  //for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
  for(j=255; j>=0; j--) {     // 1 cycle of all colors on wheel
    ArduinoOTA.handle();
    for(i=0; i< NUM_LEDS; i++) {
      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      //setPixel(i, *(c+2), *(c+1), *c);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    strip.show();
    delay(SpeedDelay);
  }
}

// Rainbow Twinkle.ino
void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAllOff();
  
  for (int i=0; i<Count; i++) {
     setPixel(ESP8266TrueRandom.random(NUM_LEDS),ESP8266TrueRandom.randomByte(),
               ESP8266TrueRandom.randomByte(),ESP8266TrueRandom.randomByte());
     strip.show();
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAllOff(); 
     }
   }
  delay(SpeedDelay);
} 

// Running Lights.ino
void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position=0;
  
  for(int i=0; i<NUM_LEDS*2; i++)
  {
      Position++; // = 0; //Position + Rate;
      for(int i=0; i<NUM_LEDS; i++) {
        // sine wave, 3 offset waves make a rainbow!
        //float level = sin(i+Position) * 127 + 128;
        //setPixel(i,level,0,0);
        //float level = sin(i+Position) * 127 + 128;
        setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
                   ((sin(i+Position) * 127 + 128)/255)*green,
                   ((sin(i+Position) * 127 + 128)/255)*blue);
      }    
      strip.show();
      delay(WaveDelay);
  }
}

// Snow Sparkle.ino
void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red,green,blue);
  int Pixel = ESP8266TrueRandom.random(NUM_LEDS);
  setPixel(Pixel,0xff,0xff,0xff);
  strip.show();
  delay(SparkleDelay);
  setPixel(Pixel,red,green,blue);
  strip.show();
  delay(SpeedDelay);
  // this is cool if you don't setAllOff() after each loop!
}

// Sparkle.ino
void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  int Pixel = ESP8266TrueRandom.random(NUM_LEDS);
  setPixel(Pixel,red,green,blue);
  strip.show();
  delay(SpeedDelay);
  setPixel(Pixel,0,0,0);
}

// Theatre Chase Rainbow.ino    // ML modified
void theaterChaseRainbow(int SpeedDelay) {
  byte *c;
  for (int j=0; j < 256; j++) {                // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < NUM_LEDS; i=i+3) {
          c = Wheel( (i+j) % 255);
          setPixel(i+q, *c, *(c+1), *(c+2));   // turn every third pixel on
        }
        strip.show();
        delay(SpeedDelay);
        for (int i=0; i < NUM_LEDS; i=i+3) {
          setPixel(i+q, 0,0,0);                 // turn every third pixel off
        }
    }
    j++;
    ArduinoOTA.handle();          // long loop, let's just check.
  }
}

// Theatre Chase.ino    // ML modified
void theaterChase(byte red, byte green, byte blue, byte loopCount, int SpeedDelay) {
  for (int j=0; j<loopCount; j++) {           // ML modified - pass in loopCount
    for (int q=0; q < 3; q++) {
      for (int i=NUM_LEDS; i >0 ; i=i-3) {    // ML Reverse (high to low)
        setPixel(i-q, red, green, blue);      //turn every third pixel on
      }
      strip.show();
      delay(SpeedDelay);
      for (int i=NUM_LEDS; i >0 ; i=i-3) {    // ML Reverse (high to low)
        setPixel(i-q, 0,0,0);                 //turn every third pixel off
      }
    }
  }
}

// Twinkle.ino
void Twinkle(byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
  setAllOff(); 
  for (int i=0; i<Count; i++) {
     setPixel(ESP8266TrueRandom.random(NUM_LEDS),red,green,blue);
     strip.show();
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAllOff(); 
     }
   }
  delay(SpeedDelay);
}


// ############################################################################# //
// ##### Common Neopixel across all functions
// ############################################################################# //

void setPixel(int Pixel, byte red, byte green, byte blue) {
  //for compatibility
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
  strip.show();
}

void setAllOff() {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    strip.setPixelColor(i, 0);
  }
}

byte * Wheel(byte WheelPos) {
  //used by Rainbow Cycle, Theatre Chase Rainbow
  static byte c[3];
  
  if(WheelPos < 85) {
   c[0]=WheelPos * 3;
   c[1]=255 - WheelPos * 3;
   c[2]=0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   c[0]=255 - WheelPos * 3;
   c[1]=0;
   c[2]=WheelPos * 3;
  } else {
   WheelPos -= 170;
   c[0]=0;
   c[1]=WheelPos * 3;
   c[2]=255 - WheelPos * 3;
  }

  return c;
}

// ############################################################################# //
// ##### Common across all functions
// ############################################################################# //

void blinkLed(int loopCount,int duration) {
  if (LEDLOGIC == "OFF" ) return;
  if (LEDLOGIC == "HIGH") {
    for (int m = 0; m < loopCount; m++) {
      digitalWrite(LEDPIN, HIGH); delay(duration);
      digitalWrite(LEDPIN, LOW); delay(duration);
    }
  }
  else {    // == "LOW"  or anything else!
    for (int m = 0; m < loopCount; m++) {
      digitalWrite(LEDPIN, LOW); delay(duration);
      digitalWrite(LEDPIN, HIGH); delay(duration);
    }
  }
}

bool randBool() {
  return ESP8266TrueRandom.randomBit();
}

byte randByte() {
  return ESP8266TrueRandom.randomByte();
}

byte randThirds() {
  k = ESP8266TrueRandom.random(0,1000);
  if ( k < 333 ) return 0;
  if ( k <= 666 ) return 128;
  if ( k > 666 ) return 255;
}

byte randFifths() {
  k = ESP8266TrueRandom.random(0,1000);
  if (k < 200) return 0;
  if (k < 400) return 64;
  if (k < 800) return 128;
  if (k < 800) return 196;
  if (k <= 1000) return 255;
}

// ############################################################################# //
// ##### Interrupts
// ############################################################################# //

// Interrupt 1 - Reset WiFi settings
void interrupt1_ISR() {
  interrupt2State = true;
  //Reset WiFi settings!!!  
  Serial.println("*** Interrupt encountered.  Resetting WiFiManager. ***");
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // reset settings
  wifiManager.resetSettings();
  Serial.println("*** WiFiManager reset.  Restarting ESP... ***");
  ESP.restart();
 }

// Interrupt 2 - Just tell 'em I've been interrupted
void interrupt2_ISR() {
  interrupt2State = true;
 }

// ################################ THE END ################################ //
