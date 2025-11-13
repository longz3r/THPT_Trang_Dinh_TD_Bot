// import thư viện
#include <ShiftRegister74HC595.h>
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PWMServoDriver.h>

ShiftRegister74HC595<1> shiftRegister_DC(15, 4, 16); // (data pin, clock pin, latch pin)
ShiftRegister74HC595<1> shiftRegister_STEP(17, 5, 18);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

enum Direction
{
  FORWARD,
  REVERSE,
  STOP
};

class InterruptPWMTracker
{
public:
  volatile unsigned long lastRaising;
  volatile unsigned long durationUp;

  const int pin;
  void (*callback)(void);

  InterruptPWMTracker(int pin, void (*callback)(void)) : pin(pin),
                                                         callback(callback)
  {
  }

  void setup()
  {
    pinMode(pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(pin), callback, CHANGE);
  }

  void updateFromPinChangeInterrupt()
  {
    unsigned long current_micros = micros();
    int state = digitalRead(pin);

    if (state)
    {
      lastRaising = current_micros;
    }
    else
    {
      durationUp = current_micros - lastRaising;
    }
  }
};

const int channelCount = 9;

// this has cyclic depedendency on callback functions, so will be initialized after them
extern InterruptPWMTracker channels[channelCount];


void step(int steps, Direction direction, int speed) {
  // *** MODIFIED PART ***
  // Set full power to the stepper coils using PCA9685
  // 4095 is 100% duty cycle for the 12-bit PCA9685
  // pwm.setPWM(STEPPER_PWM_CH, 0, 4095);

  int currentStep = 0;
  for (int i = 0; i < steps; i++) {
    if (direction == FORWARD) {
      currentStep++;
    } else {
      currentStep--;
    }
    currentStep = currentStep % 4;
    if (currentStep < 0) currentStep = 3;

    // This is a 4-step full-step sequence (this logic does not change)
    switch (currentStep) {
      case 0: shiftRegister_STEP.set(2, HIGH); shiftRegister_STEP.set(3, LOW); shiftRegister_STEP.set(1, HIGH); shiftRegister_STEP.set(4, LOW); break;
      case 1: shiftRegister_STEP.set(2, LOW); shiftRegister_STEP.set(3, HIGH); shiftRegister_STEP.set(1, HIGH); shiftRegister_STEP.set(4, LOW); break;
      case 2: shiftRegister_STEP.set(2, LOW); shiftRegister_STEP.set(3, HIGH); shiftRegister_STEP.set(1, LOW); shiftRegister_STEP.set(4, HIGH); break;
      case 3: shiftRegister_STEP.set(2, HIGH); shiftRegister_STEP.set(3, LOW); shiftRegister_STEP.set(1, LOW); shiftRegister_STEP.set(4, HIGH); break;
    }
    delayMicroseconds(speed);
  }
}

