#include <ShiftRegister74HC595.h>
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PWMServoDriver.h>

// --- CONFIGURATION & PIN DEFINITIONS ---

// DC Motor Map (PCA9685 Channels)
#define CH_MOTOR_1 0
#define CH_MOTOR_2 1
#define CH_MOTOR_3 2
#define CH_MOTOR_4 3

// Servo Map (PCA9685 Channels)
#define CH_SERVO_LIFT 8
#define CH_SERVO_GRIPPER_ROT 9
#define CH_SERVO_GRIPPER_L 12
#define CH_SERVO_GRIPPER_R 13
#define CH_SERVO_AUX_1 15
#define CH_SERVO_AUX_2 14

// Shift Register Pins (ESP32)
// DC Direction Control
#define SR_DC_DATA  15
#define SR_DC_CLOCK 4
#define SR_DC_LATCH 16

// RC Receiver Pins
const int rc_pins[] = {13, 12, 14, 27, 26, 25, 33, 32, 34};
const int channelCount = 9;

// --- OBJECT INSTANTIATION ---

ShiftRegister74HC595<1> shiftRegister_DC(SR_DC_DATA, SR_DC_CLOCK, SR_DC_LATCH);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

enum Direction { FORWARD, REVERSE, STOP };

// --- CLASSES ---

class InterruptPWMTracker {
public:
  volatile unsigned long lastRaising;
  volatile unsigned long durationUp;
  const int pin;
  void (*callback)(void);

  InterruptPWMTracker(int pin, void (*callback)(void)) : pin(pin), callback(callback) {}

  void setup() {
    pinMode(pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(pin), callback, CHANGE);
  }

  // IRAM_ATTR is crucial for ESP32 interrupts to prevent crashes/latency
  void IRAM_ATTR updateFromPinChangeInterrupt() {
    unsigned long current_micros = micros();
    if (digitalRead(pin)) {
      lastRaising = current_micros;
    } else {
      durationUp = current_micros - lastRaising;
    }
  }
};

// --- FORWARD DECLARATIONS OF ISRs ---
void IRAM_ATTR channel1onChangeInterrupt();
void IRAM_ATTR channel2onChangeInterrupt();
void IRAM_ATTR channel3onChangeInterrupt();
void IRAM_ATTR channel4onChangeInterrupt();
void IRAM_ATTR channel5onChangeInterrupt();
void IRAM_ATTR channel6onChangeInterrupt();
void IRAM_ATTR channel7onChangeInterrupt();
void IRAM_ATTR channel8onChangeInterrupt();
void IRAM_ATTR channel9onChangeInterrupt();

InterruptPWMTracker channels[channelCount] = {
    InterruptPWMTracker(rc_pins[0], channel1onChangeInterrupt),
    InterruptPWMTracker(rc_pins[1], channel2onChangeInterrupt),
    InterruptPWMTracker(rc_pins[2], channel3onChangeInterrupt),
    InterruptPWMTracker(rc_pins[3], channel4onChangeInterrupt),
    InterruptPWMTracker(rc_pins[4], channel5onChangeInterrupt),
    InterruptPWMTracker(rc_pins[5], channel6onChangeInterrupt),
    InterruptPWMTracker(rc_pins[6], channel7onChangeInterrupt),
    InterruptPWMTracker(rc_pins[7], channel8onChangeInterrupt),
    InterruptPWMTracker(rc_pins[8], channel9onChangeInterrupt),
};

