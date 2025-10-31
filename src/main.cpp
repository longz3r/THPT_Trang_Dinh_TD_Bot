// import thư viện
#include <ShiftRegister74HC595.h>
#include <Arduino.h>
#include <SPI.h>

ShiftRegister74HC595<1> shiftRegister_DC(4, 17, 21); // (data pin, clock pin, latch pin)

enum Direction {
  FORWARD,
  REVERSE,
  STOP
};

void setDCMotorSpeed(int motorNum, int speed) {
  switch (motorNum) {
    case 1:
      analogWrite(25, speed); // set speed for motor 1
      break;
    default:
      return;
      break;
  }
}

void setDCMotorDirection(int motorNum, Direction direction) {
  u_int8_t pinA, pinB;
  switch (motorNum) {
    case 1:
      pinA = 2;
      pinB = 3;
      break;
    case 2:
      pinA = 1;
      pinB = 4;
      break;
    case 3:
      pinA = 0;
      pinB = 6;
      break;
    case 4:
      pinA = 5;
      pinB = 7;
      break;
    default:
      return;
      break;
  }

  switch (direction) {
    case FORWARD:
      shiftRegister_DC.setNoUpdate(pinA, HIGH);
      shiftRegister_DC.setNoUpdate(pinB, LOW);
      break;
    case REVERSE:
      shiftRegister_DC.setNoUpdate(pinA, LOW);
      shiftRegister_DC.setNoUpdate(pinB, HIGH);
      break;
    case STOP:
      shiftRegister_DC.setNoUpdate(pinA, LOW);
      shiftRegister_DC.setNoUpdate(pinB, LOW);
      break;
  }

  shiftRegister_DC.updateRegisters();
}

void setup() {
  Serial.begin(115200);
  pinMode(25, OUTPUT); // PWM M1
  pinMode(2, OUTPUT); // LED
  pinMode(13, OUTPUT); // DC ENABLE SHIFT REGISTER
  digitalWrite(13, LOW);
  digitalWrite(25, HIGH);

}

void loop() {
  Serial.println("FORWARD");
  digitalWrite(2, HIGH);
  // setDCMotorDirection(1, FORWARD);
  shiftRegister_DC.set(2, HIGH);
  shiftRegister_DC.set(3, LOW);
  delay(3000);
  Serial.println("REVERSE");
  digitalWrite(2, LOW);
  shiftRegister_DC.set(2, LOW);
  shiftRegister_DC.set(3, HIGH);
  shiftRegister_DC.set(6, HIGH);
  // setDCMotorDirection(1, REVERSE);
  delay(3000);
}