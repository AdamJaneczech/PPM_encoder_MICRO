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

void parseInput(String input) {
  // Check if input starts with 'C' and parse the channel and value
  if (input.startsWith("C") && input.length() > 4) {
    int channel = input.substring(1, 2).toInt() - 1;  // Get channel number (0-indexed)
    int value = input.substring(3).toInt();            // Get pulse width value

    // Validate channel and value
    if (channel >= 0 && channel < NUM_CHANNELS && value >= MIN_PPM_MICROS && value <= MAX_PPM_MICROS) {
      channelValues[channel] = value;  // Update channel value
      Serial.print("Channel ");
      Serial.print(channel + 1);
      Serial.print(" set to ");
      Serial.println(value);
    } else {
      Serial.println("Invalid input. Use format 'Cx YYYY' (x: channel 1-6, YYYY: 1000-2000).");
    }
  } else {
    Serial.println("Invalid input format. Use format 'Cx YYYY'.");
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

  // Start serial communication
  Serial.begin(115200);
  Serial.println("PPM Encoder ready. Use format 'Cx YYYY' to change channel x to value YYYY.");

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC ,SSD1306_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();
}

void loop() {
  // Check for serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    parseInput(input);
  }

  // Update PPM signal with current channel values
  for (int i = 0; i < NUM_CHANNELS; i++) {
    ppmEncoder.setChannel(i, channelValues[i]);
  }

  // Update the display
  updateDisplay();

  // Short delay to ensure stable signal update
  delay(20);
}