#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)

//Adafruit_PCD8544 display = Adafruit_PCD8544(SCLK, DIN, D/C, CS, RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

#define SCREEN_MAX_X 83
#define SCREEN_MAX_Y 47
#define SHIP_LENGTH 14
#define SHIP_HEIGHT 4
#define SHIP_YPOS 44
#define GARBAGE_SIZE 5
#define MENU 0
#define GAME 1
#define SCORE 2
#define LEFT 0
#define RIGHT 1

const PROGMEM uint8_t heart[] = {0x36, 0x00, 0x7f, 0x00, 0x7f, 0x00, 0x7f, 0x00, 0x3e, 0x00, 0x1c, 0x00, 0x08};
const PROGMEM uint8_t bomb[] = {0x1f, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x04};

int screen = MENU;
int selection = LEFT;

int mode = 0; // 0 - zwykly, 1 - bomby

int shipPosition = 0;
bool shipColor = false;

const int SW_pin = 8; // digital pin
const int X_pin = 0; // analog pin
const int Y_pin = 1; // analog pin
const int threshold = 100; // joystick threshold

unsigned long drawGarbagePreviousMillis = 0;
long drawGarbageInterval = 33;

int lastpos = 0;

int score = 0;
int lives = 3;

int prevButtonState = 1;

class Garbage {
  public:
    int x = -10;
    int y = 10;
    bool type = false;

    Garbage(int x) {
      this->x = x;
    }

    Garbage(int x, bool type) {
      this->x = x;
      this->type = type;
    }
};

Garbage garbage = Garbage(10);

void readControls() {
  int posY = analogRead(Y_pin);
  int posX = analogRead(X_pin);
  if (posX > 1023 - threshold) { // right
    shipPosition++;
    if (shipPosition > SCREEN_MAX_X - SHIP_LENGTH)
      shipPosition = SCREEN_MAX_X - SHIP_LENGTH;
    if(screen == MENU)
      selection = RIGHT;
  }
  else if (posX < threshold) { // left
    shipPosition--;
    if (shipPosition < 0)
      shipPosition = 0;
    if(screen == MENU)
      selection = LEFT;
  }

  int sw = digitalRead(SW_pin);
  if (sw == 0 && prevButtonState == 1) {
    pushButton();
  }
  prevButtonState = sw;
}

void pushButton() {
  if(screen == MENU) {
    if(selection == LEFT)
      mode = 0;
    else if(selection == RIGHT)
      mode = 1;
    gameInit();
  }
  else if(screen == GAME) {
    if(mode == 0)
      changeShipColor();
  }
  else if(screen == SCORE) {
    screen = MENU;
  }
}

void changeShipColor() {
  if (!shipColor)
    shipColor = true;
  else
    shipColor = false;
}

void spawn() {
  int xpos = lastpos;
  do {
    xpos = 10 + rand() % 71;
  } while (abs(xpos - lastpos) < 10);
  lastpos = xpos;

  int type = rand() % 100;
  if(mode == 0) {
    if(type < 50)
      garbage.type = false;
    else
      garbage.type = true;
  }
  else if(mode == 1) {
    if(type < 30)
      garbage.type = false;
    else
      garbage.type = true;
  }

  garbage.x = xpos;
  garbage.y = 10;
}

void drawGarbage() {
  if (!garbage.type && mode == 0) {
      display.fillRect(garbage.x, garbage.y, GARBAGE_SIZE, GARBAGE_SIZE, BLACK);
  }
  else if(!garbage.type && mode == 1) {
//    display.setTextColor(BLACK);
//    display.setCursor(garbage.x, garbage.y);
//    display.setTextSize(1);
//    display.print("T");
    display.drawBitmap(garbage.x, garbage.y, bomb, 10, 7, BLACK);
  }
  else {
      display.drawRect(garbage.x, garbage.y, GARBAGE_SIZE, GARBAGE_SIZE, BLACK);
  }
}

