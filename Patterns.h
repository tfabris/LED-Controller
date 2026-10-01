// ----------------------------------------------------------------------------
//   LED color patterns for the LED strips on my master bedroom shelves.
//
//       By Tony Fabris, with help from FastLED and other libraries.
// ----------------------------------------------------------------------------
// 
// This is called from the accompanying parent program "LED_Controller.ino".
// See that file for more details.

// ---------------------------------------------------------------------------
// OPTIONAL: Include my CE3K mothership scanner effect (separate file) as one
// of the color patterns. This must be before the "Master_Bedroom_Patterns_h"
// lines below or else you'll get a compilation error.
//
// Obtain here: https://github.com/tfabris/Close-Encounters-Mothership-Scanner
// ---------------------------------------------------------------------------
#if __has_include("Close_Encounters_Mothership_Scanner.h") // If the file is present.
  #include "Close_Encounters_Mothership_Scanner.h"
  #define CE3K_INCLUDED
#endif

// Ensure that the code below doesn't compile twice in the IDE.
#ifndef Master_Bedroom_Patterns_h
#define Master_Bedroom_Patterns_h

// ---------------------------------------------------------------------------
// Global variable for pattern color palette index (for color cycling).
// This variable is slowly incremented during the main Loop() function.
// ---------------------------------------------------------------------------
uint8_t gHue = 0;


// ---------------------------------------------------------------------------
// Solid black pattern. This is used as the "standby" mode, as if the LEDs had
// been turned off. They're not really off, they're just all black and thus
// drawing very little energy.
// 
// This is not one of the user-selectable cycled patterns, rather, it's called
// when the user presses the standby/power button.
// ---------------------------------------------------------------------------
void solidBlack()        { fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,0)    ); }


// ---------------------------------------------------------------------------
// Solid white LEDs using only the white LEDs in the RGBW strand. But not at
// full brightness, slightly dim instead.
// ---------------------------------------------------------------------------
void solidWhiteOnlyDim() { fill_solid( leds, NUM_LEDS, CRGBW(0,0,0,70)   ); } 


// ---------------------------------------------------------------------------
// Warm White - With slight color adjustments to compensate for voltage
// variation across the shelves.
// ---------------------------------------------------------------------------
void solidWarmWhite()
{
  // Run through the array of shelf start positions.
  for (uint8_t s=0; s < 6; s++)
  {
    CRGBW color = CRGBW(0,0,0,0);
    switch (s)
    {
      // Compensate for voltage variation across the shelves.
      case 0: color = CRGBW(255,  90, 15, 220); break;
      case 1: color = CRGBW(255, 120, 55, 255); break;
      case 2: color = CRGBW(255, 150, 95, 255); break;
      case 3: color = CRGBW(255,  90, 15, 220); break;
      case 4: color = CRGBW(255, 120, 55, 255); break;
      case 5: color = CRGBW(255,  90, 15, 220); break;
    }

    // Assign colors to the array of LEDs based on the shelf positions.
    for (int i=shelfStartPoints[s]; i<shelfStartPoints[s+1]; i++)
    {
      leds[i] = color;
    };
  }
}


// ---------------------------------------------------------------------------
// Solid Purple and Blue (and Magenta). Each shelf is a different solid color.
// ---------------------------------------------------------------------------
void solidPurpleAndBlue() 
{
  // Run through the array of shelf start positions.
  for (uint8_t s=0; s < 6; s++)
  {
    CRGB color = CRGB(0,0,0);
    switch (s)
    {
      case 0: color = CRGB( 255, 0, 255); break;
      case 1: color = CRGB(   0, 0, 255); break;
      case 2: color = CRGB( 105, 0, 255); break;
      case 3: color = CRGB(   0, 0, 255); break;
      case 4: color = CRGB( 255, 0, 255); break;
      case 5: color = CRGB(   0, 0, 255); break;
    }

    // Assign colors to the array of LEDs based on the shelf positions.
    for (int i=shelfStartPoints[s]; i<shelfStartPoints[s+1]; i++)
    {
      leds[i] = color;
    };
  }
}


// ---------------------------------------------------------------------------
// Purple And Green cycling colors.
// ---------------------------------------------------------------------------
CRGBPalette16 PurpleAndGreen_p = 
    { 0x9600FF, 0x9600FF, 0x9600FF, 0x9600FF, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 
      0x9600FF, 0x9600FF, 0x9600FF, 0x9600FF, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00 };
void purpleAndGreen()
{
  fill_palette(leds, NUM_LEDS, gHue, 1, PurpleAndGreen_p, 255, LINEARBLEND);
}


