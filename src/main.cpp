#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// --- Pin Definitions ---
#define TRIGGER_PIN A0
#define ECHO_PIN    A1

// --- Constants ---
#define DISTANCE_THRESHOLD_CM 20

// LCD Pins (connected in 4-bit mode)
const int rs = 0, en = 1, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// --- Shared Variables ---
volatile uint16_t current_distance_cm = 0;
SemaphoreHandle_t distanceMutex;

volatile bool ledState = false;  // false = OFF, true = ON
SemaphoreHandle_t ledMutex;

TaskHandle_t AlertTaskHandle = NULL; // LED task handle for notifications

// --- Function Declarations ---
void SensorTask(void *pvParameters);
void DisplayTask(void *pvParameters);
void AlertTask(void *pvParameters);

// --- Utility Functions ---
void triggerUltrasonic() {
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
}

uint16_t measureDistanceCM() {
  triggerUltrasonic();
  unsigned long duration = pulseIn(ECHO_PIN, HIGH);

  uint16_t distance_cm = duration / 58;
  return distance_cm;
}

void setup() {
  // Serial.begin(9600);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Starting...");

  // Create Mutexes
  distanceMutex = xSemaphoreCreateMutex();
  ledMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreate(SensorTask, "Sensor", 128, NULL, 1, NULL);
  xTaskCreate(AlertTask, "Alert", 128, NULL, 2, &AlertTaskHandle);
  xTaskCreate(DisplayTask, "Display", 128, NULL, 1, NULL);

  // Start FreeRTOS scheduler
  vTaskStartScheduler();
}

void loop() {
}

// --- Tasks Implementation ---
void SensorTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms loop

  for (;;) {
    uint16_t distance = measureDistanceCM();

    if (xSemaphoreTake(distanceMutex, portMAX_DELAY)) {
      current_distance_cm = distance;
      xSemaphoreGive(distanceMutex);
    }

    bool desiredLedState = (distance <= DISTANCE_THRESHOLD_CM);

    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
      if (desiredLedState != ledState) {
        ledState = desiredLedState;
        xTaskNotifyGive(AlertTaskHandle); // Notify LED task only if needed
      }
      xSemaphoreGive(ledMutex);
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void AlertTask(void *pvParameters) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification

    bool localLedState;

    if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
      localLedState = ledState;
      xSemaphoreGive(ledMutex);
    }

    digitalWrite(LED_BUILTIN, localLedState ? HIGH : LOW);
  }
}

void DisplayTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(200); // 200ms update

  for (;;) {
    uint16_t distance_copy;

    if (xSemaphoreTake(distanceMutex, portMAX_DELAY)) {
      distance_copy = current_distance_cm;
      xSemaphoreGive(distanceMutex);
    }

    // lcd.clear();
    lcd.setCursor(0, 0);
    char buf[16];
    snprintf(buf, sizeof(buf), "Dist: %3d cm", distance_copy);
    lcd.print(buf);

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
