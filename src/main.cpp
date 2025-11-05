// import thư viện
#include <ShiftRegister74HC595.h>
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PWMServoDriver.h>
#include <driver/pcnt.h>

ShiftRegister74HC595<1> shiftRegister_DC(15, 4, 16); // (data pin, clock pin, latch pin)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

enum Direction {
  FORWARD,
  REVERSE,
  STOP
};

// const int PCNT_CHANNEL_COUNT = 8; 

// // The GPIO pins to read the PWM signals from
// const int PINS[PCNT_CHANNEL_COUNT] = {13, 12, 14, 27, 26, 25, 33, 32}; 

// // Array to hold the final pulse width measurement in clock ticks
// volatile int16_t pulse_width_ticks[PCNT_CHANNEL_COUNT] = {0};

// // A generic ISR that can be attached to any of the pins
// void IRAM_ATTR on_pwm_falling_edge(void* arg) {
//     int channel_index = (int)arg;
//     pcnt_unit_t unit = (pcnt_unit_t)channel_index;

//     // Read the counter value and then clear it for the next pulse
//     pcnt_get_counter_value(unit, &pulse_width_ticks[channel_index]);
//     pcnt_counter_clear(unit);
// }

// void setup_pcnt_channel(int index) {
//     pcnt_unit_t unit = (pcnt_unit_t)index;
//     int pin = PINS[index];

//     pcnt_config_t pcnt_config = {
//         .pulse_gpio_num = -1,           // We are not counting pulses on a pin
//         .ctrl_gpio_num = pin,           // This is our GATE pin
//         .lctrl_mode = PCNT_MODE_DISABLE,// When the gate is LOW, stop counting
//         .hctrl_mode = PCNT_MODE_KEEP,   // When the gate is HIGH, continue counting
//         .pos_mode = PCNT_COUNT_INC,     // We want to count up
//         .neg_mode = PCNT_COUNT_DIS,     // We don't want to count down
//         .counter_h_lim = 32767,         // High limit
//         .counter_l_lim = 0,             // Low limit
//         .unit = unit,
//         .channel = PCNT_CHANNEL_0,      // Use channel 0 of the unit
//     };
//     pcnt_unit_config(&pcnt_config);

//     // Configure the filter to ignore short glitches (optional but good practice)
//     pcnt_set_filter_value(unit, 100);
//     pcnt_filter_enable(unit);

//     // Install the GPIO interrupt service and attach our ISR
//     gpio_install_isr_service(0);
//     gpio_set_intr_type((gpio_num_t)pin, GPIO_INTR_NEGEDGE); // Trigger on FALLING edge
//     gpio_isr_handler_add((gpio_num_t)pin, on_pwm_falling_edge, (void*)index);

//     // Start the counter
//     pcnt_counter_resume(unit);
//     pcnt_counter_clear(unit);
// }

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
}

void sendDCMotorDirection() {
  shiftRegister_DC.updateRegisters();
}

void setDCMotorSpeed(int motorNum, int speed) {
  // DANG REVERSE CHO NAY
  if (speed < 0) {
    setDCMotorDirection(motorNum, FORWARD);
    pwm.setPWM(motorNum - 1, 0, abs(speed));
  } else if (speed > 0) {
    setDCMotorDirection(motorNum, REVERSE);
    pwm.setPWM(motorNum - 1, 0, speed);
  } else {
    setDCMotorDirection(motorNum, STOP);
    pwm.setPWM(motorNum - 1, 0, 0);
  }
  sendDCMotorDirection();
}

volatile unsigned long lastRaising;
volatile unsigned long durationUp;
volatile unsigned long durationCycle;
volatile unsigned long lastRaising2;
volatile unsigned long durationUp2;
volatile unsigned long durationCycle2;
volatile unsigned long lastRaising4;
volatile unsigned long durationUp4;
volatile unsigned long durationCycle4;


void onChangeInterrupt() {
  unsigned long current_micros = micros();
  int state = digitalRead(35);

  if (state) {
    durationCycle = current_micros - lastRaising;
    lastRaising = current_micros;
  } else {
    durationUp = current_micros - lastRaising;
  }
}

