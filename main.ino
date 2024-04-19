#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint8_t ROOT_MENU_CNT = 4;  // Increased for the new menu
const uint8_t SUB_MENU1_CNT = 4;
const uint8_t SUB_MENU2_CNT = 5;
const uint8_t SUB_MENU3_CNT = 2;

const int BTN_ACCEPT = A0;
const int BTN_UP = A2;
const int BTN_DOWN = A1;
const int BTN_CANCEL = A3;
const int BTN_RESET = A6;  // Replace with the actual pin number for the reset button
const int ENA_PIN = 12;    // Replace with the actual pin number for ENA
const int ENB_PIN = 13;    // Replace with the actual pin number for ENB

Stepper stepper(200, 8, 9, 10, 11);  // Adjust the steps per revolution and pins

enum PageType {
  ROOT_MENU,
  SUB_MENU1,
  SUB_MENU2,
  SUB_MENU3,
  STEPPER_CONTROL  // Added a new enum for stepper control
};

PageType currentPage = ROOT_MENU;
uint8_t rootMenuPosition = 1;
uint8_t subMenu1Position = 1;
uint8_t subMenu2Position = 1;
uint8_t subMenu3Position = 1;
int stepperPosition = 0;  // Variable to track stepper position
int stepperSpeed = 200;   // Set the speed of the stepper motor
int menuOffset = 0;       // Offset for scrolling menus
unsigned long previousActionMillis = 0;
unsigned long previousButtonMillis = 0;
const unsigned long interval = 250;  // Delay interval in milliseconds
const unsigned long buttonDebounceInterval = 500;  // Debounce interval for buttons

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();

  pinMode(BTN_ACCEPT, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_CANCEL, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  // Set up your stepper motor here
  stepper.setSpeed(stepperSpeed);  // Adjust the speed as needed

  // Disable both motors initially
  digitalWrite(ENA_PIN, LOW);
  digitalWrite(ENB_PIN, LOW);
}

void loop() {
  unsigned long currentMillis = millis();

  switch (currentPage) {
    case ROOT_MENU: handleRootMenu(currentMillis); break;
    case SUB_MENU1: handleSubMenu(SUB_MENU1_CNT, SUB_MENU1, &subMenu1Position, currentMillis); break;
    case SUB_MENU2: handleSubMenu(SUB_MENU2_CNT, SUB_MENU2, &subMenu2Position, currentMillis); break;
    case SUB_MENU3: handleSubMenu(SUB_MENU3_CNT, SUB_MENU3, &subMenu3Position, currentMillis); break;
    case STEPPER_CONTROL: handleStepperControl(currentMillis); break;
  }
  // Check if the reset button is pressed
  if (btnIsDown(BTN_RESET, currentMillis)) {
    resetStepper();
  }
}

void drawMenuTitle(const char* title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  char titleBuffer[40];
  sprintf(titleBuffer, "[ %s ]", title);
  display.println(titleBuffer);
}

void handleRootMenu(unsigned long currentMillis) {
  // Define custom menu items
  const char* rootMenuItems[] = {
    "Sub Menu One",
    "Sub Menu Two",
    "Sub Menu Three",
    "Stepper Control"  // Added a new menu item
  };

  int visibleItems = min(3, ROOT_MENU_CNT);  // Number of visible items on the screen
  int startIndex = menuOffset;
  int endIndex = min(startIndex + visibleItems, ROOT_MENU_CNT);

  drawMenuTitle("MAIN MENU");

  // Draw menu items
  display.setCursor(0, 20);
  for (uint8_t i = startIndex; i < endIndex; i++) {
    display.print(" ");
    if (i == rootMenuPosition - 1) {
      display.print("[ ");
    } else {
      display.print(" ");
    }
    display.print(i + 1);
    if (i == rootMenuPosition - 1) {
      display.print("]");
    } else {
      display.print(" ");
    }
    display.print(". ");
    display.println(rootMenuItems[i]);
  }
  display.display();

  // Handle button presses with debounce
  if (btnIsDown(BTN_UP, currentMillis) && rootMenuPosition > 1) {
    rootMenuPosition--;
    if (rootMenuPosition < startIndex + 1) {
      menuOffset--;
    }
  }
  if (btnIsDown(BTN_DOWN, currentMillis) && rootMenuPosition < ROOT_MENU_CNT) {
    rootMenuPosition++;
    if (rootMenuPosition > endIndex) {
      menuOffset++;
    }
  }
  if (btnIsDown(BTN_ACCEPT, currentMillis)) {
    switch (rootMenuPosition) {
      case 1: currentPage = SUB_MENU1; break;
      case 2: currentPage = SUB_MENU2; break;
      case 3: currentPage = SUB_MENU3; break;
      case 4:
        currentPage = STEPPER_CONTROL;
        menuOffset = 0;
        break;  // Reset menuOffset
    }
  }
}

