//R.I.P John Conway (1937–2020)

//Size of the game world
#define WORLD_WIDTH   16
#define WORLD_HEIGHT  16

//Integer arrays to store cell states. The value of the cell is the number of generations its been alive.
int16_t world[WORLD_HEIGHT][WORLD_WIDTH];
int16_t newWorld[WORLD_HEIGHT][WORLD_WIDTH];

#define GEN_DELAY 75 //Delay in milliseconds between generation.
#define GEN_LIMIT 5000 //Resets the game after this number of generations.
#define IDENTICIAL_LIMIT 50 //Number of identical generations before resetting.

bool resetW = false; //Set to true to reset the game.
uint16_t generation; //Current generation.
uint16_t currentLiving; //Number of living cells of the currently displayed generation.
uint16_t nextLiving; //Number of living cells of the next generation.
uint16_t prevLiving; ////Number of living cells of the next generation.
uint16_t sameLivingCount; //Number of generations where the next and current generations or the next and previous generations have the same number of living cells.

void setup() {
  Serial.begin(115200);
  randomSeed(A0); //Initialize RNG with noise.
  resetWorld(); //Initialize game.
}

void loop() {
  while(!resetW){
    drawWorld(); //Draw last generated game state.
    nextGen(); //Calculate next generation.
    countLiving(); //Count living cells in current and next generations.
    checkReset(); //Check for reset conditions. 
    copyWorld(); //Copy the next generation into the current generation for display next loop.
    delay(GEN_DELAY);
  }
  resetWorld();
}

void resetWorld(){
  Serial.println("Reset");
  resetW = false;
  sameLivingCount = 0;
  generation = 0;
  for (uint8_t  y = 0; y < WORLD_HEIGHT; y++){
    for (uint8_t  x = 0; x < WORLD_WIDTH; x++){
      world[y][x] = random(2);
    }
  }
}

void countLiving() {
  prevLiving = currentLiving;
  currentLiving = 0;
  nextLiving = 0;
  for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
    for (uint8_t x = 0; x < WORLD_WIDTH; x++) {
      if (world[y][x] > 0) {
        currentLiving++;
      }
      if (newWorld[y][x] > 0) {
        nextLiving++;
      }
    }
  }
}

void checkReset(){
  //Serial.println("Check Reset");
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
	Serial.println("Limit Reached");
	delay(200);
    resetW = true;
  }
}

//https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
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
  drawSerialWorld();
}

//Use a terminal emulator like putty for better readability.
void drawSerialWorld(){
  Serial.write(0x0C); //Clears the screen. Won't work for the Arduino serial monitor.
  Serial.println(generation);
  //Serial.println(sameLivingCount);
  for (uint8_t y = 0; y < WORLD_HEIGHT; y++){
    for (uint8_t x = 0; x < WORLD_WIDTH; x++){
        if (world[y][x] > 0){
          //uint16_t gen2 = world[y][x] % 10; //Mod 10 the generation for readability. 
		  //Serial.print("gen2");
          Serial.print("X");
        }
          else{
            Serial.print(".");
          }
        }
        Serial.println("");
    }
}
