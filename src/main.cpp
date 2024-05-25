#include <Arduino.h>
#include <SIM800L.h>
// Define a generic type for serial objects
typedef HardwareSerial *SerialObject;

void  send_at_command(const char *command);
void  send_post_request_at();
char *read_serial_command(SerialObject serial);

// Define a buffer to store the received serial data
char serial_buffer[128];
int  serial_buffer_index = 0;

const char APN[] = "pinternet.interkom.de";
const char URL[] = "http://postman-echo.com/get?foo1=bar1&foo2=bar2";

// Define the pins used for hardware serial communication with the SIM800L
// module
// c3
#define RX_PIN 2
#define TX_PIN 1
#define SIM800_RST_PIN 3
// s3
// #define RX_PIN 4
// #define TX_PIN 5

// Create a HardwareSerial object
HardwareSerial serial1(1);

SIM800L *sim800l;

// Define the button pin and debounce variables
const int     buttonPin = 0;
unsigned long lastDebounceTime = 0;
const int     debounceDelay = 50; // Debounce delay in milliseconds

void setupModule();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);
  sim800l = new SIM800L((Stream *)&serial1, SIM800_RST_PIN, 200, 512,
                        (Stream *)&Serial);
  Serial.println("Hello");

  setupModule();
  // Set button pin as input with internal pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);
}

void make_request() {
  Serial.println("POST");
  // do sth
  Serial.println("DONE");
}

void loop() {
  // Establish GPRS connectivity (5 trials)
  bool connected = false;
  for (uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    connected = sim800l->connectGPRS();
  }

  // Check if connected, if not reset the module and setup the config again
  if (connected) {
    Serial.print(F("GPRS connected with IP "));
    Serial.println(sim800l->getIP());
  } else {
    Serial.println(F("GPRS not connected !"));
    Serial.println(F("Reset the module."));
    sim800l->reset();
    setupModule();
    return;
  }

  Serial.println(F("Start HTTP GET..."));

  // Do HTTP GET communication with 10s for the timeout (read)
  uint16_t rc = sim800l->doGet(URL, 10000);
  if (rc == 200) {
    // Success, output the data received on the serial
    Serial.print(F("HTTP GET successful ("));
    Serial.print(sim800l->getDataSizeReceived());
    Serial.println(F(" bytes)"));
    Serial.print(F("Received : "));
    Serial.println(sim800l->getDataReceived());
  } else {
    // Failed...
    Serial.print(F("HTTP GET error "));
    Serial.println(rc);
  }

  // Close GPRS connectivity (5 trials)
  bool disconnected = sim800l->disconnectGPRS();
  for (uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    disconnected = sim800l->disconnectGPRS();
  }

  if (disconnected) {
    Serial.println(F("GPRS disconnected !"));
  } else {
    Serial.println(F("GPRS still connected !"));
  }

  // Go into low power mode
  bool lowPowerMode = sim800l->setPowerMode(MINIMUM);
  if (lowPowerMode) {
    Serial.println(F("Module in low power mode"));
  } else {
    Serial.println(F("Failed to switch module to low power mode"));
  }

  // End of program... wait...
  while (1)
    ;
}

void setupModule() {
  // Wait until the module is ready to accept AT commands
  while (!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }
  Serial.println(F("Setup Complete!"));

  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while (signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
  }
  Serial.print(F("Signal OK (strenght: "));
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while (network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
  }
  Serial.println(F("Network registration OK"));
  delay(1000);

  // Setup APN for GPRS configuration
  bool success = sim800l->setupGPRS(APN);
  while (!success) {
    success = sim800l->setupGPRS(APN);
    delay(5000);
  }
  Serial.println(F("GPRS config OK"));
}