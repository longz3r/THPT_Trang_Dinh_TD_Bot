// import thư viện
#include <ShiftRegister74HC595.h>
#include <Arduino.h>
#include <SPI.h>

ShiftRegister74HC595<1> shiftRegister_DC(4, 17, 21); // (data pin, clock pin, latch pin)

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  pinMode(25, OUTPUT);

  // --- Setup for Shield #1 (DC) ---
  // ledcSetup(0, 1000, 8); ledcAttachPin(DC_PWM1_PIN, 0);
}

void loop() {
  Serial.println("FORWARD");
  shiftRegister_DC.set(2, HIGH); // turn off pin 2
  shiftRegister_DC.set(3, LOW); // turn off pin 3
  analogWrite(25, 255); // full speed ahead
  delay(3000);
  shiftRegister_DC.set(2, LOW); // turn off pin 2
  Serial.println("REVERSE");
  shiftRegister_DC.set(3, HIGH); // turn off pin 3
  analogWrite(25, 150); // full speed ahead

  delay(3000);
  analogWrite(25, 200); // full speed ahead

  delay(3000);
}