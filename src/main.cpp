#include <Arduino.h>

// Define a generic type for serial objects
typedef HardwareSerial *SerialObject;

void  send_at_command(const char *command);
void  send_post_request_at();
char *read_serial_command(SerialObject serial);

// Define a buffer to store the received serial data
char serial_buffer[128];
int  serial_buffer_index = 0;

// Define the pins used for hardware serial communication with the SIM800L
// module
#define RX_PIN 4
#define TX_PIN 5

// Create a HardwareSerial object
HardwareSerial sim800l(1);

// Define the button pin and debounce variables
const int     buttonPin = 0;
unsigned long lastDebounceTime = 0;
const int     debounceDelay = 50; // Debounce delay in milliseconds

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  sim800l.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(2000);
  Serial.println("Hello");

  // Send AT commands to initialize the SIM800L module
  send_at_command("AT");
  send_at_command("AT+CGATT=1");
  send_at_command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  send_at_command("AT+SAPBR=3,1,\"APN\",\"pinternet.interkom.de\"");
  delay(2000);
  send_at_command("AT+SAPBR=1,1");
  send_at_command("AT+SAPBR=2,1"); // Test IP
  send_at_command("AT+CSQ");       // Test Signal quality
  // +CSQ: 17 <Strength: -113 ~ 31dBm>, 0 <BitError 0 ~ 255>

  // send_at_command("AT+COPS=?"); // list all available networks

  // Set button pin as input with internal pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);
}

void make_request() {
  Serial.println("POST");
  // Send the POST request
  send_post_request_at();
  delay(6000);
  send_at_command("AT+HTTPREAD"); // Read data
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
  // send_at_command("AT+HTTPTERM"); //Terminate http connection
  // send_at_command("AT+SAPBR=0,1"); //Terminate gprs connection
  Serial.println("DONE");
}

void loop() {
  // Read the button state
  int buttonState = digitalRead(buttonPin);

  // Debounce the button press
  if (buttonState == LOW && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();

    // Make the request
    make_request();
  }
  // Read the received command from serial
  char *command = read_serial_command((SerialObject)&Serial);

  // If a command is available, send it as an AT command
  if (command != NULL) {
    send_at_command(command);
  }

  // Read the received command from sim800l
  command = read_serial_command(&sim800l);

  // If a command is available, process it
  if (command != NULL) {
    // Process the received command from sim800l
    Serial.println(command);
  }
}

void send_at_command(const char *command) {
  sim800l.println(command);
  delay(2000);
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
}

void send_post_request_at() {
  // Build the HTTP POST request string
  delay(2000);

  send_at_command("AT+HTTPINIT"); // Start http

  // AT+HTTPPARA="CID",1
  send_at_command("AT+HTTPPARA=\"CID\",1"); // Set connection type

  // send_at_command("AT+HTTPSSL=1"); // Uncomment this if your URL is https

  // AT+HTTPPARA="URL","http://sandro.awardspace.info/php/hola.php?hello_world"
  send_at_command("AT+HTTPPARA=\"URL\",\"https://catfact.ninja/fact\"");
  // send_at_command("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  // send_at_command("AT+HTTPDATA=1500,5000\r\n");
  // send_at_command("{ \"body\": { \"result\": \"true\" } }");
  delay(2000);
  send_at_command("AT+HTTPACTION=0");
}

// Function to read serial data and return the received command
char *read_serial_command(SerialObject serial) {
  // Check for incoming serial data
  if (serial->available()) {
    char c = serial->read();

    // Store the received character in the buffer
    serial_buffer[serial_buffer_index++] = c;

    // Check if the end of the command is reached
    if (c == '\n' || c == '\r') {
      // Terminate the string
      serial_buffer[serial_buffer_index] = '\0';

      // Reset the buffer index
      serial_buffer_index = 0;

      // Return the received command
      return serial_buffer;
    }
  }

  // Return NULL if no command is available
  return NULL;
}