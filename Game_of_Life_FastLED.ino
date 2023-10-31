//Uses the FastLED library to drive a 16x16 array of 8x8 panels of WS2812 RGB LEDs. 

#include <FastLED.h>
#include "ESP8266TrueRandom.h"
#include "palletes.h"
#define WORLD_WIDTH   16
#define WORLD_HEIGHT  16

int16_t world[WORLD_HEIGHT][WORLD_WIDTH];
int16_t newWorld[WORLD_HEIGHT][WORLD_WIDTH];

CRGBPalette256 currentPalette;
TBlendType    currentBlending;

bool reset = false;
#define GEN_DELAY 250
#define GEN_LIMIT 5000
#define AGE_LIMIT 255
#define IDENTICIAL_LIMIT 50
#define CELL_AGING true

uint16_t generation;
uint16_t currentLiving;
uint16_t nextLiving;
uint16_t sameLivingCount;
bool reversePalette = false;

CRGB aliveColor;
CRGB deadColor= 0x000000;  // Color for dead cells (black).
uint8_t palleteOffset; 

bool resetW = false;

#define LED_PIN 2      
#define COLOR_ORDER GRB     
#define CHIPSET WS2812
#define GAME_BRIGHTNESS 64 //Max Brightness
#define NUM_LEDS (WORLD_WIDTH * WORLD_HEIGHT)
CRGB leds[NUM_LEDS];


void setup() {
  Serial.begin(115200);
  randomSeed(ESP8266TrueRandom.random());
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>( leds, NUM_LEDS ).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness( GAME_BRIGHTNESS );
  currentBlending = LINEARBLEND;
  resetWorld();
}

void loop() {
  while(!resetW){
    drawWorld();
    nextGen();
    countLiving();
    checkReset();
    copyWorld();
    delay(GEN_DELAY);
  }
  resetWorld();
}

void resetWorld(){
  Serial.println("Reset");
  currentPalette = listofpalettes[random(18)];
  resetW = false;
  sameLivingCount = 0;
  generation = 0;
  for (uint8_t  y = 0; y < WORLD_HEIGHT; y++){
    for (uint8_t  x = 0; x < WORLD_WIDTH; x++){
      world[y][x] = random(2);
    }
  }
}

void countLiving(){
  currentLiving = 0;
  nextLiving = 0;
  for (uint8_t y = 0; y < WORLD_HEIGHT; y++){
    for (uint8_t x = 0; x < WORLD_WIDTH; x++){
        if (world[y][x] > 0){
          currentLiving++;
        }
        if (newWorld[y][x] > 0){
          nextLiving++;
        }
    }
  }
}

void checkReset(){
  Serial.println("Check Reset");
  if(currentLiving == nextLiving){
    sameLivingCount++;
  }
  else {
    sameLivingCount = 0;
  }
  if(sameLivingCount > IDENTICIAL_LIMIT){
    Serial.println("Limit Reached");
    delay(200);
    resetW = true;
   }
  if(currentLiving == 0){
    Serial.println("No Living");
    delay(200);
    resetW = true;
  }
  if(generation > GEN_LIMIT){
    resetW = true;
  }
}

void nextGen(){
  generation++;
  for (uint8_t y = 0; y < WORLD_HEIGHT; y++){
    for (uint8_t x = 0; x < WORLD_WIDTH; x++){
      uint8_t neighbors = getNumberOfNeighbors(x, y);
      if ( world[y][x] > 0 && (neighbors == 2 || neighbors == 3 )){
        newWorld[y][x] = world[y][x]+1;
      } 
      else if (world[y][x] > 0){
        newWorld[y][x] = 0;
      }
      if (world[y][x] == 0 && neighbors == 3){
        newWorld[y][x] = 1;
      }
      else if (world[y][x] == 0){
        newWorld[y][x] = 0;
      }
//      if (world[y][x] > AGE_LIMIT){
//        newWorld[y][x] = -1;
//      }
//      if (world[y][x] == -1){
//        newWorld[y][x] = 0;
//      }      
    }
  }
}


void copyWorld(){
  for (uint8_t  y = 0; y < WORLD_HEIGHT; y++){
    for (uint8_t  x = 0; x < WORLD_WIDTH; x++){
      world[y][x] = newWorld[y][x];
    }
  }
}

int getNumberOfNeighbors(uint8_t x, uint8_t y) {
  
  uint8_t count = 0;
  boolean wrapX = true;
  boolean wrapY = true;

  uint8_t x_l = x - 1;
  uint8_t x_r = x + 1;
  uint8_t y_t = y - 1;
  uint8_t y_b = y + 1;


  if ( wrapX ) {
    if      ( x == 0 )  x_l = WORLD_WIDTH - 1;
    else if ( x >= WORLD_WIDTH - 1 )  x_r = 0;
  }
  if ( wrapY ) {
    if      ( y == 0 ) y_t = WORLD_HEIGHT - 1;
    else if ( y >= WORLD_HEIGHT - 1 )  y_b = 0;
  }

  count += world[ y_t ][ x_l ] > 0; // above left
  count += world[ y_t ][  x  ] > 0; // above
  count += world[ y_t ][ x_r ] > 0; // above right

  count += world[  y  ][ x_l ] > 0; // left
  count += world[  y  ][ x_r ] > 0; // right

  count += world[ y_b ][ x_l ] > 0; // bottom left
  count += world[ y_b ][  x  ] > 0; // bottom
  count += world[ y_b ][ x_r ] > 0; // bottom right
  return count;
}

void drawWorld(){
  drawLEDWorld();
}

void drawLEDWorld() {
  FastLED.clear();
  uint8_t index = 0; 
  for ( int y = 0; y < WORLD_HEIGHT; y++ ) {
    for ( int x = 0; x < WORLD_WIDTH; x++ ) {
      #if CELL_AGING // Change color based on age.    
        if ( world[ x ][ y ] > 0 ) {
            if (reversePalette) {
              index = max(255-generation-(world[x][y]/2), 0); //+ palleteOffset;
              leds[XYsafe(x,y)]= ColorFromPalette(currentPalette, index); 
            }
            else {
              index = min(generation+(world[x][y]/2), 255);
              
              leds[XYsafe(x,y)]= ColorFromPalette(currentPalette, index);               
            }
        }      
        else leds[ XYsafe( x, y ) ] = deadColor;
      #else // Static color for living cells.
        if ( world[x][y] > 0 ) leds[XYsafe(x,y)] = aliveColor;
        else leds[XYsafe(x,y)] = deadColor;
      #endif
    }
  }
  FastLED.show();
}

//Non-serpentine layout
//1 3
//2 4
uint8_t kMatrixWidth = WORLD_WIDTH;
uint8_t kMatrixHeight = WORLD_HEIGHT;
uint16_t XYsafe( uint8_t x, uint8_t y ) {
  if (x >= kMatrixWidth || y >= kMatrixHeight || x < 0 || y < 0)
    return -1;

  // determine which panel this pixel falls on
  const uint8_t panel_size = 8;
  uint8_t x_panel = x / panel_size;
  uint8_t y_panel = y / panel_size;
  // and where in the leds[] array this panel starts
  uint16_t paneloffset = (2 * x_panel + y_panel) * panel_size * panel_size;

  // constrain coordinates to the panel size
  x %= panel_size;
  y %= panel_size;

  return paneloffset + x + y * panel_size;
}
