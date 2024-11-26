#include <PPMEncoder.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define NUM_CHANNELS 6      // Define the number of channels
#define PPM_PIN 9           // Pin to output the PPM signal
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin (not used with I2C)
#define SSD1306_I2C_ADDRESS 0x3C // I2C display address

#define MAX_PPM_MICROS 2000
#define MIN_PPM_MICROS 1000

// Initialize the display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Array to store the channel values
uint16_t channelValues[NUM_CHANNELS] = {1500, 1500, 1500, 1500, 1500, 1500};
uint16_t cyclicPeriod[NUM_CHANNELS] = {0, 0, 0, 0, 0, 0};  // Store the period for cyclic channels
bool isCyclic[NUM_CHANNELS] = {false, false, false, false, false, false};  // Track if a channel is in cyclic mode
uint32_t cyclicStartTime[NUM_CHANNELS] = {0, 0, 0, 0, 0, 0};  // Track start time for each cyclic channel

void parseInput(String input) {
  // Reset all channels to 1500 microseconds if 'R' command is received
  Serial.println(input);
  if (input.startsWith("R") && input.length() < 3) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
      channelValues[i] = 1500;
      isCyclic[i] = false; // Disable cyclic mode for all channels
    }
    Serial.println("All channels reset to 1500 microseconds.");
  }
  // Parse channel setting command 'Cx YYYY'
  else if (input.startsWith("C") && input.length() > 4) {
    int channel = input.substring(1, 2).toInt() - 1;
    int value = input.substring(3).toInt();

    // Validate and set channel value
    if (channel >= 0 && channel < NUM_CHANNELS && value >= MIN_PPM_MICROS && value <= MAX_PPM_MICROS) {
      channelValues[channel] = value;
      isCyclic[channel] = false; // Disable cyclic mode if manually setting
      Serial.print("Channel ");
      Serial.print(channel + 1);
      Serial.print(" set to ");
      Serial.println(value);
    } else {
      Serial.println("Invalid input. Use format 'Cx YYYY' (x: channel 1-6, YYYY: 1000-2000).");
    }
  } 
  // Parse cyclic motion command 'Ax YYYY'
  else if (input.startsWith("A") && input.length() > 4) {
    int channel = input.substring(1, 2).toInt() - 1;
    int period = input.substring(3).toInt();

    // Validate and set cyclic motion
    if (channel >= 0 && channel < NUM_CHANNELS && period > 0) {
      cyclicPeriod[channel] = period;
      isCyclic[channel] = true; // Enable cyclic mode for the channel
      cyclicStartTime[channel] = millis(); // Set the start time for the cycle
      Serial.print("Cyclic motion enabled on channel ");
      Serial.print(channel + 1);
      Serial.print(" with period ");
      Serial.print(period);
      Serial.println(" ms");
    } else {
      Serial.println("Invalid input. Use format 'Ax YYYY' (x: channel 1-6, YYYY: positive integer).");
    }
  } else {
    Serial.println("Invalid input format. Use format 'Cx YYYY', 'Ax YYYY', or 'R'.");
  }
}

void updateDisplay() {
  display.clearDisplay();

  for (int i = 0; i < NUM_CHANNELS; i++) {
    // Draw channel name
    display.setCursor(0, i * 10);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("C");
    display.print(i + 1);

    // Calculate fill based on channel value (1000 to 2000)
    int fillWidth = map(channelValues[i], MIN_PPM_MICROS, MAX_PPM_MICROS, 0, 100);

    // Draw border
    display.drawRect(20, i * 10, 100, 8, SSD1306_WHITE);
    // Draw fill
    display.fillRect(20, i * 10, fillWidth, 8, SSD1306_WHITE);
  }

  // Display everything
  display.display();
}

void setup() {
  // Start the PPM encoder
  ppmEncoder.begin(PPM_PIN, NUM_CHANNELS);
  pinMode(12,OUTPUT);
  digitalWrite(12,HIGH);

  // Start serial communication
  Serial.begin(115200);
  Serial.println("PPM Encoder ready. Use format 'Cx YYYY' to change channel x to value YYYY, 'Ax YYYY' for cyclic motion, or 'R' to reset.");

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();
}

// Function to handle cyclic motion for channels in cyclic mode
void handleCyclicMotion() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (isCyclic[i]) {
      uint32_t elapsedTime = (millis() - cyclicStartTime[i]) % cyclicPeriod[i];

      // Calculate the phase of the cyclic motion
      if (elapsedTime < cyclicPeriod[i] / 4) {
        channelValues[i] = map(elapsedTime, 0, cyclicPeriod[i] / 4, 1500, 2000); // Ramp up
      } else if (elapsedTime < cyclicPeriod[i] / 2) {
        channelValues[i] = map(elapsedTime, cyclicPeriod[i] / 4, cyclicPeriod[i] / 2, 2000, 1500); // Ramp down
      } else if (elapsedTime < 3 * cyclicPeriod[i] / 4) {
        channelValues[i] = map(elapsedTime, cyclicPeriod[i] / 2, 3 * cyclicPeriod[i] / 4, 1500, 1000); // Ramp down further
      } else {
        channelValues[i] = map(elapsedTime, 3 * cyclicPeriod[i] / 4, cyclicPeriod[i], 1000, 1500); // Ramp back up
      }
    }
  }
}

void loop() {
  // Check for serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    parseInput(input);
  }

  // Update PPM signal with current channel values
  handleCyclicMotion();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    ppmEncoder.setChannel(i, channelValues[i]);
  }

  // Update the display
  updateDisplay();

  // Short delay to ensure stable signal update
  //delay(20);
}