// cho người sau:
// code được tối ưu khá tốt dù chỉ chạy trên 1 nhân CPU
// chạy 2 CPU thì hơi dư thừa

// import thư viện

// shift register để điều khiển hướng động cơ DC
#include <ShiftRegister74HC595.h>
// thư viện của arduino do dùng arduino framework
#include <Arduino.h>
// điều khiển mạch pca9585 để xuất xung pwm cho động cơ dc và servo
#include <SPI.h>
#include <Adafruit_PWMServoDriver.h>

// --- CONFIGURATION & PIN DEFINITIONS ---

// chân tín hiệu tốc độ động cơ trên pca9685
#define CH_MOTOR_1 0
#define CH_MOTOR_2 1
#define CH_MOTOR_3 2
#define CH_MOTOR_4 3

// chân servo trên pca9685
#define CH_SERVO_LIFT 8
#define CH_SERVO_GRIPPER_ROT 9
#define CH_SERVO_GRIPPER_L 12
#define CH_SERVO_GRIPPER_R 13
#define CH_SERVO_AUX_1 15
#define CH_SERVO_AUX_2 14

// chân điều khiển hướng trên mạch điều khiển động cơ
#define SR_DC_DATA  15
#define SR_DC_CLOCK 4
#define SR_DC_LATCH 16

// chân tín hiệu từ tay điều khiển
const int rc_pins[] = {13, 12, 14, 27, 26, 25, 33, 32, 35, 34};
const int channelCount = 10;

// --- OBJECT INSTANTIATION ---

// cài đặt ban đầu
ShiftRegister74HC595<1> shiftRegister_DC(SR_DC_DATA, SR_DC_CLOCK, SR_DC_LATCH);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// biến hướng
enum Direction { FORWARD, REVERSE, STOP };

// --- CLASSES ---
// không sửa phần này
// đọc tín hiệu từ tay điều khiển
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

  //  is crucial for ESP32 interrupts to prevent crashes/latency
  void  updateFromPinChangeInterrupt() {
    unsigned long current_micros = micros();
    if (digitalRead(pin)) {
      lastRaising = current_micros;
    } else {
      durationUp = current_micros - lastRaising;
    }
  }
};

// --- FORWARD DECLARATIONS OF ISRs ---
void  channel1onChangeInterrupt();
void  channel2onChangeInterrupt();
void  channel3onChangeInterrupt();
void  channel4onChangeInterrupt();
void  channel5onChangeInterrupt();
void  channel6onChangeInterrupt();
void  channel7onChangeInterrupt();
void  channel8onChangeInterrupt();
void  channel9onChangeInterrupt();
void  channel10onChangeInterrupt();

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
    InterruptPWMTracker(rc_pins[9], channel10onChangeInterrupt),
};

// --- ISR IMPLEMENTATIONS ---
void  channel1onChangeInterrupt() { channels[0].updateFromPinChangeInterrupt(); }
void  channel2onChangeInterrupt() { channels[1].updateFromPinChangeInterrupt(); }
void  channel3onChangeInterrupt() { channels[2].updateFromPinChangeInterrupt(); }
void  channel4onChangeInterrupt() { channels[3].updateFromPinChangeInterrupt(); }
void  channel5onChangeInterrupt() { channels[4].updateFromPinChangeInterrupt(); }
void  channel6onChangeInterrupt() { channels[5].updateFromPinChangeInterrupt(); }
void  channel7onChangeInterrupt() { channels[6].updateFromPinChangeInterrupt(); }
void  channel8onChangeInterrupt() { channels[7].updateFromPinChangeInterrupt(); }
void  channel9onChangeInterrupt() { channels[8].updateFromPinChangeInterrupt(); }
void  channel10onChangeInterrupt() { channels[9].updateFromPinChangeInterrupt(); }
// hết phần không sửa, đọc tín hiệu từ tay điều khiển

// hàm điều khiển trạng thái động cơ DC
void setDCMotorState(int motorNum, Direction direction, int pwmValue) {
  uint8_t pinA, pinB;
  
  // không sửa phần này, chân điều khiển hướng động cơ
  switch (motorNum) {
    case 1: pinA = 2; pinB = 3; break;
    case 2: pinA = 1; pinB = 4; break;
    case 3: pinA = 0; pinB = 6; break;
    case 4: pinA = 5; pinB = 7; break;
    default: return;
  }

  // cài đặt hướng cho động cơ
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

  // cài tốc độ
  pwm.setPWM(motorNum - 1, 0, pwmValue);
}

// hàm điều khiển góc servo (0-180),
// dùng servo 360 thì có thể dùng hàm này
// servo 270 sẽ không lấy được full góc bằng hàm này
void setServoAngle(uint8_t channel, int angle) {
  // Constrain angle to safe limits
  // angle = constrain(angle, 0, 180);
  // int pulseLen = map(angle, 0, 180, 102, 512);
  pwm.setPWM(channel, 0, map(angle, 0, 180, 102, 512));
}