void handleSubMenu(uint8_t itemCount, PageType nextPage, uint8_t* position, unsigned long currentMillis) {
  // Define custom submenu items
  const char* submenuItems[] = {
    "First Item",
    "Second Item",
    "Third Item",
    "Fourth Item",
    "Fifth Item"
  };

  int visibleItems = min(3, itemCount);  // Number of visible items on the screen
  int startIndex = menuOffset;
  int endIndex = min(startIndex + visibleItems, itemCount);

  drawMenuTitle(("SUB MENU #" + String(nextPage - SUB_MENU1 + 1)).c_str());

  // Draw menu items
  display.setCursor(0, 20);
  for (uint8_t i = startIndex; i < endIndex; i++) {
    display.print(" ");
    if (i == *position - 1) {
      display.print("[ ");
    } else {
      display.print(" ");
    }
    display.print(i + 1);
    if (i == *position - 1) {
      display.print(" ]");
    } else {
      display.print(" ");
    }
    display.print(". ");
    display.println(submenuItems[i]);
  }
  display.display();

  // Handle button presses with debounce
  if (btnIsDown(BTN_UP, currentMillis) && *position > 1) {
    (*position)--;
    if (*position < startIndex + 1) {
      menuOffset--;
    }
  }
  if (btnIsDown(BTN_DOWN, currentMillis) && *position < itemCount) {
    (*position)++;
    if (*position > endIndex) {
      menuOffset++;
    }
  }
  if (btnIsDown(BTN_CANCEL, currentMillis)) {
    currentPage = ROOT_MENU;
  }
}

void handleStepperControl(unsigned long currentMillis) {
  // Enable both motors by setting ENA and ENB pins to HIGH
  digitalWrite(ENA_PIN, HIGH);
  digitalWrite(ENB_PIN, HIGH);

  drawMenuTitle("STEPPER CONTROL");

  // Display the current degree of rotation in the center of the menu
  display.setTextSize(3);
  display.setCursor(40, 30);
  display.print(stepperPosition);  // Display the degree

  display.display();

  // Handle button presses with debounce
  if (btnIsDown(BTN_UP, currentMillis) && currentMillis - previousActionMillis >= interval) {
    // Rotate the stepper motor counter-clockwise (decrease degree)
    stepper.step(200);
    stepperPosition--;
    previousActionMillis = currentMillis;
  }
  if (btnIsDown(BTN_DOWN, currentMillis) && currentMillis - previousActionMillis >= interval) {
    // Rotate the stepper motor clockwise (increase degree)
    stepper.step(-200);
    stepperPosition++;
    previousActionMillis = currentMillis;
  }
  if (btnIsDown(BTN_CANCEL, currentMillis)) {
    // Disable both motors when returning to the root menu
    digitalWrite(ENA_PIN, LOW);
    digitalWrite(ENB_PIN, LOW);
    currentPage = ROOT_MENU;  // Return to the root menu
  }
  if (btnIsDown(BTN_RESET, currentMillis)) {
    // Reset the stepper to home
    resetStepper();
  }
}

void resetStepper() {
  // If the stepperPosition is positive, rotate counterclockwise to 0
  if (stepperPosition > 0) {
    while (stepperPosition > 0) {
      stepper.step(200);
      stepperPosition--;
      delay(250);  // Adjust delay as needed for a smooth reset
    }
  }
  // If the stepperPosition is negative, rotate clockwise to 0
  else if (stepperPosition < 0) {
    while (stepperPosition < 0) {
      stepper.step(-200);
      stepperPosition++;
      delay(250);  // Adjust delay as needed for a smooth reset
    }
  }
}

boolean btnIsDown(int btn, unsigned long currentMillis) {
  if (digitalRead(btn) == LOW && currentMillis - previousButtonMillis >= buttonDebounceInterval) {
    previousButtonMillis = currentMillis;
    return true;
  }
  return false;
}
