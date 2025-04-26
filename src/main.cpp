// #include <Arduino.h>
// #include <Arduino_FreeRTOS.h>  // Include FreeRTOS for Arduino

// const int trigPin = A0; // TRIG pin (using analog pin A0)
// const int echoPin = A1; // ECHO pin (using analog pin A1)

// // A simple helper function as in the original code
// int myFunction(int x, int y) {
//   return x + y;
// }

// // Task to handle the ultrasonic sensor reading and LED control
// void SensorTask(void *pvParameters) {
//   (void) pvParameters;
//   // Demonstrate the use of myFunction (result unused)
//   int result = myFunction(2, 3);
  
//   for (;;) {
//     // Trigger the ultrasonic sensor:
//     digitalWrite(trigPin, LOW);
//     delayMicroseconds(2);
//     digitalWrite(trigPin, HIGH);
//     delayMicroseconds(10);
//     digitalWrite(trigPin, LOW);
    
//     // Measure the echo pulse duration:
//     long duration = pulseIn(echoPin, HIGH);
    
//     // Calculate the distance (in cm):
//     long distance = (duration * 0.034) / 2;
    
//     // Turn the LED on if an object is closer than 10cm:
//     if (distance < 10) {
//       digitalWrite(LED_BUILTIN, HIGH);
//     } else {
//       digitalWrite(LED_BUILTIN, LOW);
//     }
    
//     // Wait 500ms before the next measurement using FreeRTOS delay:
//     vTaskDelay(500 / portTICK_PERIOD_MS);
//   }
// }

// void setup() {
//   // Initialize the pins:
//   pinMode(trigPin, OUTPUT);
//   pinMode(echoPin, INPUT);
//   pinMode(LED_BUILTIN, OUTPUT);

//   // Create the sensor task:
//   xTaskCreate(
//     SensorTask,      // Task function
//     "SensorTask",    // Name of the task
//     128,             // Stack size (in words)
//     NULL,            // Task parameter (none in this case)
//     1,               // Priority
//     NULL             // Task handle
//   );

//   // Start the scheduler so that tasks begin executing:
//   vTaskStartScheduler();
// }

// void loop() {
//   // The loop remains empty 1as FreeRTOS tasks manage the program execution.
// }


#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal.h>

// HC-SR04 Pins
const int trigPin = A0;
const int echoPin = A1;


// LCD Pins (connected in 4-bit mode)
const int rs = 0, en = 1, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Shared distance variable
volatile long distance = 0;

// Task handles (optional)
TaskHandle_t TaskMeasure, TaskDisplay, TaskWrite;


// Task 1: Measure Distance
void measureDistanceTask(void *pvParameters) {
  while (1) {

    if (distance < 10) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

// Task 2: Display Distance
void displayDistanceTask(void *pvParameters) {
  char buf[16];
  while (1) {
    lcd.setCursor(0, 1); // Second line
    snprintf(buf, sizeof(buf), "Dist: %3ld cm", distance);
    lcd.print(buf);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// Task 3: Write Distance
void writeDistanceTask(void *pvParameters) {
  while (1) {
    digitalWrite(trigPin, LOW);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    digitalWrite(trigPin, HIGH);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    distance = (duration * 0.034) / 2 + 1; // sai so

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Init pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Init LCD
  lcd.begin(16, 2);
  lcd.print("Distance:");

  // Create FreeRTOS tasks
  xTaskCreate(measureDistanceTask, "Measure", 128, NULL, 2, &TaskMeasure);
  xTaskCreate(displayDistanceTask, "Display", 128, NULL, 1, &TaskDisplay);
  xTaskCreate(writeDistanceTask, "Write", 128, NULL, 1, &TaskWrite);

  vTaskStartScheduler();
}

void loop() {
  // Not used in FreeRTOS
}