// hàm điều khiển chuyển động robot
void handleDrive(int speedX, int speedY, int rotation) {
  // loại bỏ sai số từ tay điều khiển
  if (abs(speedX) < 10) speedX = 0;
  if (abs(speedY) < 10) speedY = 0;
  if (abs(rotation) < 10) rotation = 0;

  // nếu không có gì thì dừng động cơ
  if (speedX == 0 && speedY == 0 && rotation == 0) {
    setDCMotorState(1, STOP, 0);
    setDCMotorState(2, STOP, 0);
    setDCMotorState(3, STOP, 0);
    setDCMotorState(4, STOP, 0);
    shiftRegister_DC.updateRegisters();
    return;
  }

  // phương trình điều khiển bánh xe mecanum
  int FL = speedY + speedX + rotation; // bánh trái trước
  int FR = speedY - speedX - rotation; // bánh phải trước
  int RL = speedY - speedX + rotation; // bánh trái sau
  int RR = speedY + speedX - rotation; // bánh phải sau

  // giới hạn công suất tối đa động cơ
  // bánh nào quá 500 thì kéo cả 4 xuống theo tỉ lệ
  int maxPower = max(max(abs(FL), abs(FR)), max(abs(RL), abs(RR)));
  if (maxPower > 500) {
    FL = map(FL, -maxPower, maxPower, -500, 500);
    FR = map(FR, -maxPower, maxPower, -500, 500);
    RL = map(RL, -maxPower, maxPower, -500, 500);
    RR = map(RR, -maxPower, maxPower, -500, 500);
  }

  // DEBUG OUTPUT
  // Serial.print("Speed X: ");
  // Serial.print(speedX);
  // Serial.print(" | Speed Y: ");
  // Serial.print(speedY);
  // Serial.print(" | FR: ");
  // Serial.print(FR);
  // Serial.print(" | FL: ");
  // Serial.print(FL);
  // Serial.print(" | RL: ");
  // Serial.print(RL);
  // Serial.print(" | RR: ");
  // Serial.println(RR);

  // xác định hướng tiến hay lùi (tốc độ > 0 thì tiến, < 0 thì lùi, = 0 thì dừng)
  auto getDir = [](int p) { return (p > 0) ? FORWARD : ((p < 0) ? REVERSE : STOP); };
  
  // gửi tín hiệu đến động cơ
  setDCMotorState(1, getDir(FR), map(abs(FR), 0, 500, 0, 4095));
  setDCMotorState(2, getDir(FL), map(abs(FL), 0, 500, 0, 4095));
  setDCMotorState(3, getDir(RL), map(abs(RL), 0, 500, 0, 4095));
  setDCMotorState(4, getDir(RR), map(abs(RR), 0, 500, 0, 4095));
  
  // gửi tín hiệu đến shift register
  shiftRegister_DC.updateRegisters();
}

// hàm các cần gạt điều khiển
void handleManipulators() {
  // TAY GẮP LÊN XUỐNG

  // Tay gắp cao nhất
  if (channels[7].durationUp > 1900) {
    if (channels[6].durationUp < 1500) {
      // tay gắp lên cao nhất có thể
      setServoAngle(8, 135); 
    } else {
      // tay gắp khi gạt nước
      setServoAngle(8, 90);
    }
  } else if (channels[7].durationUp < 1600 && channels[7].durationUp > 1400) {
    // tay gắp ở giữa (lấy hạt trên cao)
    setServoAngle(8, 70); 
  } else if (channels[7].durationUp < 1100) {
    // tay gắp hạ thấp (lấy hạt dưới thấp)
    setServoAngle(8, 0); 
  }

  // thả hạt 2 bên
  if (channels[4].durationUp > 1900) {
    // mở bên trái
    setServoAngle(12, 45);
  } else if (channels[4].durationUp > 1400 && channels[4].durationUp < 1600) {
    // đóng 2 bên
    setServoAngle(12, 120);
    setServoAngle(CH_SERVO_GRIPPER_R, 60);
  } else if (channels[4].durationUp < 1100) {
    // mở bên phải
    setServoAngle(CH_SERVO_GRIPPER_R, 155);
  }

  // TAY GẮP VÀO
  if (channels[5].durationUp < 1500) {
    // nhả ra
    setServoAngle(CH_SERVO_GRIPPER_ROT, 10);
  } else if (channels[5].durationUp >= 1500) {
    // găp vào
    // độ sâu gắp vào thay đổi theo núm vặn
    int angle = map(channels[8].durationUp, 1000, 2000, 40, 80);
    setServoAngle(CH_SERVO_GRIPPER_ROT, angle);
  }
}

// --- MAIN SETUP & LOOP ---

void setup() {
  // Serial.begin(115200); // Debugging if needed

  
  pinMode(2, OUTPUT); // PIN 2 -> đèn led trên ESP32
  
  // cài đặt pca9685
  pwm.begin();
  pwm.setOscillatorFrequency(26150000);   // chỉnh tần số xung nhịp từ 25MHz (mặc định) đến 27MHz khi nào dùng đồng hồ đo điện ra được 50Hz thì (chấp nhận được 49.9->50.1Hz)
  pwm.setPWMFreq(50); 

  // dừng tất cả động cơ
  handleDrive(0, 0, 0);

  // cài 2 chân cuối trên pca9685 để cắm test
  pwm.setPWM(CH_SERVO_AUX_1, 0, 102);
  pwm.setPWM(CH_SERVO_AUX_2, 0, 512);


  // cài đặt chân nhận tín hiệu từ tay điều khiển
  for (int i = 0; i < channelCount; i++) {
    channels[i].setup();
  }

  digitalWrite(2, HIGH); // bật đèn led trên ESP32 để báo đã khởi động xong
}

void loop() {
  // tính tín hiệu từ tay điều khiển trong khoảng 1000-2000, 1500 là ở giữa
  int speedY = channels[1].durationUp - 1500;
  int rotation = channels[3].durationUp - 1500;
  int speedX = channels[0].durationUp - 1500;

  // gọi hàm điều khiển
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
  // Serial.print(channels[8].durationUp);
  // Serial.print(" | CH10: ");
  // Serial.println(channels[9].durationUp);

  // gọi hàm xử lí các cần gạt
  handleManipulators();
  
}