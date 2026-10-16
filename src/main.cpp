#include <Arduino.h>

// LED pins
const int RED_LED    = A1;
const int GREEN_LED  = A2;
const int YELLOW_LED = A3;
const int BLUE_LED   = A4;

const int LEDS[] = {RED_LED, GREEN_LED, YELLOW_LED, BLUE_LED};
const int NUM_LEDS = sizeof(LEDS) / sizeof(LEDS[0]);

// Color buttons (wired to GND, internal pull-up), same index order as LEDS
const int COLOR_BUTTONS[] = {2, 3, 4, 5};   // red, green, yellow, blue
const char *COLOR_NAMES[] = {"red", "green", "yellow", "blue"};

// Start button (wired between pin 6 and GND, uses internal pull-up)
const int BUTTON_1 = 6;

// Buzzer and note frequencies (Hz), same index order as LEDS
const int BUZZER = A0;
const unsigned int COLOR_NOTES[] = {587, 554, 494, 440};   // red D5, green C#5, yellow B4, blue A4

// C major arpeggio for winning: C5 E5 G5 C6 E6 G6 C7
const unsigned int WIN_ARPEGGIO[] = {523, 659, 784, 1047, 1319, 1568, 2093};
const int WIN_ARPEGGIO_LEN = sizeof(WIN_ARPEGGIO) / sizeof(WIN_ARPEGGIO[0]);

// Idle rotation order: blue -> yellow -> green -> red
const int ROTATION_ORDER[] = {BLUE_LED, YELLOW_LED, GREEN_LED, RED_LED};
const unsigned long ROTATION_STEP_MS = 300;

// Game settings
const int MAX_ROUNDS = 10;
const unsigned long SHOW_ON_MS  = 500;   // how long each LED is shown
const unsigned long SHOW_OFF_MS = 250;   // gap between shown LEDs
const unsigned long DEBOUNCE_MS = 50;

int sequence[MAX_ROUNDS];

void setAllLeds(int state) {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS[i], state);
  }
}

void blinkAll(int times, unsigned long onMs, unsigned long offMs) {
  for (int i = 0; i < times; i++) {
    setAllLeds(HIGH);
    delay(onMs);
    setAllLeds(LOW);
    delay(offMs);
  }
}

bool buttonPressed() {
  // Button reads LOW when pressed (INPUT_PULLUP)
  return digitalRead(BUTTON_1) == LOW;
}

// Cycle the LEDs until the start button is pressed and released
void waitForStart() {
  int step = 0;
  unsigned long lastStep = 0;

  while (!buttonPressed()) {
    if (millis() - lastStep >= ROTATION_STEP_MS) {
      setAllLeds(LOW);
      digitalWrite(ROTATION_ORDER[step], HIGH);
      step = (step + 1) % NUM_LEDS;
      lastStep = millis();
    }
  }

  setAllLeds(LOW);
  delay(DEBOUNCE_MS);
  while (buttonPressed()) {}    // wait for release
  delay(DEBOUNCE_MS);
}

// Show the sequence up to (and including) the given round
void showSequence(int length) {
  for (int i = 0; i < length; i++) {
    digitalWrite(LEDS[sequence[i]], HIGH);
    tone(BUZZER, COLOR_NOTES[sequence[i]]);
    delay(SHOW_ON_MS);
    noTone(BUZZER);
    digitalWrite(LEDS[sequence[i]], LOW);
    delay(SHOW_OFF_MS);
  }
}

// All LEDs on with a quick rising two-note chirp
void playRoundSuccess() {
  setAllLeds(HIGH);
  tone(BUZZER, 880);    // A5
  delay(120);
  tone(BUZZER, 1175);   // D6
  delay(200);
  noTone(BUZZER);
  delay(180);
  setAllLeds(LOW);
}

// Low descending buzz with a long flash
void playGameOver() {
  setAllLeds(HIGH);
  tone(BUZZER, 220);    // A3
  delay(400);
  tone(BUZZER, 147);    // D3
  delay(600);
  noTone(BUZZER);
  setAllLeds(LOW);
}

// C major arpeggio with the LEDs chasing along, then a final flash
void playWin() {
  for (int i = 0; i < WIN_ARPEGGIO_LEN; i++) {
    setAllLeds(LOW);
    digitalWrite(ROTATION_ORDER[i % NUM_LEDS], HIGH);
    tone(BUZZER, WIN_ARPEGGIO[i]);
    delay(i == WIN_ARPEGGIO_LEN - 1 ? 600 : 150);
  }
  noTone(BUZZER);
  setAllLeds(LOW);
  blinkAll(5, 100, 100);
}

// Block until a color button is pressed and released; returns its index.
// The matching LED stays lit while the button is held.
int readColorButton() {
  while (true) {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (digitalRead(COLOR_BUTTONS[i]) == LOW) {
        digitalWrite(LEDS[i], HIGH);
        tone(BUZZER, COLOR_NOTES[i]);
        delay(DEBOUNCE_MS);
        while (digitalRead(COLOR_BUTTONS[i]) == LOW) {}
        noTone(BUZZER);
        digitalWrite(LEDS[i], LOW);
        delay(DEBOUNCE_MS);
        return i;
      }
    }
  }
}

// Returns true if the player repeats the first `length` steps correctly
bool playerRepeats(int length) {
  for (int i = 0; i < length; i++) {
    int pressed = readColorButton();
    if (pressed != sequence[i]) {
      Serial.print("wrong: pressed ");
      Serial.print(COLOR_NAMES[pressed]);
      Serial.print(", expected ");
      Serial.println(COLOR_NAMES[sequence[i]]);
      return false;
    }
  }
  return true;
}

void setup() {
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LEDS[i], OUTPUT);
    pinMode(COLOR_BUTTONS[i], INPUT_PULLUP);
  }
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON_1, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("initiated");
  delay(1000);
}

void loop() {
  waitForStart();
  Serial.println("game started");

  // Seed from button-press timing so each game is different
  randomSeed(micros());
  for (int i = 0; i < MAX_ROUNDS; i++) {
    sequence[i] = random(NUM_LEDS);
  }

  blinkAll(3, 200, 200);
  delay(2000);

  for (int round = 1; round <= MAX_ROUNDS; round++) {
    Serial.print("round ");
    Serial.println(round);

    showSequence(round);

    if (!playerRepeats(round)) {
      Serial.print("game over, reached round ");
      Serial.println(round);
      playGameOver();
      delay(500);
      return;                   // back to idle rotation
    }

    delay(300);
    if (round < MAX_ROUNDS) {
      playRoundSuccess();
      delay(700);               // pause before next round
    }
  }

  Serial.println("you win!");
  playWin();
}