// --- ISR IMPLEMENTATIONS ---
void IRAM_ATTR channel1onChangeInterrupt() { channels[0].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel2onChangeInterrupt() { channels[1].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel3onChangeInterrupt() { channels[2].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel4onChangeInterrupt() { channels[3].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel5onChangeInterrupt() { channels[4].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel6onChangeInterrupt() { channels[5].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel7onChangeInterrupt() { channels[6].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel8onChangeInterrupt() { channels[7].updateFromPinChangeInterrupt(); }
void IRAM_ATTR channel9onChangeInterrupt() { channels[8].updateFromPinChangeInterrupt(); }

// --- HELPER FUNCTIONS ---

void setDCMotorState(int motorNum, Direction direction, int pwmValue) {
  uint8_t pinA, pinB;
  
  // Map Motor Number to Shift Register Pins
  switch (motorNum) {
    case 1: pinA = 2; pinB = 3; break;
    case 2: pinA = 1; pinB = 4; break;
    case 3: pinA = 0; pinB = 6; break;
    case 4: pinA = 5; pinB = 7; break;
    default: return;
  }

  // Update Shift Register buffer
  if (direction == FORWARD) {
    shiftRegister_DC.setNoUpdate(pinA, HIGH);
    shiftRegister_DC.setNoUpdate(pinB, LOW);
  } else if (direction == REVERSE) {
    shiftRegister_DC.setNoUpdate(pinA, LOW);
    shiftRegister_DC.setNoUpdate(pinB, HIGH);
  } else {
    shiftRegister_DC.setNoUpdate(pinA, LOW);
    shiftRegister_DC.setNoUpdate(pinB, LOW);
  }

  // Set PWM
  pwm.setPWM(motorNum - 1, 0, pwmValue);
}

void setServoAngle(uint8_t channel, int angle) {
  // Constrain angle to safe limits
  // angle = constrain(angle, 0, 180);
  // int pulseLen = map(angle, 0, 180, 102, 512);
  pwm.setPWM(channel, 0, map(angle, 0, 180, 102, 512));
}

void handleDrive(int speedX, int speedY, int rotation) {
  // Deadband to prevent buzzing
  if (abs(speedX) < 20) speedX = 0;
  if (abs(speedY) < 20) speedY = 0;
  if (abs(rotation) < 20) rotation = 0;

  // Constrain Inputs
  speedX = constrain(speedX, -500, 500);
  speedY = constrain(speedY, -500, 500);
  rotation = constrain(rotation, -500, 500);

  if (speedX == 0 && speedY == 0 && rotation == 0) {
    setDCMotorState(1, STOP, 0);
    setDCMotorState(2, STOP, 0);
    setDCMotorState(3, STOP, 0);
    setDCMotorState(4, STOP, 0);
    shiftRegister_DC.updateRegisters();
    return;
  }

  // Mecanum Kinematics
  int FL = speedY + speedX + rotation;
  int FR = speedY - speedX - rotation;
  int RL = speedY - speedX + rotation;
  int RR = speedY + speedX - rotation;

  // Normalize if exceeding max PWM
  int maxPower = max(max(abs(FL), abs(FR)), max(abs(RL), abs(RR)));
  if (maxPower > 500) {
    FL = map(FL, -maxPower, maxPower, -500, 500);
    FR = map(FR, -maxPower, maxPower, -500, 500);
    RL = map(RL, -maxPower, maxPower, -500, 500);
    RR = map(RR, -maxPower, maxPower, -500, 500);
  }

  // Apply power
  // Helper lambda to get direction
  auto getDir = [](int p) { return (p > 0) ? FORWARD : ((p < 0) ? REVERSE : STOP); };
  
  setDCMotorState(1, getDir(FR), map(abs(FR), 0, 500, 0, 4095));
  setDCMotorState(2, getDir(FL), map(abs(FL), 0, 500, 0, 4095));
  setDCMotorState(3, getDir(RL), map(abs(RL), 0, 500, 0, 4095));
  setDCMotorState(4, getDir(RR), map(abs(RR), 0, 500, 0, 4095));
  
  shiftRegister_DC.updateRegisters();
}

void handleManipulators() {
  // --- LIFT SERVO ---
  if (channels[7].durationUp > 1900) {
    setServoAngle(8, 90); 
  } else if (channels[7].durationUp < 1600 && channels[7].durationUp > 1400) {
    setServoAngle(8, 70); 
  } else if (channels[7].durationUp < 1100) {
     setServoAngle(8, 0); 
  }

  // --- GRIPPER & ARM ROTATION ---
  if (channels[4].durationUp > 1900) {
    setServoAngle(12, 45);
  } else if (channels[4].durationUp > 1400 && channels[4].durationUp < 1600) {
    setServoAngle(12, 135);
    setServoAngle(CH_SERVO_GRIPPER_R, 70);
  } else if (channels[4].durationUp < 1100) {
    setServoAngle(CH_SERVO_GRIPPER_R, 155);
  }

  // --- GRIPPER ROTATION ---
  if (channels[5].durationUp < 1500) {
    setServoAngle(CH_SERVO_GRIPPER_ROT, 10);
  } else if (channels[5].durationUp >= 1500) {
    // Map channel 9 (Aux) to rotation angle
    int angle = map(channels[8].durationUp, 1000, 2000, 40, 80);
    setServoAngle(CH_SERVO_GRIPPER_ROT, angle);
  }
}

// --- MAIN SETUP & LOOP ---

void setup() {
  // Serial.begin(115200); // Debugging if needed
  pinMode(2, OUTPUT); // Built-in LED
  
  pwm.begin();
  pwm.setOscillatorFrequency(27000000); // Tuned slightly often fixes servo jitter
  pwm.setPWMFreq(50); 

  // Initial State: Motors STOP
  handleDrive(0, 0, 0);

  // Initial Servos
  pwm.setPWM(CH_SERVO_AUX_1, 0, 150);
  pwm.setPWM(CH_SERVO_AUX_2, 0, 600);

  digitalWrite(2, HIGH); // LED ON indicating ready

  // Initialize Interrupts
  for (int i = 0; i < channelCount; i++) {
    channels[i].setup();
  }
}

void loop() {
  // Calculate RC inputs (Centered at 0)
  // Assuming RC range is roughly 1000 to 2000. 1500 is center.
  int speedY = channels[1].durationUp - 1500;
  int rotation = channels[3].durationUp - 1500;
  int speedX = channels[0].durationUp - 1500;

  handleDrive(speedX, speedY, rotation);
  // Serial.print("CH1: ");
  // Serial.print(channels[0].durationUp);
  // Serial.print(" | CH2: ");
  // Serial.print(channels[1].durationUp);
  // Serial.print(" | CH3: ");
  // Serial.print(channels[2].durationUp);
  // Serial.print(" | CH4: ");
  // Serial.print(channels[3].durationUp);
  // Serial.print(" | CH5: ");
  // Serial.print(channels[4].durationUp);
  // Serial.print(" | CH6: ");
  // Serial.print(channels[5].durationUp);
  // Serial.print(" | CH7: ");
  // Serial.print(channels[6].durationUp);
  // Serial.print(" | CH8: ");
  // Serial.print(channels[7].durationUp);
  // Serial.print(" | CH9: ");
  // Serial.println(channels[8].durationUp);
  handleManipulators();
  
  // Small delay not needed as handleDrive has no blocking code, 
  // but keeps the loop from running unnecessarily fast if no work is done.
  // However, for smoothest stepper, we remove delay.
}