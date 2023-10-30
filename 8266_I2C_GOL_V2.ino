#include <Arduino.h>
#include <Wire.h>
#include <EasyButton.h> //https://github.com/evert-arias/EasyButton
#include <U8g2lib.h> //https://github.com/olikraus/u8g2
#include "ESP8266TrueRandom.h" //https://github.com/marvinroger/ESP8266TrueRandom

#define BUTTON_PIN 3 
#define WORLD_WIDTH 128 
#define WORLD_HEIGHT 64 

uint8_t WORLD_W = WORLD_WIDTH;
uint8_t WORLD_H = WORLD_HEIGHT; 

bool runGOL = true; 
bool resetW = false; //Set to true to reset the game
bool stats = true; //Draw a list of stats at the bottom of the screen

EasyButton button(BUTTON_PIN); //Initialize EasyButton

void ScreenToggle() { //Turn the display on or off
  clearScreen();
  runGOL = !runGOL;
}

void StatsToggle() {
  stats = !stats;
  (stats) ? WORLD_H = WORLD_HEIGHT-10 : WORLD_H = WORLD_HEIGHT; //Reduce game area size for stats at the bottom
  resetW = true;
  runGOL = !runGOL;
}

void onPressedForDuration() { 
  resetW = true;
}

int16_t world[WORLD_HEIGHT][WORLD_WIDTH]; //Integer array for the currently displayed generation. Each value in the array is the number of generations that cell has been alive. 
int16_t newWorld[WORLD_HEIGHT][WORLD_WIDTH]; //Array for the next calculated generation.


#define GEN_DELAY 5 //Time in ms between generations.
#define GEN_LIMIT 5000 //Resets the game after 5000 generations.
#define AGE_LIMIT 255 //Add an age limit to cells.
#define IDENTICIAL_LIMIT 50 //Number of generations with an identical number of living cells. Resets the game after this limit is reached. 

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); //Initialize OLED.

int generation; //Current generation, starts at 0. Displayed in the stats. 
int currentLiving; //Number of currently living cells. Displayed in the stats. 
int nextLiving; //Number of living cells in the next calculated generation.
int prevLiving; //Number of living cells in the previous generation.
int sameLivingCount; //Number of generations that have had the same number of living cells.
int EPOCH = 1; //Number of times the game has reset since power on. Displayed in the stats. 
int cursor; //Offset used to properly display epoch in the stats. 

void setup() {
  Serial.begin(115200);
  button.begin();
  button.onSequence(2, 1000, ScreenToggle);
  button.onSequence(3, 2000, StatsToggle);
  button.onPressedFor(1000, onPressedForDuration);
  randomSeed(ESP8266TrueRandom.random()); //Use the ESP's onboard ADC to generate entropy for the RNG. 
  Wire.begin(2, 0); //SDA, SCL
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x13_tf);
  resetWorld(); //Initialize the game. 
}

void loop() {
  while (!resetW) {
    if (runGOL) 
    drawWorld(); //Display current generation
    nextGen(); //Generate next generation
    countLiving(); //Count living cells in current generation
    checkReset(); //Check for reset conditions
    copyWorld(); //Copy next generation for displaying next loop
    button.read(); //Read the pushbutton
    delay(GEN_DELAY); //Delay between generations
  }
  resetWorld();
}


void resetWorld() {
  Serial.println("Reset");
  resetW = false;
  sameLivingCount = 0;
  generation = 0;
  for (uint8_t  y = 0; y < WORLD_H; y++) {
    for (uint8_t  x = 0; x < WORLD_W; x++) {
      world[y][x] = random(2);
    }
  }
}

void countLiving() {
  prevLiving = currentLiving;
  currentLiving = 0;
  nextLiving = 0;
  for (uint8_t y = 0; y < WORLD_H; y++) {
    for (uint8_t x = 0; x < WORLD_W; x++) {
      if (world[y][x] > 0) {
        currentLiving++;
      }
      if (newWorld[y][x] > 0) {
        nextLiving++;
      }
    }
  }
}