void onChangeInterrupt2() {
  unsigned long current_micros2 = micros();
  int state = digitalRead(32);

  if (state) {
    durationCycle2 = current_micros2 - lastRaising2;
    lastRaising2 = current_micros2;
  } else {
    durationUp2 = current_micros2 - lastRaising2;
  }
}

void onChangeInterrupt4() {
  unsigned long current_micros4 = micros();
  int state = digitalRead(25);

  if (state) {
    durationCycle4 = current_micros4- lastRaising4;
    lastRaising4 = current_micros4;
  } else {
    durationUp4 = current_micros4 - lastRaising4;
  }
}

void setup() {
  pinMode(35, INPUT);
  attachInterrupt(digitalPinToInterrupt(35), onChangeInterrupt, CHANGE);
  pinMode(32, INPUT);
  attachInterrupt(digitalPinToInterrupt(32), onChangeInterrupt2, CHANGE);
  pinMode(25, INPUT);
  attachInterrupt(digitalPinToInterrupt(25), onChangeInterrupt4, CHANGE);

  Serial.begin(115200);
  pinMode(2, OUTPUT); // LED

  Serial.println("TD BOT v1.0");
  Serial.println("Nghiên cứu và phát triển bởi CLB Lập trình & Robotics THPT Tràng Định");

  pwm.begin();
  pwm.setOscillatorFrequency(26500000);
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz updates

  setDCMotorDirection(1, STOP);
  setDCMotorDirection(2, STOP);
  setDCMotorDirection(3, STOP);
  setDCMotorDirection(4, STOP);
  sendDCMotorDirection();

  // for (int i = 0; i < PCNT_CHANNEL_COUNT; i++) {
  //   setup_pcnt_channel(i);
  //   Serial.printf("PCNT Unit %d initialized on GPIO %d\n", i, PINS[i]);
  // }

  int speedX = 0;
  int speedY = 0;
}

void setServoAngle(uint8_t channel, int angle) {
  int pulseLen = map(angle, 0, 180, 100, 500);
  pwm.setPWM(channel, 0, pulseLen);
}

void loop() {
  int speedY = durationUp2 - 1500;
  int speedX = durationUp - 1500;
  int rotation = durationUp4 - 1500;

  if (speedX > 490) speedX = 500;
  else if (speedX < -490) speedX = -500;
  else if (speedX <= 20 && speedX >= -20) speedX = 0;

  if (speedY > 490) speedY = 500;
  else if (speedY < -490) speedY = -500;
  else if (speedY <= 20 && speedY >= -20) speedY = 0;

  if (rotation > 490) rotation = 500;
  else if (rotation < -490) rotation = -500;
  else if (rotation <= 20 && rotation >= -20) rotation = 0;

  // Serial.print("Speed X: ");
  // Serial.print(speedX);
  // Serial.print(" | ");
  // Serial.print("Rotation: ");
  // Serial.println(rotation);

  double theta = atan2(speedY, speedX);

  if (speedX == 0 && speedY == 0 && rotation == 0) {
    setDCMotorDirection(1, STOP);
    setDCMotorDirection(2, STOP);
    setDCMotorDirection(3, STOP);
    setDCMotorDirection(4, STOP);
    sendDCMotorDirection();
  } else if (speedY != 0) {
    setDCMotorSpeed(1, map(speedY, -500, 500, -4095, 4095));
    setDCMotorSpeed(2, map(speedY, -500, 500, -4095, 4095));
    setDCMotorSpeed(3, map(speedY, -500, 500, -4095, 4095));
    setDCMotorSpeed(4, map(speedY, -500, 500, -4095, 4095));
  } else if (speedX != 0) {
    setDCMotorSpeed(1, map(speedX, -500, 500, 4095, -4095));
    setDCMotorSpeed(2, map(speedX, -500, 500, -4095, 4095));
    setDCMotorSpeed(3, map(speedX, -500, 500, 4095, -4095));
    setDCMotorSpeed(4, map(speedX, -500, 500, -4095, 4095));
  } else if (rotation != 0) {
    setDCMotorSpeed(1, map(rotation, -500, 500, 4095, -4095));
    setDCMotorSpeed(2, map(rotation, -500, 500, -4095, 4095));
    setDCMotorSpeed(3, map(rotation, -500, 500, -4095, 4095));
    setDCMotorSpeed(4, map(rotation, -500, 500, 4095, -4095));
  }

  
}