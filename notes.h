/* Notes for NeoPixel on ESP8266 + ArduinoOTA + WiFiManager
 *  Mike Lance - December, 2017
 */
/*****************************************************************************
 * For initial board startup (in a new WiFi environment):
 *
 *  1. Connect board to power.
 *  2. Connect to board's WiFi SSID  "ML-NMCU-1"
 *  3. In browser, enter local WiFi SSID & Password.
 *
/*****************************************************************************
 * Erkobg's WiFiManager-OTA notes.
 *  1. Checks for stored Wifi and password
 *  2. If found tries to connect
 *  3. If successful - handles code and OTA
 *  4. If nothing is found or connection fails - creates AP - where you can set
 *      using default address 192.168.4.1 connection parameters
 */
/*****************************************************************************
 * Change Log:
 * 11/26/17 ML  R1
 *  Combined working compliation of tweaking4all NeoPixel routines, erkobg's WiFiManager-OTA
 *   and some of my earlier work.
 *  Note: "pragma message..." are OK during verify/compile time.
 *  Failed simple verify, couldn't find getCommand() in ArduinoOTA class.
 *   Looked in the library, wasn't in the ArduinoOTA.h, but it was in https://github.com/esp8266/Arduino
 *   Upon closer inspection, looked like what got installed was labeled version 2.3.0 but was actually
 *    version 2.2.0.  Downloaded the entire library from github, pulled out ArduinoODA and a dependency
 *    WString, replaced them...peachy.
 *   
 * 11/30/17  ML 
 *  Load onto an Amica NodeMCU board, eventually to Wemos D1 Mini or just an ESP-01.
 *  Changed hostname, tried with password.  Don't use password, authenication issues with Arduino.
 *  WARNING: NodeMCU board pin labels D# don't map properly -- use GPIO #s.
 *  Power board on/off to get Arduino to re-recognize OTA port.
 *  Added lots of led blink & serial.print() for testing/diagnostics.
 *
 * 12/3/17	ML  R2
 *  Major code reorg & cleanup.
 *  Switch random fuctions to marvinroger/ESP8266TrueRandom library.
 *  
 * 12/9/17	ML
 *  Begin individual tests and timings.
 *   - Had to put ArduinoOTA.handle() in some loops.
 *   - Many of the samples from tweaking4all have been modified.  Some significantly!  ***
 *  Added Neopixel startup indications:
 *   - 3 Red:	WiFiManager Mode
 *   - 3 Blue:	Wating for ArduinoOTA 
 *  Added a 10 second ArduinoOTA.handle() loop in setup() to catch OTA for runaway functions.
 *  
 * 12/10/17	ML  R3
 *  Continue individual tests and timings.
 *  Add interrupt code.  Contrary to some, FALLING trigger didn't work; CHANGE did.  ???
 *   - Two interrupts: one for new pattern, the other reset WiFiManager.
 *  Transfer to Wemos D1 Mini. Looks like might fit on an ESP-01/1M....
 *  Another round of code cleanup & comments.
 *  
 */
/*****************************************************************************
 * References:
 *
 * Adafruit NeoPixel library:
 *  https://github.com/adafruit/Adafruit_NeoPixel
 *
 * Wemos D1 pin mappings:
 *  https://github.com/esp8266/Arduino/blob/master/variants/d1_mini/pins_arduino.h
 *  
 * NodeMCU pin mappings:
 *  https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
 */ 