// ---------------------------------------------------------------------------
// Seattle Kraken cycling colors - With static red eye spot.
// ---------------------------------------------------------------------------
CRGBPalette16 SeattleKraken_p = 
    { 0x021525, 0x021525, 0x021525, 0x021525, 0x000000,
      0x214D43, 0x214D43, 0x214D43, 0x214D43, 0x214D43, 0x000000,
      0x84B6B7, 0x84B6B7, 0x84B6B7, 0x84B6B7, 0x000000 };
void seattleKraken()
{
  // Kraken color cycles at a different speed than the global gHue cycle, to
  // reduce apparent "flickering".
  static uint8_t colorPosition = 0;
  EVERY_N_MILLISECONDS(40)
  {
    if (colorCyclingIsOn)
    {
      colorPosition++;
    }
  }
  fill_palette(leds, NUM_LEDS, colorPosition, 1, SeattleKraken_p, 255, LINEARBLEND);

  // Black outline for the red eye spot.
  for (int i=shelfStartPoints[2]+12; i<shelfStartPoints[2]+28; i++)
  {
    leds[i] = CRGB::Black;
  }

  // Red eye spot.
  for (int i=shelfStartPoints[2]+15; i<shelfStartPoints[2]+25; i++)
  {
    leds[i] = CRGB::Red;
  }
}


// ---------------------------------------------------------------------------
// Rainbow - FastLED's built-in rainbow generator. 
// ---------------------------------------------------------------------------
void rainbow() 
{
  fill_rainbow( leds, NUM_LEDS, gHue, 1);
}


// ---------------------------------------------------------------------------
// Rainbow Plus W - By Tony Fabris
// White antialiased bars move across a scrolling rainbow.
//
// This has been specifically made for RGBW strands. The white bars use the
// white pixels and the rainbow uses the RGB pixels. Requires my modified
// "FastLED_RGBW_2.h" to be included in the main Arduino ".ino" file.
// ---------------------------------------------------------------------------
void rainbowPlusW() 
{
  static uint8_t  animationSpeed = 10;              // Number of milliseconds between frames of animation. Smaller=faster animation.      
  static uint8_t  barBrightness = 150;              // How bright are the white bars which overlay the rainbow?
  static int      subPixelOffset = 0;               // Tracks the current subpixel position of the white bar within one pixel.
  static int      subpixelResolution = 16;          // How many subpixel steps (how many antialiased steps) per pixel. Larger=slower animation.
  static uint16_t whiteInterval = (NUM_LEDS / 5);   // How many white bars scroll across the strand.
  static uint16_t wideness = (whiteInterval / 3);   // How thick the bars are (larger numbers=smaller bars).
  static int16_t  whiteStart = 0;                   // Tracks the current position of the main white bar in the strand (whole pixels).
  static float oneSubPixelWeightUnit = ((float)1 / (float)subpixelResolution); // How much each subpixel increases in brighness per animation frame.

  // Start with the rainbow background, which scrolls the same as the Rainbow
  // pattern (using the global variable gHue). Ideally, the white bars should
  // scroll at a different speed than the rainbow.
  fill_rainbow( leds, NUM_LEDS, gHue, 1);
 
  // Draw the white bars atop the rainbow. Outer loop is the length of the
  // strand.
  //
  // Special note: At first, the loop starts at a negative number less than
  // zero (calculated with "whiteStart-wideness" which sometimes results in a
  // negative number) so that the first bar in the strand doesn't spring into
  // life at full width at the beginning of each cycle. It slides into
  // existence at the start of the strand.
  for ( int16_t i = whiteStart-wideness; i < NUM_LEDS; i += whiteInterval )
  {
    // Inner loop draws each bar, up to its bar length.
    for ( uint16_t n = 0; n < wideness; n++ )
    {
      // "i+n" is the position in the strand plus how many pixels of the bar we
      //  have drawn. Ensure we don't draw before the start or past the end of
      //  the strand.
      if ( i+n >= 0 && i+n < NUM_LEDS )
      {
        // Using some of the subpixel antialiasing code that I had originally
        // written for my CE3K Scanner animation. The white bars move only a
        // partial pixel during each animation frame. The antialiasing lets
        // them smoothly glide along the strand instead of blinking along it. 
        float blendWeight = subPixelOffset * oneSubPixelWeightUnit;
        int nextPixelDarkness = barBrightness;
        int thisPixelDarkness = barBrightness;

        // Only antialias the first and last pixels of the white bar.
        if ( n == 0 )            {thisPixelDarkness = barBrightness; nextPixelDarkness = 0;            }
        if ( n == (wideness-1) ) {thisPixelDarkness = 0;             nextPixelDarkness = barBrightness;}

        // Paint the pixels of the white bar. Most will just be white
        // except the first and last ones.
        int blendedDarkness = (nextPixelDarkness*blendWeight)+(thisPixelDarkness*(1-blendWeight));
        leds[i+n].white = blendedDarkness;
      }
    }
  }

  // Animate the white bars by incrementing the variables.
  EVERY_N_MILLISECONDS(animationSpeed)
  {
    if (colorCyclingIsOn)
    {
      // Increment the bar position by subpixels.
      subPixelOffset ++;
      if (subPixelOffset >= subpixelResolution)
      {
        // We have looped through all subpixels, increment whole pixels and restart subpixels.
        subPixelOffset = 0;
        whiteStart++;
        if (whiteStart >= whiteInterval)
        {
          // Since the white bars are evenly spaced, we reset them at that spacing interval.
          whiteStart = 0;
        }
      }
    }
  }

  // Finally, blur the entire strand heavily.
  blur1d( leds, NUM_LEDS, 255);
}


