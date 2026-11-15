#include <Arduino.h>
#include <M5Unified.h> // library for screen and buttons

// grove port pins used for UART communication with STM32
#define RX_PIN 32
#define TX_PIN 33

// motor simulation parameters
float speed = 0.0f; // current simulated speed
float tau = 0.5f;   // time constant of the motor (seconds)
float K = 1.0f;     // motor gain

unsigned long last_time = 0;
unsigned long last_display = 0;

// state variables
float current_command = 0.0f; // command received from STM32
bool is_running = true;       // flag to start/stop the motor (Btn A)
float disturbance = 0.0f;     // simulated mechanical load (Btn B)

void setup()
{
  // initialize M5 hardware (screen, buttons, power)
  auto cfg = M5.config();
  M5.begin(cfg);

  // configure display
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Motor Sim");

  // initialize USB Serial for debugging
  Serial.begin(115200);

  // initialize Serial2 for UART communication with STM32
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // store initial time for dt calculation
  last_time = millis();
}

void loop()
{
  // update M5 state (must be called in every loop to read buttons)
  M5.update();

  // --- button handling ---
  // toggle motor state (RUN/STOP)
  if (M5.BtnA.wasPressed())
  {
    is_running = !is_running;
  }
  // apply a sudden mechanical disturbance (+30)
  if (M5.BtnB.wasPressed())
  {
    disturbance += 30.0f;
  }

  // --- uart communication ---
  // if a command arrives from STM32, parse it
  if (Serial2.available())
  {
    current_command = Serial2.parseFloat();
  }

  // --- physics model ---
  // calculate time delta (dt) in seconds
  unsigned long now = millis();
  float dt = (now - last_time) / 1000.0f;
  last_time = now;

  // determine target speed (0 if stopped by button)
  float target_speed = is_running ? (K * current_command) : 0.0f;

  // apply disturbance (simulating friction/load)
  target_speed -= disturbance;
  if (target_speed < 0)
  {
    target_speed = 0; // speed cannot be negative
  }

  // disturbance fades over time (simulating release of the brake)
  if (disturbance > 0)
  {
    disturbance -= (dt * 20.0f);
    if (disturbance < 0)
    {
      disturbance = 0;
    }
  }

  // first-order differential equation (motor inertia)
  speed += (target_speed - speed) * (dt / tau);

  // send the computed speed back to STM32
  Serial2.println(speed, 2);

  // --- display update ---
  // refresh screen only every 100ms to prevent flickering
  if (now - last_display > 100)
  {
    last_display = now;

    M5.Display.setCursor(0, 30);
    M5.Display.printf("Cmd: %5.1f\n", current_command);           // command from STM32
    M5.Display.printf("Vel: %5.1f\n", speed);                     // simulated speed
    M5.Display.printf("Sta: %s\n", is_running ? "RUN " : "STOP"); // status
    M5.Display.printf("Dis: %5.1f\n", disturbance);               // disturbance value
  }
}