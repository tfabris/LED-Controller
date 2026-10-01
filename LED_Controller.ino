// ----------------------------------------------------------------------------
//   LED Lighting controller for the LED strips on my master bedroom shelves.
//
//       By Tony Fabris, with help from FastLED and other libraries.
// ----------------------------------------------------------------------------
//
// This code is for an Arduino Mega. It controls the LED strip lights mounted
// on the edge of the display shelves in my master bedroom. The shelves can do
// various solid colors and color-cycling animations which I have programmed
// into this code.
//
// I have deliberately chosen to make these LED light colors and patterns
// hardcoded, and controlled with physical buttons: No WiFi, remotes, or apps.
//
// This code depends upon the FastLED code library to control the RGB lighting
// strips. However, I am using RGBW strips (note the "W" at the end), meaning
// that they have four LEDs per pixel: red, green, blue, and white. FastLED
// doesn't yet support RGBW strips (at the time of this writing), so I have
// included an additional code library that adds partial support for RGBW
// strips.
//
// Hardware:
//
// - Six sections of "BTF" brand SK6812 RGBW strips (one section per shelf),
//   wired in series:
//   https://www.amazon.com/dp/B079ZW1265
//
// - Driven by a Mean Well LRS-200-5 200W 5V 40 Amp power supply, supplying
//   power injection points, wired at several places along the LED strands:
//   https://www.amazon.com/dp/B0131V99BA
//
// - Controlled by an Arduino Mega 2560, "small" version, with eight
//   pushbuttons mounted on a small perfboard:
//   https://www.amazon.com/dp/B07TGF9VMQ
// ----------------------------------------------------------------------------

// Libraries.
#include <EEPROM.h>          // https://docs.arduino.cc/learn/built-in-libraries/eeprom
#include <ArduinoUniqueID.h> // https://github.com/ricaun/ArduinoUniqueID
#include <FastLED.h>         // https://fastled.io/
#include <Button2.h>         // https://github.com/LennartHennigs/Button2
#include "FastLED_RGBW_2.h"  // My modified version of the original FastLED_RGBW.h found at
                             // https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/

// Optional: Profiling tool. Comment out the line below when not profiling. 
// Obtain the profiler from https://github.com/tfabris/ScopeProfiler
//    #include "ScopeProfiler.h"  

// Global definitions.
#define LED_TYPE                WS2812B
#define COLOR_ORDER             RGB
#define DATA_PIN                48
#define SERIAL_PORT_SPEED       115200
#define COLOR_CYCLE_TIME        65

// EEPROM memory address positions for saving the lighting controller's state.
// Allows the controller to remember what it was doing across power cycles.
#define BRIGHTNESS_SAVE_ADDR    0
#define MODESELECTION_SAVE_ADDR 1
#define LIGHTSON_SAVE_ADDR      2
#define SAVEDMODE1_SAVE_ADDR    3
#define SAVEDMODE2_SAVE_ADDR    4
#define SAVEDMODE3_SAVE_ADDR    5
#define SPOTSON_SAVE_ADDR       6
#define CYCLEON_SAVE_ADDR       7

// Length of time, in milliseconds, that you must hold down one of the custom
// mode-selection buttons, in order to save the current pattern to that button.
// This value is also used as the basis for the time to hold down the power 
// button to reset the unit, except that it will be double this time. And it 
// is also used as the basis for the time to hold down the pattern select
// button to toggle the spotlights on and off, except that it will be half
// this time.
#define LONGCLICK_MS 1500

// Length of time in milliseconds that the "hold down" retriggers on the
// brightness control buttons, in essence, how quickly each brightness level
// goes up and down as you hold down the button.
#define HOLDDOWN_MS 100

// Define an Array Size function which will be used later in the code, to index
// through the various color patterns.
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Define a Reset function, so that we can reset the board by holding down the
// power button.
void(* resetFunc) (void) = 0;

// Define user button press handlers, using "Button2" code library. More code
// for these handlers appears farther down in this file. Details of how the
// Button2 library works is here: https://github.com/LennartHennigs/Button2
//
Button2 buttonOnOff; // Press: On/off (Sleep).       Hold: Reset.
Button2 buttonPatDn; // Press: Switch color pattern. Hold: Spotlights.  Both: Color Cycling on/off.
Button2 buttonPatUp; // Press: Switch color pattern. Hold: Spotlights.  Both: Color Cycling on/off.
Button2 buttonSave1; // Press: Select favorite A.    Hold: Set favorite A.
Button2 buttonSave2; // Press: Select favorite B.    Hold: Set favorite B.
Button2 buttonSave3; // Press: Select favorite C.    Hold: Set favorite C.
Button2 buttonBrtDn; // Press or hold: Increase brightness.
Button2 buttonBrtUp; // Press or hold: Decrease brightness.

// These important values will be repopulated in the setup() routine once the
// hardware has been self-identified. These are normally constants in most
// FastLED example code, however in this case I need to self-detect which board
// I'm running on, in order to be able to automatically switch back and forth
// between a development board at my desk, and the main unit connected to the
// full shelf strips in the master bedroom.
int MAX_POWER_MILLIAMPS = 500;
int NUM_LEDS = 40;
int shelfStartPoints[7] = {0,0,0,0,0,0,0};