void setDCMotorDirection(int motorNum, Direction direction)
{
  u_int8_t pinA, pinB;
  switch (motorNum)
  {
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
    break;
  }

  switch (direction)
  {
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
}

void sendDCMotorDirection()
{
  shiftRegister_DC.updateRegisters();
}

void setDCMotorSpeed(int motorNum, int speed)
{
  // DANG REVERSE CHO NAY
  if (speed < 0)
  {
    setDCMotorDirection(motorNum, REVERSE);
    pwm.setPWM(motorNum - 1, 0, abs(speed));
  }
  else if (speed > 0)
  {
    setDCMotorDirection(motorNum, FORWARD);
    pwm.setPWM(motorNum - 1, 0, speed);
  }
  else
  {
    setDCMotorDirection(motorNum, STOP);
    pwm.setPWM(motorNum - 1, 0, 0);
  }
  sendDCMotorDirection();
}

// void printChannelToSerial(int number) {
//   Serial.print("CH");
//   Serial.print(number + 1);
//   Serial.print(" ");
//   if (channels[number].durationUp < 1000) {
//     Serial.print(" ");
//   }
//   Serial.print(channels[number].durationUp);
// }

void setup()
{

  Serial.begin(115200);
  pinMode(2, OUTPUT); // LED

  Serial.println("TD BOT v1.0");
  Serial.println("Nghiên cứu và phát triển bởi CLB Lập trình & Robotics THPT Tràng Định");

  pwm.begin();
  pwm.setOscillatorFrequency(26500000);
  pwm.setPWMFreq(50); // Analog servos run at ~50 Hz updates

  setDCMotorDirection(1, STOP);
  setDCMotorDirection(2, STOP);
  setDCMotorDirection(3, STOP);
  setDCMotorDirection(4, STOP);
  sendDCMotorDirection();

  pwm.setPWM(15, 0, 150);
  pwm.setPWM(14, 0, 600);

  digitalWrite(2, HIGH); // LED ON
  for (int i = 0; i < channelCount; i++)
  {
    channels[i].setup();
  }

  int speedX = 0;
  int speedY = 0;
}

void setServoAngle(uint8_t channel, int angle)
{
  int pulseLen = map(angle, 0, 180, 102, 512);
  pwm.setPWM(channel, 0, pulseLen);
}

void loop() {
  // pwm.setPWM(8, 0, map(channels[8].duratinUp, 1000, 2000, 102, 512)); // gripper servo
  // setServoAngle(9, map(channels[8].durationUp, 1000, 2000, 0, 180));
  // pwm.setPWM(10, 0, map(channels[8].durationUp, 1000, 2000, 102, 512)); // gripper servo
  // step(100, FORWARD, 2000); // Faster this time
  int speedY = channels[1].durationUp - 1500;
  int speedX = channels[3].durationUp - 1500;
  int rotation = channels[0].durationUp - 1500;

  if (channels[4].durationUp > 2000) {
    step(100, FORWARD, 2000);
  } else if (channels[4].durationUp < 1000) {
    step(100, REVERSE, 2000);
  }

  // tay gap len xuong
  if (channels[7].durationUp > 1900) {
    setServoAngle(8, 180); // lift servo up
  } else if (channels[7].durationUp < 1600 && channels[7].durationUp > 1400) {
    setServoAngle(8, 140); // lift servo up
  } else if (channels[7].durationUp < 1100) {
     setServoAngle(8, 110); // lift servo up
  }

  
  if (channels[5].durationUp < 1500) {
    // pwm.setPWM(12, 0, 500); // open gripper
    setServoAngle(12, 90);
  } else if (channels[5].durationUp > 1500) {
    setServoAngle(12, 160);
  }

  // tay gap dong mo
  if (channels[6].durationUp < 1500) {
    setServoAngle(9, 10);
  } else if (channels[6].durationUp > 1500) {
    setServoAngle(9, 65);
  }

  if (speedX > 490)
    speedX = 500;
  else if (speedX < -490)
    speedX = -500;
  else if (speedX <= 20 && speedX >= -20)
    speedX = 0;

  if (speedY > 490)
    speedY = 500;
  else if (speedY < -490)
    speedY = -500;
  else if (speedY <= 20 && speedY >= -20)
    speedY = 0;

  if (rotation > 490)
    rotation = 500;
  else if (rotation < -490)
    rotation = -500;
  else if (rotation <= 20 && rotation >= -20)
    rotation = 0;

  if (speedX == 0 && speedY == 0 && rotation == 0)
  {
    setDCMotorDirection(1, STOP);
    setDCMotorDirection(2, STOP);
    setDCMotorDirection(3, STOP);
    setDCMotorDirection(4, STOP);
    sendDCMotorDirection();
  }
  else
  {
    int FL_power = speedY + speedX + rotation;
    int FR_power = speedY - speedX - rotation;
    int RL_power = speedY - speedX + rotation;
    int RR_power = speedY + speedX - rotation;

    int maxPower = max(max(abs(FL_power), abs(FR_power)), max(abs(RL_power), abs(RR_power)));
    if (maxPower > 500)
    {
      FL_power = FL_power * 500 / maxPower;
      FR_power = FR_power * 500 / maxPower;
      RL_power = RL_power * 500 / maxPower;
      RR_power = RR_power * 500 / maxPower;
    }

    setDCMotorSpeed(1, map(FR_power, -500, 500, -4095, 4095));
    setDCMotorSpeed(2, map(FL_power, -500, 500, -4095, 4095));
    setDCMotorSpeed(3, map(RL_power, -500, 500, -4095, 4095));
    setDCMotorSpeed(4, map(RR_power, -500, 500, -4095, 4095));
  }
}

void channel1onChangeInterrupt()
{
  channels[0].updateFromPinChangeInterrupt();
}

void channel2onChangeInterrupt()
{
  channels[1].updateFromPinChangeInterrupt();
}

void channel3onChangeInterrupt()
{
  channels[2].updateFromPinChangeInterrupt();
}

void channel4onChangeInterrupt()
{
  channels[3].updateFromPinChangeInterrupt();
}

void channel5onChangeInterrupt()
{
  channels[4].updateFromPinChangeInterrupt();
}

void channel6onChangeInterrupt()
{
  channels[5].updateFromPinChangeInterrupt();
}

void channel7onChangeInterrupt()
{
  channels[6].updateFromPinChangeInterrupt();
}

void channel8onChangeInterrupt()
{
  channels[7].updateFromPinChangeInterrupt();
}

void channel9onChangeInterrupt()
{
  channels[8].updateFromPinChangeInterrupt();
}

void channel10onChangeInterrupt()
{
  channels[9].updateFromPinChangeInterrupt();
}

InterruptPWMTracker channels[channelCount] = {
    InterruptPWMTracker(13, channel1onChangeInterrupt),
    InterruptPWMTracker(12, channel2onChangeInterrupt),
    InterruptPWMTracker(14, channel3onChangeInterrupt),
    InterruptPWMTracker(27, channel4onChangeInterrupt),
    InterruptPWMTracker(26, channel5onChangeInterrupt),
    InterruptPWMTracker(25, channel6onChangeInterrupt),
    InterruptPWMTracker(33, channel7onChangeInterrupt),
    InterruptPWMTracker(32, channel8onChangeInterrupt),
    InterruptPWMTracker(35, channel9onChangeInterrupt),
    // InterruptPWMTracker(34, channel10onChangeInterrupt),
};