void checkReset() {
  //Serial.println("Check Reset");
  if ((currentLiving == nextLiving) || (prevLiving == nextLiving)) { //If the current generation has the same number of living cells as the next or prior generation, increment the counter
    sameLivingCount++;
  }
  else {
    sameLivingCount = 0;
  }
  if (sameLivingCount > IDENTICIAL_LIMIT) { //Prevents endless loops
    //Serial.println("Limit Reached");
    delay(200);
    EPOCH+=1;
    resetW = true;
  }
  if (currentLiving == 0) {
    //Serial.println("No Living");
    delay(200);
    EPOCH+=1;
    resetW = true;
  }
  if (generation > GEN_LIMIT) { //Prevents the game going on forever
    delay(200);
    EPOCH+=1;
    resetW = true;
  }
}

//https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
void nextGen() {
  generation++;
  for (uint8_t y = 0; y < WORLD_H; y++) {
    for (uint8_t x = 0; x < WORLD_W; x++) {
      uint8_t neighbors = getNumberOfNeighbors(x, y);
      if ( world[y][x] > 0 && (neighbors == 2 || neighbors == 3 )) { //Can be simplified but easier to read this way
        newWorld[y][x] = world[y][x] + 1;
      }
      else if (world[y][x] > 0) {
        newWorld[y][x] = 0;
      }
      if (world[y][x] == 0 && neighbors == 3) {
        newWorld[y][x] = 1;
      }
      else if (world[y][x] == 0) {
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


void copyWorld() {
  for (uint8_t  y = 0; y < WORLD_H; y++) {
    for (uint8_t  x = 0; x < WORLD_W; x++) {
      world[y][x] = newWorld[y][x];
    }
  }
}

//Adapted from https://github.com/loelkes/arduino-gameoflife
int getNumberOfNeighbors(uint8_t x, uint8_t y) {

  uint8_t count = 0;
  boolean wrapX = true;
  boolean wrapY = true;

  uint8_t x_l = x - 1;
  uint8_t x_r = x + 1;
  uint8_t y_t = y - 1;
  uint8_t y_b = y + 1;

  //Allows cells on the top, bottom and side edges to influence cells on the other side. 
  if ( wrapX ) {
    if      ( x == 0 )  x_l = WORLD_W - 1;
    else if ( x >= WORLD_W - 1 )  x_r = 0;
  }
  if ( wrapY ) {
    if      ( y == 0 ) y_t = WORLD_H - 1;
    else if ( y >= WORLD_H - 1 )  y_b = 0;
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

void drawWorld() {
  //drawSerialWorld();
  drawOLEDWorld();
}

 void drawSerialWorld() {
   Serial.write(0x0C); //Clears screen, does not work in Arduino serial monitor
   Serial.println(generation);
   Serial.println(sameLivingCount);
   for (uint8_t y = 0; y < WORLD_H; y++) {
     for (uint8_t x = 0; x < WORLD_W; x++) {
       if (world[y][x] > 0) {
         uint8_t gen2 = world[y][x] % 10; //mod 10 for readability
         Serial.print("X");
       }
       else {
         Serial.print(".");
       }
     }
     Serial.println("");
   }
 }

void drawOLEDWorld() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  drawStats();
  for (uint8_t y = 0; y < WORLD_H; y++) {
    for (uint8_t x = 0; x < WORLD_W; x++) {
      if (world[y][x] > 0) {
        u8g2.drawPixel(x, y);
      }
    }
  }
  u8g2.sendBuffer();
}

void clearScreen() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void drawStats() {
  if (stats) {
    u8g2.setCursor(0, 64);
    u8g2.print(generation);
    u8g2.setCursor(64-8, 64);
    u8g2.print(currentLiving);
	//Epoch is right justified, so we move the cursor over for each power of 10.
    if (EPOCH < 10000) cursor = 4;  
    if (EPOCH < 1000) cursor = 3;
    if (EPOCH < 100) cursor = 2;
    if (EPOCH < 10) cursor = 1;
    u8g2.setCursor(128-(cursor*6), 64);
    u8g2.print(EPOCH); 
  }
}