// Individual LED white spotlights to light up specific objects that I'm
// displaying on my shelves. When the spotlight feature is turned on, the LEDs
// in this list will always be steady white, rather than color cycling. Most of
// these spotlights are just sections of the LED strips turned white, to help
// illuminate some sections of the shelves more brightly. Other spotlights are
// single white pixels which have a special lens assembly mounted over them, to
// make these: https://github.com/tfabris/EnterpriseSpotlights
// Note: For reference, the shelf start points on my main shelves are:
//    shelfStartPoints[0] = 0;
//    shelfStartPoints[1] = 100;
//    shelfStartPoints[2] = 206;
//    shelfStartPoints[3] = 313;
//    shelfStartPoints[4] = 365;
//    shelfStartPoints[5] = 417;
int spotlights[43] = {5,                              // Test light
                      15,16,17,18,19,                 // Books
                      39,40,41,42,43,                 // Books
                      110,111,112,113,114,            // Yellow Submarine
                      195,196,197,198,199,            // Tom and Crow
                      260,261,262,263,264,265,266,    // Saber
                      290,291,292,293,294,295,296,    // Aperture Science
                      328,329,                        // TOS Enterprise (Rear)
                      349,350,                        // TOS Enterprise (Front)
                      373,                            // Refit Enterprise (Saucer Front)
                      386,                            // Refit Enterprise (Saucer Rear)
                      394,                            // Refit Enterprise (Pylon)
                      415                             // Refit Enterprise (Far Nacelle)
                     };

// Standard spotlight color for most of the shelves. Most of the time I want the
// spotlights to be a slightly-warm white glow.
CRGBW spotlightColor = CRGBW(255,240,180,255);

// Special spotlight color for the lighting of my Tomy 1:350 Refit Enterprise
// model being displayed. More details about these spotlights can be found
// here: https://github.com/tfabris/EnterpriseSpotlights
// I am trying to match my inner nacelle spotlight color to the model's existing
// raytheon lighting on the outer nacelles. It is yellower than the other lights.
CRGBW refitSpotlightColor = CRGBW(255,240,10,160);
int refitSpotRangeStart = 415;
int refitSpotRangeEnd = 416;

// Additional variables which govern spotlight button controls.
bool spotlightsAreOn = true;
uint8_t patternSelectButtonsPressedCounter = 0;
bool colorCyclingIsOn = true;

// Black pixels surrounding the spotlight groups, to make the spots pop out a
// bit more.
int blackPixels[26] = {4, 6,        // Test
                       14, 20,      // Books
                       38, 44,      // Books
                       109, 115,    // Yellow Submarine
                       194, 200,    // Tom and Crow
                       259, 267,    // Saber
                       289, 297,    // Aperture Science
                       327, 329,    // TOS (Rear)
                       348, 351,    // TOS (Front)
                       372, 374,    // Refit (Saucer Front)
                       385, 387,    // Refit (Saucer Rear)
                       393, 395,    // Refit (Secondary Hull)
                       414, 416     // Refit (Far Nacelle) 
                      };
CRGBW blackPixelColor = CRGBW(0,0,0,0);

// Special system for defining different LED strand lengths. Normally it would
// look like this:
//   CRGBW leds[NUM_LEDS];
//   CRGB *ledsRGB = (CRGB *) &leds[0];
// But we won't know NUM_LEDS until after setup() runs and identifies which
// hardware we're running on. So we have to dynamically define the arrays as
// described here: https://arduino.stackexchange.com/a/3778 - Later in the
// setup() routine we will set the array size once we know NUM_LEDS.
CRGBW* leds = 0;
CRGB *ledsRGB = 0;

// Special mode for locating the numbers of LEDs in a strip. Use this only for
// debugging purposes, comment this out most of the time. If you uncomment this
// line, the code will light up only a single LED at a time, and it will print
// the array position of that LED on the serial port. To use this, look at the
// serial port output and look at the LEDs, then press the mode select buttons
// to increment or decrement which LED is lit. This method was used to obtain
// the actual numbers in the shelf and spotlight lists elsewhere in the code.
//    #define LED_COUNTING_MODE

// Trick: Define brightness and mode selection as signed integers so that we can
// use math which allows them to be changed in differently-sized steps and be
// truncated at the ends. For example, the math allows the brightness to be
// scaled at 1, 5, or 10-unit increments, and the math might make it go below
// zero or above 255. But then after it exceeds the bounds, it can be be reined
// back in to 0-255 after the changes. Then, when saving to the EEPROM, it will
// be cast back into a uint8_t before saving.
int brightness;
int modeSelection;

// Other variables governing button controls.
uint8_t savedMode1;
uint8_t savedMode2;
uint8_t savedMode3;
bool lightsAreOn;
int ledCounter; // Used for LED_COUNTING_MODE.

// Character buffer for printing short padded numbers. Frequently re-used.
char emit[6];
char emit2[6];

// String buffer for Unique ID to identify which individual Arduino board
// I'm running upon. String size will be reserved in the Setup routine.
const int uniqueIDStringSize = 19;
static String uniqueIDString = "";

// The individual LED color patterns are defined in this separate header file.
#include "Patterns.h"


// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------
// This main setup function is called once, when the Arduino boots up.
// ---------------------------------------------------------------------------
void setup()
{
  // Setup the debugging serial output so that it can be viewed in the Arduino
  // Serial Monitor. Note: Serial printouts will print any constant strings
  // using the "F()" macro, and will not be doing any concatenation. This is to
  // save some of the limited memory on the Arduino. For more details, see:
  // https://support.arduino.cc/hc/en-us/articles/360013825179
  Serial.begin(SERIAL_PORT_SPEED);
 
  // --------------------------------------------------------------------------
  // READ FROM EEPROM
  // --------------------------------------------------------------------------
  // Read the initial EEPROM values from flash storage. These values are the
  // ones that will be saved across power cycles and resets.
  brightness    =     (int)EEPROM.read( BRIGHTNESS_SAVE_ADDR    );
  modeSelection =     (int)EEPROM.read( MODESELECTION_SAVE_ADDR );
  savedMode1    = (uint8_t)EEPROM.read( SAVEDMODE1_SAVE_ADDR    );
  savedMode2    = (uint8_t)EEPROM.read( SAVEDMODE2_SAVE_ADDR    );
  savedMode3    = (uint8_t)EEPROM.read( SAVEDMODE3_SAVE_ADDR    );
  lightsAreOn   =    (bool)EEPROM.read( LIGHTSON_SAVE_ADDR      );
  spotlightsAreOn =  (bool)EEPROM.read( SPOTSON_SAVE_ADDR       );
  colorCyclingIsOn = (bool)EEPROM.read( CYCLEON_SAVE_ADDR       );

  // Sanity check the EEPROM values and, if they are not correct, then set them
  // to something within the correct range. This allows for crash-free "first
  // installation" on a fresh machine which might have random EEPROM values.
  if (brightness    > 255                    || brightness    < 1) { brightness = 255;  Serial.println( F("NOTE: Had to set brightness = 255")); }
  if (modeSelection >= ARRAY_SIZE(gPatterns) || modeSelection < 0) { modeSelection = 0; Serial.println( F("NOTE: Had to set modeSelection = 0")); }
  if (savedMode1    >= ARRAY_SIZE(gPatterns) || savedMode1    < 0) { savedMode1 = 0;    Serial.println( F("NOTE: Had to set savedMode1 = 0")); }
  if (savedMode2    >= ARRAY_SIZE(gPatterns) || savedMode2    < 0) { savedMode2 = 0;    Serial.println( F("NOTE: Had to set savedMode2 = 0")); }
  if (savedMode3    >= ARRAY_SIZE(gPatterns) || savedMode3    < 0) { savedMode3 = 0;    Serial.println( F("NOTE: Had to set savedMode3 = 0")); }

  // --------------------------------------------------------------------------
  // IDENTIFY WHICH ARDUINO BOARD IS RUNNING, AND SET APPROPRIATE VARIABLES
  // --------------------------------------------------------------------------
  // Get the unique ID for the Arduino board. This trick Works on an Arduino
  // Mega, but is ineffective on a Nano (all Nano boards return the same ID
  // number). More details at: https://github.com/ricaun/ArduinoUniqueID
  // ALL Arduino Nano boards:         UniqueID: 58 FF DF FF DF FF FF FF FF uniqueIDString: 58FFDFFFDFFFFFFFFF
  // My Desk Arduino Mega board:      UniqueID: 6E 75 6E 6B 77 6F 00 10 12 uniqueIDString: 6E756E6B776F001012
  // My Master Bedroom Arduino Mega:  UniqueID: 6E 75 6E 6B 77 6F 00 08 03 uniqueIDString: 6E756E6B776F000803
  uniqueIDString.reserve(uniqueIDStringSize*2);

  // This is the first thing that will be printed to the serial port, so print
  // some blank lines to start. This allows me to see a separation between
  // multiple compile/upload runs in the Arduino Serial Monitor.
  for (uint8_t i =0; i < 4; i++) {Serial.println(F(""));}
  UniqueIDdump(Serial);

  // Convert the board's unique ID to a string, for easier comparisons.
  for (size_t i = 0; i < UniqueIDsize; i++)
  {
    if (UniqueID[i] < 0x10) { uniqueIDString += (F("0")); }
    char pHexStr[2];
    sprintf(pHexStr,"%x",UniqueID[i]);
    uniqueIDString += pHexStr;
  }
  uniqueIDString.toUpperCase();
  Serial.print(F("Board uniqueIDString: "));
  Serial.println(uniqueIDString);

  // Different Arduino boards, wired with different pinouts, were used during
  // development. I tried a Nano board, but it runs out of memory for a full
  // set of LEDs. So I am using two different Mega boards, one at my desk
  // (where I have a few short strands for development and experimentation),
  // and the final one on my Master Bedroom shelves.
  //
  // Initialize the button pinouts and other values, depending on which board
  // was identified. The button pinouts use Lennart Hennigs' "Button2" code
  // library, which can be found at https://github.com/LennartHennigs/Button2
  if (uniqueIDString == "6E756E6B776F001012")
  {
    Serial.println( F("Board Identified:     Arduino Mega Dev at Desk"));
    buttonOnOff.begin(43);      // On/Off     
    buttonPatDn.begin(41);      // Pattern selection-
    buttonPatUp.begin(40);      // Pattern selection+
    buttonSave1.begin(31);      // Saved Pattern 1   
    buttonSave2.begin(38);      // Saved Pattern 2   
    buttonSave3.begin(39);      // Saved Pattern 3 
    buttonBrtDn.begin(30);      // Brightness-  
    buttonBrtUp.begin(27);      // Brightness+
    MAX_POWER_MILLIAMPS = 500;  // Desk unit uses USB-only power. Limit this or you'll fry your board.
    NUM_LEDS            = 130;  // Desk LED strip count.
    // DATA_PIN         = 48;   // Couldn't change DATA_PIN dynamically in code because of template problems in FastLED.
    shelfStartPoints[0] = 0;    // There are no shelves at my desk, so I am using arbitrary shelf points for test/dev.
    shelfStartPoints[1] = 22;
    shelfStartPoints[2] = 43;
    shelfStartPoints[3] = 65;
    shelfStartPoints[4] = 80;
    shelfStartPoints[5] = 105;
    shelfStartPoints[6] = NUM_LEDS; // There is no 7th shelf, this is one past the end of the LED array, allows code "shelfStartPoints[s+1]" to work.
  }
  else if (uniqueIDString == "6E756E6B776F000803")
  {
    Serial.println( F("Board Identified:     Arduino Mega Small in Master Bedroom"));

    // Before using the button inputs, I have to set the unused pins to a
    // certain mode, to turn off their pullup resistors. This is due to the
    // board layout being tight on this version of the control box - some of
    // the unused pins had to share pads with the pins that are being used.
    pinMode(A2 , INPUT); // Unused pin
    pinMode(A4 , INPUT); // Unused pin
    pinMode(A6 , INPUT); // Unused pin
    pinMode(A10, INPUT); // Unused pin
    pinMode(A12, INPUT); // Unused pin
    pinMode(A14, INPUT); // Unused pin
    pinMode(34 , INPUT); // Unused pin
    pinMode(36 , INPUT); // Unused pin
    pinMode(38 , INPUT); // Unused pin
    pinMode(42 , INPUT); // Unused pin
    pinMode(44 , INPUT); // Unused pin
    pinMode(46 , INPUT); // Unused pin

    // This version of my controller is using a couple of the analog pins on
    // this Mega board. According to this post, analog pins A0 through A15 are
    // actually just pins 54 through 69 on the Arduino Mega board:
    //
    // https://forum.arduino.cc/t/analog-pin-numbers-dont-match-standard-pinout-mega2560/610772/2
    //
    // However, for some reason, setting those numeric values doesn't work with
    // the Button2 library, we must call the A0 and A8 aliases directly, for
    // Button2 to work.
    buttonOnOff.begin(A0);       // On/Off     
    buttonPatDn.begin(47 );      // Pattern selection-
    buttonPatUp.begin(49 );      // Pattern selection+
    buttonSave1.begin(A8 );      // Saved Pattern 1   
    buttonSave2.begin(32 );      // Saved Pattern 2   
    buttonSave3.begin(40 );      // Saved Pattern 3  
    buttonBrtDn.begin(51 );      // Brightness- 
    buttonBrtUp.begin(53 );      // Brightness+ 
    MAX_POWER_MILLIAMPS = 15000; // External power supply can handle full power.
    NUM_LEDS            = 469;   
    // DATA_PIN         = 48;    // Couldn't change DATA_PIN dynamically in code because of template problems in FastLED.
    shelfStartPoints[0] = 0;
    shelfStartPoints[1] = 100;
    shelfStartPoints[2] = 206;
    shelfStartPoints[3] = 313;
    shelfStartPoints[4] = 365;
    shelfStartPoints[5] = 417;
    shelfStartPoints[6] = NUM_LEDS; // There is no 7th shelf, this is one past the end of the LED array, allows code "shelfStartPoints[s+1]" to work.
  }
  else
  {
    // If the board isn't one of the ones defined above, warn the user with a
    // serial port message, and restart the Arduino.
    Serial.println( F("----------------------------"));
    Serial.println( F("-   Board Unidentified.    -"));
    Serial.println( F("-                          -"));
    Serial.println( F("-   Ensure this board      -"));
    Serial.println( F("-   is entered into the    -"));
    Serial.println( F("-   code with the correct  -"));
    Serial.println( F("-   pinouts.               -"));
    Serial.println( F("-                          -"));
    Serial.println( F("-   Board uniqueIDString:  -"));
    Serial.print( F("-   "));
    Serial.print(uniqueIDString);
    Serial.println( F("     -"));
    Serial.println( F("----------------------------"));
    delay(5000);
    resetFunc();
  }

  // Print all config values to the serial port and pad the numeric digits. I'm
  // printing the constant strings with "F()" to save memory, and not doing
  // concatenation, per memory-saving tips here:
  // https://support.arduino.cc/hc/en-us/articles/360013825179
  Serial.print( F("NUM_LEDS: ")); Serial.println (String (NUM_LEDS));
  Serial.print( F("MAX_POWER_MILLIAMPS: ")); Serial.println (String (MAX_POWER_MILLIAMPS));
  Serial.print( F("Read from EEPROM:      lightsAreOn ")); Serial.println (String(lightsAreOn));
  Serial.print( F("Read from EEPROM:  spotlightsAreOn ")); Serial.println (String(spotlightsAreOn));
  Serial.print( F("Read from EEPROM: colorCyclingIsOn ")); Serial.println (String(colorCyclingIsOn));
  sprintf(emit, "%03d", brightness);
  Serial.print( F("Read from EEPROM:       brightness ")); Serial.println (String(emit));
  sprintf(emit, "%02d", modeSelection);
  Serial.print( F("Read from EEPROM:    modeSelection ")); Serial.print (String(emit)); Serial.print (F(" - ")); Serial.println (String(gPatterns[modeSelection].name));
  sprintf(emit, "%02d", savedMode1);
  Serial.print( F("Read from EEPROM:       savedMode1 ")); Serial.print (String(emit)); Serial.print (F(" - ")); Serial.println (String(gPatterns[savedMode1].name));
  sprintf(emit, "%02d", savedMode2);
  Serial.print( F("Read from EEPROM:       savedMode2 ")); Serial.print (String(emit)); Serial.print (F(" - ")); Serial.println (String(gPatterns[savedMode2].name));
  sprintf(emit, "%02d", savedMode3);
  Serial.print( F("Read from EEPROM:       savedMode3 ")); Serial.print (String(emit)); Serial.print (F(" - ")); Serial.println (String(gPatterns[savedMode3].name));

  // --------------------------------------------------------------------------
  // BUTTON HANDLER SETUPS
  // --------------------------------------------------------------------------
  // Button function setups, using https://github.com/LennartHennigs/Button2
  // These are the initial button setups, done only once at startup. They point
  // to more detailed functions lower down in the code.

  // On/off button.
  // ----------------------
  // "setPressedHandler" means that the button triggers quickly as soon as the
  // button is pressed down. This handler has a snappy response, making the
  // results of the button press appear immediately, even before the user's
  // finger lifts off the button.
  buttonOnOff.setPressedHandler(PressedButtonOnOff);

  // Holding down the on/off button is a system reset.
  buttonOnOff.setLongClickDetectedHandler(LongClickButtonOnOff); buttonOnOff.setLongClickTime(LONGCLICK_MS * 2); buttonOnOff.setLongClickDetectedRetriggerable(false);

  // Pattern up/down buttons.
  // ------------------------
  // Tapping the pattern select buttons will increment or decrement the
  // pattern.
  // 
  // Two ways we can do this:
  //
  // - setTapHandler - This makes the response to short presses reasonably
  //   snappy, but, when you do a longclick and release (see below) it
  //   increments the pattern after you release, which we don't want.
  //
  // - setClickHandler - This makes the response to the short presses less
  //   snappy, but you don't get the inadvertent pattern change upon release.
  buttonPatDn.setClickHandler(ClickedButtonPatDn);
  buttonPatUp.setClickHandler(ClickedButtonPatUp);

  // Pressing BOTH pattern select buttons at the same time will toggle color
  // cycling on/off. To do this, we need to set pressed and released handlers
  // so that we can keep track of whether they have been pressed or released
  // at the same time (by using the patternSelectButtonsPressedCounter
  // variable).
  buttonPatDn.setPressedHandler(PressedButtonPatDn); 
  buttonPatUp.setPressedHandler(PressedButtonPatUp);
  buttonPatDn.setReleasedHandler(ReleasedButtonPatDn);
  buttonPatUp.setReleasedHandler(ReleasedButtonPatUp);

  // Holding down either pattern select button will toggle the spotlights on/off.
  buttonPatDn.setLongClickDetectedHandler(LongClickButtonPatDn); buttonPatDn.setLongClickTime(LONGCLICK_MS / 2); buttonPatDn.setLongClickDetectedRetriggerable(false);
  buttonPatUp.setLongClickDetectedHandler(LongClickButtonPatUp); buttonPatUp.setLongClickTime(LONGCLICK_MS / 2); buttonPatUp.setLongClickDetectedRetriggerable(false);

  // Saved Pattern buttons.
  // ----------------------
  // The custom "saved mode" buttons work like this: Tap and release the button
  // to select that saved mode. To save a new mode to that button: Use the mode
  // select buttons to cycle to the desired pattern, then hold down one of the
  // saved mode buttons to save that pattern to that button. In order for this
  // to work, the button's quick press handler must be a Tap Handler which
  // triggers only after the button was pressed and released. If I had used a
  // pressed handler like the other buttons, then the initial press would
  // register before the long click, and it would select the previously saved
  // pattern and not the current pattern, and the user would save the wrong
  // pattern. By doing it as a tap handler, it registers the long click first
  // before the tap, thus correctly saving the pattern. The only drawback is
  // that simply clicking these buttons to select the saved pattern is not
  // quite as snappy in its response.
  buttonSave1.setTapHandler(TappedButtonSave1); buttonSave1.setLongClickDetectedHandler(LongClickButtonSave1); buttonSave1.setLongClickTime(LONGCLICK_MS);
  buttonSave2.setTapHandler(TappedButtonSave2); buttonSave2.setLongClickDetectedHandler(LongClickButtonSave2); buttonSave2.setLongClickTime(LONGCLICK_MS);
  buttonSave3.setTapHandler(TappedButtonSave3); buttonSave3.setLongClickDetectedHandler(LongClickButtonSave3); buttonSave3.setLongClickTime(LONGCLICK_MS);

  // Brightness buttons.
  // -------------------
  // The brightness buttons have special handling so that they can be held down
  // to quickly increase or decrease the brightness overall, without having to
  // taptaptaptap on the button to change the brightness by large amounts.
  buttonBrtDn.setPressedHandler(PressedButtonBrtDn); buttonBrtDn.setLongClickDetectedHandler(HoldDownButtonBrtDn); buttonBrtDn.setLongClickTime(HOLDDOWN_MS); buttonBrtDn.setLongClickDetectedRetriggerable(true);
  buttonBrtUp.setPressedHandler(PressedButtonBrtUp); buttonBrtUp.setLongClickDetectedHandler(HoldDownButtonBrtUp); buttonBrtUp.setLongClickTime(HOLDDOWN_MS); buttonBrtUp.setLongClickDetectedRetriggerable(true);

  // --------------------------------------------------------------------------
  // FASTLED SETUP
  // --------------------------------------------------------------------------
  // Initialize FastLED, modified for use with RGBW light strips - Details at
  // https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
  // Additionally, define LED arrays dynamically based on the number of LEDs
  // connected to the hardware, which is detected by the hardware
  // self-detection routine above. The global array size can be set at runtime
  // using the trick described here: https://arduino.stackexchange.com/a/3778
  leds = new CRGBW [NUM_LEDS];
  ledsRGB = (CRGB *) &leds[0];
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setMaxPowerInVoltsAndMilliamps( 5, MAX_POWER_MILLIAMPS);
  FastLED.setBrightness(brightness);

  // Before starting the loop, set all LEDs to black, so that, if we are in a
  // debugging session where the loop code is changing each time we re-upload
  // or reset the serial port, then there aren't any weird "stuck" colors of
  // LEDs hanging around between runs. It also makes a convenient "black blink"
  // appear when the device is reset.
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show();

  // Print out the list of color pattern names on the debugging console.
  // The code for the color patterns is in the included "Patterns.h" file.
  Serial.println(F(""));
  Serial.println(F("Available patterns:"));
  for (int i=0; i < ARRAY_SIZE(gPatterns); i++)
  {
    sprintf(emit, "%02d", i);
    Serial.print(F("Pattern ")); Serial.print(String (emit)); Serial.print(F(": ")); Serial.println(String(gPatterns[i].name));
  }

  // Print the button functions and their serial command equivalents.
  Serial.println(F(""));
  Serial.println(F("Available buttons and serial commands:"));
  Serial.println(F(" Button:                  Function:             Serial cmd:"));
  Serial.println(F(" Sleep                    Standby on/off        o      "));
  Serial.println(F(" Sleep (hold)             Reset controller      r      "));
  Serial.println(F(" Color up or down         Select color pattern  n  p   "));
  Serial.println(F(" Color up or down (hold)  Spotlights on/off     s      "));
  Serial.println(F(" Color up+down (both)     Toggle color cycling  t      "));
  Serial.println(F(" Favorites (tap)          Select favorite       a  b  c"));
  Serial.println(F(" Favorites (hold)         Save favorite         A  B  C"));
  Serial.println(F(" Bright up or down        Brightness +/-        +  -   "));
 
  // Print the current pattern name.
  sprintf(emit, "%02d", modeSelection);
  Serial.println(F(""));
  Serial.print(F("Current Pattern = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[modeSelection].name));

  // Special mode to help locate the LED shelf start positions.
  #if defined(LED_COUNTING_MODE)
    Serial.println( F("---------------------------------------"));
    Serial.println( F("-       SPECIAL LED COUNTING MODE     -"));
    Serial.println( F("-                                     -"));
    Serial.println( F("-  Use the pattern selection buttons  -"));
    Serial.println( F("-  to cycle through the LEDs.         -"));
    Serial.println( F("---------------------------------------"));
  #endif

  Serial.println(F(""));
  Serial.println(F("Startup complete."));
}


// ---------------------------------------------------------------------------
// Function: Run the button state detection algorithm for all enabled buttons.
// --------------------------------------------------------------------------- 
void ScanAllButtons()
{
  buttonOnOff.loop();
  buttonPatDn.loop();
  buttonPatUp.loop();
  buttonSave1.loop();
  buttonSave2.loop();
  buttonSave3.loop();
  buttonBrtDn.loop();
  buttonBrtUp.loop();
}


// ---------------------------------------------------------------------------
// Function: Interpret commands on the Arduino Serial Monitor port, for lazy
// debugging sessions where my control box's physical buttons are far away.
// ---------------------------------------------------------------------------
void ScanSerialInput()
{
  char userChar = 0;
  if (Serial.available())
  {
    // Read one character (byte) from the Arduino debug serial port.
    userChar = Serial.read();

    // These single-character commands do the same thing as pressing buttons.
    switch (userChar)
    {
      case 'o': PressedButtonOnOff(buttonOnOff); break;
      case '-': PressedButtonBrtDn(buttonBrtDn); break;
      case '+': PressedButtonBrtUp(buttonBrtUp); break;
      case 'r': resetFunc(); break;
      case 's': LongClickButtonPatDn(buttonPatDn); break; // Toggle Spotlights on/off
      case 't': ToggleColorCycling(); break;
      case 'p': ClickedButtonPatDn(buttonPatDn); break;
      case 'n': ClickedButtonPatUp(buttonPatUp); break;
      case 'a': TappedButtonSave1(buttonSave1); break;
      case 'b': TappedButtonSave2(buttonSave2); break;
      case 'c': TappedButtonSave3(buttonSave3); break;
      case 'A': LongClickButtonSave1(buttonSave1); break;       
      case 'B': LongClickButtonSave2(buttonSave2); break;
      case 'C': LongClickButtonSave3(buttonSave3); break;
      default: break;
    }
  }
}


// ---------------------------------------------------------------------------
// Main program loop which repeats infinitely while the Arduino is turned on.
// ---------------------------------------------------------------------------
void loop()
{
  // Scan each button which is being used with the "Button2" code library.
  ScanAllButtons();

  // Scan for serial input from a USB debugging session.
  ScanSerialInput();

  // Decide what to do, based on whether the user wants the lights on or off.
  if (lightsAreOn)
  {
    // Special LED counting mode for debugging.
    #if defined(LED_COUNTING_MODE)
      fill_solid(leds, NUM_LEDS, CRGBW(0,0,0,0));
      leds[ledCounter] = CRGBW(0,0,0,255);
    #else     
      // If lights are on, call the function of the selected color pattern, each
      // time we pass through the main loop. Calling one of these functions
      // will update the array colors, and for any functions which do color
      // cycling, they will use the global gHue variable as the palette index.
      // These pattern functions are defined in the included file "Patterns.h".
      gPatterns[modeSelection].functPtr();  

      // If white spotlights are on, add those on top of the existing lights.
      if (spotlightsAreOn)
      {
        for (int i=0; i < ARRAY_SIZE(spotlights); i++)
        {
          if (spotlights[i] < NUM_LEDS)
          {
            if ( (spotlights[i] >= refitSpotRangeStart) && (spotlights[i] <= refitSpotRangeEnd) )
            {
              // Special color for specific spotlights on one particular shelf.
              leds[spotlights[i]] = refitSpotlightColor; 
            }
            else
            {
              // Normal spotlight color.
              leds[spotlights[i]] = spotlightColor;
            }
          }
        }

        // Also add black pixels around the white spotlight areas, to make them pop out.
        for (int i=0; i < ARRAY_SIZE(blackPixels); i++)
        {
          if (blackPixels[i] < NUM_LEDS)
          leds[blackPixels[i]] = blackPixelColor; 
        }
      }
    #endif  
  }
  else
  {
    // If lights are off, set all LEDs to dark.
    solidBlack();
  }

  // Each time through the main loop, set the brightness and then update the
  // actual LED strip with the colors chosen by the pattern functions which
  // were called above.
  FastLED.setBrightness((uint8_t)brightness);
  FastLED.show();

  // Write the data of our saved values to EEPROM, but only if the new value
  // differs from old values. Using "update" instead of "write" will save the
  // flash memory from being written too many times and going bad.
  EEPROM.update(BRIGHTNESS_SAVE_ADDR,    (uint8_t)brightness);
  EEPROM.update(MODESELECTION_SAVE_ADDR, (uint8_t)modeSelection);
  EEPROM.update(SAVEDMODE1_SAVE_ADDR,    (uint8_t)savedMode1);
  EEPROM.update(SAVEDMODE2_SAVE_ADDR,    (uint8_t)savedMode2);
  EEPROM.update(SAVEDMODE3_SAVE_ADDR,    (uint8_t)savedMode3);
  EEPROM.update(LIGHTSON_SAVE_ADDR,      (uint8_t)lightsAreOn);
  EEPROM.update(SPOTSON_SAVE_ADDR,       (uint8_t)spotlightsAreOn);
  EEPROM.update(CYCLEON_SAVE_ADDR,       (uint8_t)colorCyclingIsOn);

  // Slowly cycle through the color palette, for any patterns which use the
  // variable gHue to cycle their colors.
  if (colorCyclingIsOn)
  {
    EVERY_N_MILLISECONDS( COLOR_CYCLE_TIME ) { gHue++; }
  }

  // Optional profiling printout, from https://github.com/tfabris/ScopeProfiler
  #ifdef SCOPE_PROFILER_H
    DisplayProfileAverages();
  #endif
}


// ---------------------------------------------------------------------------
// Functions: Process the user button presses.
//
// The button processing functions print some output to the serial port. When
// printing, these functions save memory by using the F() macro for constant
// strings, and they don't do string concatenation, per the tips here:
// https://support.arduino.cc/hc/en-us/articles/360013825179
// ---------------------------------------------------------------------------

// -----------------------
// On/Off (Sleep) button.
// -----------------------
void PressedButtonOnOff(Button2& btn)
{
  lightsAreOn = !lightsAreOn;
  Serial.print(F("On/Off Button pressed. lightsAreOn = ")); Serial.println(String(lightsAreOn));
}

// Hold down the On/Off button to reset the Arduino. This isn't the same as a
// true reset button, but it can get me out of a situation where the unit is
// working but its USB port won't show up on my PC.
void LongClickButtonOnOff(Button2& btn)
{
  Serial.println(F("On/Off Button held down. Resetting unit."));
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0)        ); FastLED.show();
  fill_solid( leds, NUM_LEDS, CRGBW(100,0,0,0)      ); FastLED.show();    
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0)        ); FastLED.show();
  fill_solid( leds, NUM_LEDS, CRGBW(0,100,0,0)      ); FastLED.show();    
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0)        ); FastLED.show();
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,100,0)      ); FastLED.show();    
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0)        ); FastLED.show();
  fill_solid( leds, NUM_LEDS, CRGBW(100,100,100,100)); FastLED.show();    
  resetFunc();
}