// ---------------------------------------------------------------------------
// Party Colors: Scroll one of the FastLED built-in palettes.
//
// Tip: Try using different built-in palettes:
// https://fastled.io/docs/d3/d4f/group___predefined_palettes.html
// ---------------------------------------------------------------------------
void PartyColors()
{
  // Use NOBLEND for crisp color bars, and LINEARBLEND for soft color transitions.
  fill_palette(leds, NUM_LEDS, gHue, 1, PartyColors_p, 255, NOBLEND); 
}


// ---------------------------------------------------------------------------
// Overlays By Tony Fabris: Multiple waves of color moving across each other.
// ---------------------------------------------------------------------------
void overlays()
{ 
  // Number of milliseconds between position updates. Larger is slower.
  const int overallSpeed = 35;

  // Amount to fade to black. Larger numbers fade to black faster.
  const int fadeAmount = 1;

  // The first time this pattern is started, fill the background with a palette
  // to get some basic color in there instead of just blank. It will fade, but
  // at least it's starting with something other than black.
  static bool firstTime = true;
  if (firstTime)
  {
    firstTime = false;
    fill_palette(leds, NUM_LEDS, gHue, 1, CloudColors_p, 255, NOBLEND);
  }

  // Fade all LEDs a little, each time through the loop. The value for every N
  // milliseconds is used to control the fade speed. Short strands of LEDs need
  // to be faded faster than long ones, to prevent the strand from just slowly
  // turning fully white over time.
  EVERY_N_MILLISECONDS (NUM_LEDS / 100)
  {
    if (colorCyclingIsOn)
    {
      fadeToBlackBy( leds, NUM_LEDS, fadeAmount);
    }
  }

  // Declare an array with all of the information about each of the color waves
  // that will be scrolling across the LED strip. Each row of the array is the
  // dataset for one cubicwave8's worth of color. Each column is a different
  // type of data about the wave.
  //
  // Define column names so that I can refer to them by name instead of number.
  const int dataColumns = 4;
  const int position =    0;
  const int direction =   1;
  const int width =       2;
  const int hue =         3;
  static int colorWaves[][dataColumns] =
  { 
       // Note: These are starting values, some will change during the loop.
       // Position,             Direction,  Width,    Hue 
    {      0-15,                    1,       5,       1,    },
    {      NUM_LEDS+7,             -2,       7,       64,   },
    {      NUM_LEDS/3,              2,       7,      128,   },
    {      NUM_LEDS-(NUM_LEDS/3),  -1,       5,      192,   },
  };

  // Calculate the number of rows in the array based on the size of an integer.
  int rows = sizeof(colorWaves)/sizeof(int)/dataColumns;

  // Animate the color waves avery N milliseconds.
  EVERY_N_MILLISECONDS (overallSpeed)
  {
    // Loop through each wave in the data table.
    uint16_t i = rows;
    if (rows <= 0) return;
    while (i--)
    {
      // Width of the wave, in pixels. Must be between 1 and 256. The wave is a
      // cubicwave8 (similar to sine wave) of brightness values so that the wave
      // is a "hump" of color that moves through the strand. Sine wave info here:
      // https://github.com/FastLED/FastLED/wiki/FastLED-Wave-Functions
      int waveWidth = colorWaves[i][width];
      int numWaveSteps = 256/waveWidth;

      // Reposition each wave by adding its direction to its position.
      if (colorCyclingIsOn)
      {
          colorWaves[i][position]+=colorWaves[i][direction];
      }

      // When a wave hits the end of the strip, wrap it around to the other end.
      // Note that the width of the wave is added or subtracted from the position
      // so that the entire wave "eases onto" the strand instead of starting at
      // its start position. Also assign that wave a new hue while you're at it.
      if (colorWaves[i][position] > NUM_LEDS+waveWidth) { colorWaves[i][position] = 0-waveWidth; colorWaves[i][hue] += gHue; }
      if (colorWaves[i][position] < 0-waveWidth) { colorWaves[i][position] = NUM_LEDS+waveWidth; colorWaves[i][hue] += gHue; }

      // Loop through each pixel of each wave. Each wave has a starting position
      // plus the width of the wave's pixels.
      for (int ww=0; ww <= waveWidth; ww++)
      {
        // Convert the current position ("each step") in the wave, into a number
        // that will produce the desired result out of the FastLED wave functions.
        int inputValue = numWaveSteps * ww;

        // Add the current wave's current pixel to the strand.
        // Ensure that the pixel we're modifying is within the strand first.
        if ( (colorWaves[i][position]+ww < NUM_LEDS) && (colorWaves[i][position]+ww > 0) ) 
        {
          if (colorCyclingIsOn)
          {
            leds[colorWaves[i][position]+ww] += CHSV(colorWaves[i][hue], 255, cubicwave8(inputValue));
          }
        }
      }    
    }
  }
}