void drawShip() {
  if (!shipColor) {
    display.fillRect(shipPosition, SHIP_YPOS, SHIP_LENGTH, SHIP_HEIGHT, BLACK);
  }
  else {
    display.drawRect(shipPosition, SHIP_YPOS, SHIP_LENGTH, SHIP_HEIGHT, BLACK);
  }
}

void dropGarbage() {
  garbage.y++;
  if (checkCollision())
    removeGarbage(SHIP_YPOS);
  else
    removeGarbage(SCREEN_MAX_Y);
}

bool checkCollision() {
  return (garbage.x > shipPosition - GARBAGE_SIZE) && (garbage.x < shipPosition + SHIP_LENGTH);
}

void removeGarbage(int ypos) {
  if (garbage.y > ypos) {
    if(mode == 0) {
      if (ypos == SHIP_YPOS && garbage.type == shipColor)
        score += 1;
      else if (ypos == SCREEN_MAX_Y || (ypos == SHIP_YPOS && garbage.type != shipColor))
        lives--;
    }
    else if(mode == 1) {
      if (ypos == SHIP_YPOS && garbage.type)
        score += 1;
      else if ((ypos == SCREEN_MAX_Y && garbage.type) || (ypos == SHIP_YPOS && !garbage.type))
        lives--;
    }
    if(lives < 0) screen = SCORE;
    spawn();
  }
}

void drawScore() {
  display.setTextColor(BLACK);
  display.setCursor(70, 2);
  display.setTextSize(1);
  display.print(score);
}

void drawLives() {
  display.setTextColor(BLACK);
  display.setCursor(10, 2);
  display.setTextSize(1);
//  display.print(lives);
  for(int i = 0; i < lives; i++)
    display.drawBitmap(10 + i*9, 2, heart, 14, 7, BLACK);
//    display.print("<3 ");
}

void drawMenu() {
  display.setTextColor(BLACK);
  display.setTextSize(1);
  display.setCursor(4, 2);
  display.print("GARBAGE");
  display.setCursor(26, 12);
  display.print("COLLECTOR");

  if(selection == LEFT)
    display.setTextColor(WHITE, BLACK);
  else if(selection == RIGHT)
    display.setTextColor(BLACK);
  display.setCursor(4, 30);
  display.print("COLORS");
  
  if(selection == LEFT)
    display.setTextColor(BLACK);
  else if(selection == RIGHT)
    display.setTextColor(WHITE, BLACK);
  display.setCursor(50, 30);
  display.print("BOMBS");
}

void drawGameOver(){
  display.setTextColor(BLACK);
  display.setTextSize(1);
  display.setCursor(15, 2);
  display.print("GAME OVER");
  display.setCursor(2, 30);
  display.print("Score: ");
  display.setTextSize(2);
  display.setCursor(38, 23);
  display.print(score);
}

void gameInit() {
  if(mode == 0)
    drawGarbageInterval = 33;
  else if(mode == 1)
    drawGarbageInterval = 25;

  spawn();

  shipPosition = 36;
  score = 0;
  lives = 3;
  screen = GAME;
}

// main

void setup() {
  Serial.begin(9600);
  pinMode(SW_pin, INPUT_PULLUP);
  digitalWrite(SW_pin, HIGH);

  srand(time(0));
  display.begin();                          //uruchom ekran
  display.setContrast(50);                  //ustaw kontrast
  display.clearDisplay();                   //wyczyść bufor ekranu
}

void loop() {
  readControls();
  if(screen == MENU) {
    drawMenu();
  }

  else if(screen == GAME) {
    unsigned long currentMillis = millis();
  
    if (currentMillis - drawGarbagePreviousMillis >= drawGarbageInterval) {
      drawGarbagePreviousMillis = currentMillis;
      dropGarbage();
    }
  
    drawShip();
    drawGarbage();
    drawScore();
    drawLives();
  }

  else if(screen == SCORE) {
    drawGameOver();
  }
  
  display.display();
  display.clearDisplay();
}