// -----------------------
// Pattern select buttons.
// -----------------------
void nextPattern()
{
  modeSelection++;
  if (modeSelection >= ARRAY_SIZE(gPatterns))
  {
    modeSelection = 0; // Wrap to the beginning.
  }
}
void prevPattern()
{
  modeSelection--;
  if (modeSelection < 0)
  {
    modeSelection = ARRAY_SIZE(gPatterns)-1; // Wrap to the end.
  }
}
void ClickedButtonPatDn(Button2& btn)
{
  #if defined(LED_COUNTING_MODE)
    ledCounter--;
    if (ledCounter < 0) { ledCounter = (NUM_LEDS - 1); }
    Serial.print( F("COUNTING: LED Number: [")); Serial.print( String(ledCounter)); Serial.print(F("] - ")); Serial.print(String(ledCounter+1)); Serial.print(F("/")); Serial.println(String(NUM_LEDS)); 
  #else
    Serial.print(F("Pattern select Down pressed. "));
    lightsAreOn = true;
    prevPattern();
    //  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Removed: Black blink for user feedback.
    sprintf(emit, "%02d", modeSelection);
    Serial.print(F("modeSelection = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[modeSelection].name));
  #endif
}
void ClickedButtonPatUp(Button2& btn)
{
  #if defined(LED_COUNTING_MODE)
    ledCounter++;
    if (ledCounter > (NUM_LEDS - 1)) { ledCounter = 0; }
    Serial.print( F("COUNTING: LED Number: [")); Serial.print( String(ledCounter)); Serial.print(F("] - "));; Serial.print(String(ledCounter+1));; Serial.print(F("/")); Serial.println(String(NUM_LEDS));
  #else
    Serial.print(F("Pattern select Up pressed.   "));
    lightsAreOn = true;
    nextPattern();
    //  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Removed: Black blink for user feedback.
    sprintf(emit, "%02d", modeSelection);
    Serial.print(F("modeSelection = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[modeSelection].name));
  #endif
}

// Special handlers for detecting when both of the pattern select buttons are
// pressed at the same time. This will toggle color cycling on and off. Keep
// track of how many of those two buttons are pressed at any given time by
// using the global patternSelectButtonsPressedCounter variable. If the counter
// is equal to 2 then two of those two buttons are pressed.
void ToggleColorCycling()
{
  colorCyclingIsOn = !colorCyclingIsOn;
  Serial.print(F("colorCyclingIsOn = ")); Serial.println(String(colorCyclingIsOn));
}
void PressedButtonPatDn(Button2& btn)
{
  patternSelectButtonsPressedCounter++;
  if (patternSelectButtonsPressedCounter == 2)
  {
    ToggleColorCycling();
  }
}
void PressedButtonPatUp(Button2& btn)
{
  patternSelectButtonsPressedCounter++;
  if (patternSelectButtonsPressedCounter == 2)
  {
    ToggleColorCycling();
  }
}
void ReleasedButtonPatDn(Button2& btn)
{
  patternSelectButtonsPressedCounter--;
  patternSelectButtonsPressedCounter &= -!(patternSelectButtonsPressedCounter == 255); // BUGFIX: Prevent it from wrapping to 255
}
void ReleasedButtonPatUp(Button2& btn)
{
  patternSelectButtonsPressedCounter--;
  patternSelectButtonsPressedCounter &= -!(patternSelectButtonsPressedCounter == 255); // BUGFIX: Prevent it from wrapping to 255
}

// Hold down pattern cycle buttons to toggle spotlights on/off.
void LongClickButtonPatDn(Button2& btn)
{
  spotlightsAreOn = !spotlightsAreOn;
  Serial.print(F("Pattern select down held down. spotlightsAreOn = ")); Serial.println(String(spotlightsAreOn));
}
void LongClickButtonPatUp(Button2& btn)
{
  spotlightsAreOn = !spotlightsAreOn;
  Serial.print(F("Pattern select up held down. spotlightsAreOn = ")); Serial.println(String(spotlightsAreOn));
}

// -----------------------
// Saved mode buttons.
// -----------------------
void TappedButtonSave1(Button2& btn)
{
  lightsAreOn = true;
  modeSelection = savedMode1;
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Black blink for user feedback.
  sprintf(emit, "%02d", modeSelection);
  Serial.print(F("Saved Pattern 1 pressed. modeSelection = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[modeSelection].name));
}
void LongClickButtonSave1(Button2& btn)
{
  lightsAreOn = true;
  savedMode1 = modeSelection;
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Black blink for user feedback.
  sprintf(emit, "%02d", savedMode1);
  Serial.print(F("Saved Pattern 1 held down. savedMode1 = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[savedMode1].name));
}
void TappedButtonSave2(Button2& btn)
{
  lightsAreOn = true;
  modeSelection = savedMode2;
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Black blink for user feedback.
  sprintf(emit, "%02d", modeSelection);
  Serial.print(F("Saved Pattern 2 pressed. modeSelection = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[modeSelection].name));
}
void LongClickButtonSave2(Button2& btn)
{
  lightsAreOn = true;
  savedMode2 = modeSelection;
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Black blink for user feedback.
  sprintf(emit, "%02d", savedMode2);
  Serial.print(F("Saved Pattern 2 held down. savedMode2 = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[savedMode2].name));
}
void TappedButtonSave3(Button2& btn)
{
  lightsAreOn = true;
  modeSelection = savedMode3;
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Black blink for user feedback.
  sprintf(emit, "%02d", modeSelection);
  Serial.print(F("Saved Pattern 3 pressed. modeSelection = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[modeSelection].name));
}
void LongClickButtonSave3(Button2& btn)
{
  lightsAreOn = true;
  savedMode3 = modeSelection;
  fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0) ); FastLED.show(); // Black blink for user feedback.
  sprintf(emit, "%02d", savedMode3);
  Serial.print(F("Saved Pattern 3 held down. savedMode3 = ")); Serial.print(String(emit)); Serial.print(F(" - ")); Serial.println(String(gPatterns[savedMode3].name));
}

// -----------------------
// Brightness buttons.
// -----------------------
// General handlers for changing brightness (called by the functions below.)
void IncreaseBrightness()
{
  lightsAreOn = true;
  int brightnessScaling = 5;
  if (brightness < 10) {brightnessScaling = 1;}
  if (brightness > 100) {brightnessScaling = 10;}
  brightness += brightnessScaling;
  if ( brightness > 255 ) {brightness = 240; FastLED.setBrightness((uint8_t)brightness); FastLED.show(); brightness = 255; FastLED.setBrightness((uint8_t)brightness); FastLED.show();}
  sprintf(emit,  "%03d", brightness);
  sprintf(emit2, "%03d", brightnessScaling);
  Serial.print(F("Brightness = ")); Serial.print(String(emit)); Serial.print(F(" Scaling = ")); Serial.println(String(emit2));
}
void DecreaseBrightness()
{
  lightsAreOn = true;
  int brightnessScaling = 5;
  if (brightness < 11) {brightnessScaling = 1;}
  if (brightness > 110) {brightnessScaling = 10;}
  brightness -= brightnessScaling;
  if ( brightness < 2 ) {brightness = 3; FastLED.setBrightness((uint8_t)brightness); FastLED.show(); brightness=2; FastLED.setBrightness((uint8_t)brightness); FastLED.show();}
  sprintf(emit,  "%03d", brightness);
  sprintf(emit2, "%03d", brightnessScaling);
  Serial.print(F("Brightness = ")); Serial.print(String(emit)); Serial.print(F(" Scaling = ")); Serial.println(String(emit2));
}    
void PressedButtonBrtDn(Button2& btn)
{
  Serial.print(F("Brightness - pressed.   "));
  DecreaseBrightness();
}
void HoldDownButtonBrtDn(Button2& btn)
{
  Serial.print(F("Brightness - held down. "));
  DecreaseBrightness();
}  
void PressedButtonBrtUp(Button2& btn)
{
  Serial.print(F("Brightness + pressed.   "));
  IncreaseBrightness();
}
void HoldDownButtonBrtUp(Button2& btn)
{
  Serial.print(F("Brightness + held down. "));
  IncreaseBrightness();
}
