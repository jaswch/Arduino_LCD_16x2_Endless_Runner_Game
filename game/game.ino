#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#define PIN_BUTTON 2
#define PIN_AUTOPLAY 1
#define SPRITE_RUN1 1  //Car sprite
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_JUMP_UPPER '.'         
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' '      
#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7
#define CAR_HORIZONTAL_POSITION 1    // Horizontal position of CAR on screen
#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2
#define CAR_POSITION_OFF 0          // CAR is invisible
#define CAR_POSITION_RUN_LOWER_1 1  // CAR is running on lower row (pose 1)
#define CAR_POSITION_RUN_LOWER_2 2  //                              (pose 2)
#define CAR_POSITION_JUMP_1 3       // Starting a jump
#define CAR_POSITION_JUMP_2 4       // Half-way up
#define CAR_POSITION_JUMP_3 5       // Jump is on upper row
#define CAR_POSITION_JUMP_4 6       // Jump is on upper row
#define CAR_POSITION_JUMP_5 7       // Jump is on upper row
#define CAR_POSITION_JUMP_6 8       // Jump is on upper row
#define CAR_POSITION_JUMP_7 9       // Half-way down
#define CAR_POSITION_JUMP_8 10      // About to land
#define CAR_POSITION_RUN_UPPER_1 11 // CAR is running on upper row (pose 1)
#define CAR_POSITION_RUN_UPPER_2 12 //                              (pose 2)
LiquidCrystal_I2C lcd(0x27, 16, 2);
static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];
static bool buttonPushed = false;
void initializeGraphics() {
  static byte graphics[] = {
    // Run position 1
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Run position 2
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Jump
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Jump lower
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Ground
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    // Ground right
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    // Ground left
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
  };
  int i;
  // Skip using character 0, this allows lcd.print() to be used to
  // quickly draw multiple characters
  for (i = 0; i < 7; ++i) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

// Slide the terrain to the left in half-character increments
//
void advanceTerrain(char* terrain, byte newTerrain) {
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
    switch (current) {
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

bool drawCAR(byte position, char* terrainUpper, char* terrainLower, unsigned int score) {
  bool collide = false;
  char upperSave = terrainUpper[CAR_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[CAR_HORIZONTAL_POSITION];
  byte upper, lower;
  switch (position) {
    case CAR_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case CAR_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case CAR_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case CAR_POSITION_JUMP_1:
    case CAR_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case CAR_POSITION_JUMP_2:
    case CAR_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case CAR_POSITION_JUMP_3:
    case CAR_POSITION_JUMP_4:
    case CAR_POSITION_JUMP_5:
    case CAR_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case CAR_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case CAR_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }
  if (upper != ' ') {
    terrainUpper[CAR_HORIZONTAL_POSITION] = upper;
    collide = (upperSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  if (lower != ' ') {
    terrainLower[CAR_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }

  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;

  // Draw the scene
  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';
  char temp = terrainUpper[16 - digits];
  terrainUpper[16 - digits] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(terrainUpper);
  terrainUpper[16 - digits] = temp;
  lcd.setCursor(0, 1);
  lcd.print(terrainLower);

  lcd.setCursor(16 - digits, 0);
  lcd.print(score);

  terrainUpper[CAR_HORIZONTAL_POSITION] = upperSave;
  terrainLower[CAR_HORIZONTAL_POSITION] = lowerSave;
  return collide;
}

// Handle the button push as an interrupt
void buttonPush() {
  buttonPushed = true;
}

void setup() {
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_BUTTON, HIGH);
  pinMode(PIN_AUTOPLAY, OUTPUT);
  digitalWrite(PIN_AUTOPLAY, HIGH);

  // Digital pin 2 maps to interrupt 0
  attachInterrupt(0/*PIN_BUTTON*/, buttonPush, FALLING);

  initializeGraphics();

  lcd.init();
  lcd.backlight();
}

void loop() {
  static byte CARPos = CAR_POSITION_RUN_LOWER_1;
  static byte newTerrainType = TERRAIN_EMPTY;
  static byte newTerrainDuration = 1;
  static bool playing = false;
  static bool blink = false;
  static unsigned int distance = 0;

  if (!playing) {
    drawCAR((blink) ? CAR_POSITION_OFF : CARPos, terrainUpper, terrainLower, distance >> 3);
    if (blink) {
      lcd.setCursor(0, 0);
      lcd.print("Press Start");
    }
    delay(100);
    blink = !blink;
    if (buttonPushed) {
      initializeGraphics();
      CARPos = CAR_POSITION_RUN_LOWER_1;
      playing = true;
      buttonPushed = false;
      distance = 0;
    }
    return;
  }

  // Shift the terrain to the left
  advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

  // Make new terrain to enter on the right
  if (--newTerrainDuration == 0) {
    if (newTerrainType == TERRAIN_EMPTY) {
      newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
      newTerrainDuration = 10 + random(6);
    } else {
      newTerrainType = TERRAIN_EMPTY;
      newTerrainDuration = 10 + random(6);
    }
  }

  if (buttonPushed) {
    if (CARPos <= CAR_POSITION_RUN_LOWER_2) CARPos = CAR_POSITION_JUMP_1;
    buttonPushed = false;
  }

  if (drawCAR(CARPos, terrainUpper, terrainLower, distance >> 3)) {
    playing = false; // The CAR collided with something. Too bad.
    for (int i = 0; i <= 2; i++) {
    }
  } else {
    if (CARPos == CAR_POSITION_RUN_LOWER_2 || CARPos == CAR_POSITION_JUMP_8) {
      CARPos = CAR_POSITION_RUN_LOWER_1;
    } else if ((CARPos >= CAR_POSITION_JUMP_3 && CARPos <= CAR_POSITION_JUMP_5) && terrainLower[CAR_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
      CARPos = CAR_POSITION_RUN_UPPER_1;
    } else if (CARPos >= CAR_POSITION_RUN_UPPER_1 && terrainLower[CAR_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
      CARPos = CAR_POSITION_JUMP_5;
    } else if (CARPos == CAR_POSITION_RUN_UPPER_2) {
      CARPos = CAR_POSITION_RUN_UPPER_1;
    } else {
      ++CARPos;
    }
    ++distance;

    digitalWrite(PIN_AUTOPLAY, terrainLower[CAR_HORIZONTAL_POSITION + 2] == SPRITE_TERRAIN_EMPTY ? HIGH : LOW);
  }
}
