#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>

// Pin Definitions
#define SCL        26
#define SDA        27

#define force_1    12   // ADC2_CH5
#define force_2    13   // ADC2_CH4

#define mtrpwm     21
#define in1        22
#define in2        23
#define mtrstndby  25

// OLED Config
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT    64
#define OLED_RESET       -1
#define OLED_ADDR      0x3C

// Motor Config
#define MOTOR_SLOW_PWM    75   // 0–255; ~30% speed
#define MOTOR_DELAY_MS  3000   // 3-second startup delay

// Globals
Adafruit_MPU6050  mpu;
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool motorRunning = false;
unsigned long bootTime = 0;

// Motor Helpers
void motorStop() {
  analogWrite(mtrpwm, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(mtrstndby, LOW);   // standby = disabled
}

void motorForward(uint8_t speed) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(mtrstndby, HIGH);  // standby = enabled
  analogWrite(mtrpwm, speed);
}

// Setup
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);

  // MPU6050 
  if (!mpu.begin()) {
    Serial.println("[ERR] MPU6050 not found – check wiring!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("[OK] MPU6050");

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[ERR] SSD1306 not found – check address/wiring!");
    while (1) delay(10);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  Serial.println("[OK] SSD1306");

  // Motor pins
  pinMode(mtrpwm,    OUTPUT);
  pinMode(in1,       OUTPUT);
  pinMode(in2,       OUTPUT);
  pinMode(mtrstndby, OUTPUT);
  motorStop();

  bootTime = millis();
  Serial.println("Setup complete. Motor starts in 3 s...");
}

void loop() {

  // Force Sensors
  int f1 = analogRead(force_1);   // 0–4095 on ESP32 (12-bit ADC)
  int f2 = analogRead(force_2);

  // MPU6050: Net Upward Acceleration
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  // Z-axis includes gravity (~9.81 m/s²); subtract it for net upward accel
  float upAccel = a.acceleration.z - 9.81f;

  // Motor: engage after 3-second delay, run slowly
  if (!motorRunning && (millis() - bootTime >= MOTOR_DELAY_MS)) {
    motorRunning = true;
    motorForward(MOTOR_SLOW_PWM);
    Serial.println("Motor ON (slow)");
  }

  // OLED (128×64)
  display.clearDisplay();
  display.setTextSize(1);          // 6×8 px per character

  // Force sensor readings
  display.setCursor(0, 0);
  display.print("Force 1: ");
  display.println(f1);

  display.setCursor(0, 12);
  display.print("Force 2: ");
  display.println(f2);

  // Divider
  display.drawLine(0, 23, 127, 23, SSD1306_WHITE);

  // Upward acceleration
  display.setCursor(0, 27);
  display.print("Up Accel:");
  display.setCursor(0, 38);
  display.print(upAccel, 2);
  display.print(" m/s\xB2");       // m/s²

  // Divider
  display.drawLine(0, 50, 127, 50, SSD1306_WHITE);

  // Motor status
  display.setCursor(0, 54);
  display.print("Motor: ");
  display.print(motorRunning ? "ON  (slow)" : "OFF");

  display.display();

  // Serial Monitor
  Serial.printf("F1: %4d | F2: %4d | UpAccel: %6.2f m/s2 | Motor: %s\n",
                f1, f2, upAccel, motorRunning ? "ON" : "OFF");

  delay(50);
}