// ---------------------------------------------------------------------------
//  "Pacifica"
//  Gentle, blue-green ocean waves.
//  December 2019, Mark Kriegsman and Mary Corey March.
//  For Dan.
// ---------------------------------------------------------------------------
// In this animation, there are four "layers" of waves of light.  
//
// Each layer moves independently, and each is scaled separately.
//
// All four wave layers are added together on top of each other, and then 
// another filter is applied that adds "whitecaps" of brightness where the 
// waves line up with each other more.  Finally, another pass is taken
// over the led array to 'deepen' (dim) the blues and greens.
//
// The speed and scale and motion each layer varies slowly within independent 
// hand-chosen ranges, which is why the code has a lot of low-speed 'beatsin8' functions
// with a lot of oddly specific numeric ranges.
//
// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
// ---------------------------------------------------------------------------
CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = ledsRGB[i].getAverageLight();
    if( l > threshold)
    {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      ledsRGB[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 

    // This line doesn't work in the RGBW hack, so do it as separate changes instead.
    //    leds[i] |= CRGB( 2, 5, 7);
    leds[i].red   |= 2;
    leds[i].green |= 5;
    leds[i].blue  |= 7;
  }
}

void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++)
  {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

void pacifica()
{
  EVERY_N_MILLISECONDS(20)
  {
    // Increment the four "color index start" counters, one for each wave layer.
    // Each is incremented at a different speed, and the speeds vary over time.
    static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
    static uint32_t sLastms = 0;
    uint32_t ms = GET_MILLIS();
    uint32_t deltams = ms - sLastms;
    sLastms = ms;
    uint16_t speedfactor1 = beatsin16(3, 179, 269);
    uint16_t speedfactor2 = beatsin16(4, 179, 269);
    uint32_t deltams1 = (deltams * speedfactor1) / 256;
    uint32_t deltams2 = (deltams * speedfactor2) / 256;
    uint32_t deltams21 = (deltams1 + deltams2) / 2;
    sCIStart1 += (deltams1 * beatsin88(1011,10,13));
    sCIStart2 -= (deltams21 * beatsin88(777,8,11));
    sCIStart3 -= (deltams1 * beatsin88(501,5,7));
    sCIStart4 -= (deltams2 * beatsin88(257,4,6));

    // Clear out the LED array to a dim background blue-green
    fill_solid( leds, NUM_LEDS, CRGBW( 2, 6, 10, 0));

    // Do not continue animating the colors if the user has deactivated the color cycling features.
    if (!colorCyclingIsOn)
    {
      return;
    }

    // Render each of four layers, with different scales and speeds, that vary over time
    pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
    pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
    pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
    pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

    // Add brighter 'whitecaps' where the waves lines up more
    pacifica_add_whitecaps();

    // Deepen the blues and greens a bit
    pacifica_deepen_colors();
  }
}

// ---------------------------------------------------------------------------
// Pattern selection list. This is used for allowing the user to press the
// pattern-select buttons on the controller module, to cycle through the
// different color patterns in this code. 
//
// This pattern selection code is from:
// https://forum.arduino.cc/t/use-of-typedef-for-calling-function/509576/4
//
// This code must be after the pattern functions, to prevent compile errors.
// ---------------------------------------------------------------------------
typedef void (*PointerToPattern)();
struct ledPattern { PointerToPattern functPtr; const char * const name; };
ledPattern gPatterns[] =
{ 
    // Function Pointer    // Friendly name string for terminal output
  { solidWarmWhite,        "Solid Warm White"       }, 
  { solidWhiteOnlyDim,     "Solid White Only, Dim"  },
  { solidPurpleAndBlue,    "Solid Purple and Blue"  },
  { rainbow,               "Rainbow"                },
  { rainbowPlusW,          "Rainbow Plus W"         },
  { PartyColors,           "Party Colors"           },
  { overlays,              "Overlays"               },
  { pacifica,              "Pacifica"               },
  { purpleAndGreen,        "Purple and Green"       },
  { seattleKraken,         "Seattle Kraken"         },
  #ifdef CE3K_INCLUDED
    { ce3kScanner,         "CE3K Scanner"           },
  #endif
};

#endif
