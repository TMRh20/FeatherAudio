/** 
* Wireless Audio Transceiver
*
* This code allows users to create a wireless radio or intercomm using the custom PCB
* found at https://github.com/TMRh20/FeatherAudio
* 
* by TMRh20 2026
*/



#include <AutoAnalogAudio.h>
#include <nrf_to_nrf.h>

AutoAnalog aaAudio;
nrf_to_nrf radio;


#if defined(USE_TINYUSB)
// Needed for Serial.print on non-MBED enabled or adafruit-based nRF52 cores
#include "Adafruit_TinyUSB.h"
#endif

/**************** USER VARIABLES *************************/

#define AUDIO_DEBUG 1                // Set to 1 to print out values from the microphone
#define RADIO_CHANNEL 50             // Set the channel for the radio
volatile float volumeControl = 3.5;  // Default Volume level
uint32_t sampleRate = 32000;         // Either 16000 or 32000 supported

/*********************************************************/

#define BUFFER_SIZE 124
uint8_t address[6] = { "1Node" };
volatile bool micActive = false;
volatile bool rxToggled = false;
volatile bool txToggled = false;

/*****************************************/

void volUp() {
  volumeControl += 0.3;         // Raise Volume
  if (digitalRead(9) == LOW) {  // If both pins are pressed, volDown pressed first
    micActive = true;           // Enable the microphone until PTT is pressed
    setupInput();               // Setup audio for the microphone
  }
}
/*****************************************/
void volDown() {
  volumeControl -= 0.3;       // Lower volume
  if (volumeControl < 0.1) {  // If volume setting too low set to 0.0
    volumeControl = 0.0;
  }
}

/*****************************************/

void ptt() {

  if (digitalRead(6) == LOW) {  // If PTT button pressed
    micActive = true;           // Enable I2S microphone
    rxToggled = true;           // Only enable microphone once via this variable

  } else {
    micActive = false;  // Enable I2s amplifier output
    txToggled = true;   // Only enable output once via this variable
  }
}

/*****************************************/

void setupOutput() {
  aaAudio.manualI2S = false;             // Use internal I2S timers for I2S
  aaAudio.begin(0, 2);                   // Setup aaAudio using I2S amplifier output
  aaAudio.dacBitsPerSample = 16;         // 16-bit samples output to I2S
  aaAudio.setSampleRate(sampleRate, 0);  // 32 or 16kHz
  radio.startListening();                // Start the radio in listen mode
}

/*****************************************/

void setupInput() {
  aaAudio.manualI2S = true;              // Use PWM to drive the I2S signal
  aaAudio.begin(2, 0);                   // Enable I2S microphone input
  aaAudio.adcBitsPerSample = 24;         // Needs to be 24 bits for the onboard microphone
  aaAudio.setSampleRate(sampleRate, 0);  // 32 or 16kHz
  radio.stopListening();                 // Start the radio in tx mode
}

/*****************************************/

void setup() {

  Serial.begin(115200);
  radio.begin();  // General radio config
  radio.setChannel(RADIO_CHANNEL);
  radio.setPayloadSize(BUFFER_SIZE);
  radio.setAutoAck(0);
  radio.setDataRate(NRF_1MBPS);
  radio.openWritingPipe(address);
  radio.openReadingPipe(1, address);
  radio.startListening();

  pinMode(LED_BUILTIN, OUTPUT);  // LED toggles to red when data is received, off if no data
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(6, INPUT_PULLUP);  // Enable PTT pin
  attachInterrupt(digitalPinToInterrupt(6), ptt, CHANGE);
  pinMode(9, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(9), volDown, FALLING);  // Enable Volume pins
  attachInterrupt(digitalPinToInterrupt(5), volUp, FALLING);

  aaAudio.I2S_PIN_LRCK = 28;                // WS Pin: On the Feather 52840 Express or Sense, this is pin A3
  aaAudio.maxBufferSize = BUFFER_SIZE * 2;  // Set maxBufferSize to limit memory usage
  setupOutput();                            // Setup I2S audio output
}

/*****************************************/

int32_t audioBuffer[BUFFER_SIZE / 2];
uint32_t lastRadio = millis();

/*****************************************/

void loop() {

  if (micActive) {    // If the mic has been toggled on by the PTT or combination of LowVolume then HighVolume buttons
    if (rxToggled) {  // Only do this once per toggle
      setupInput();   // Configure I2S audio input
      rxToggled = false;
    }
    aaAudio.getADC(BUFFER_SIZE / 2);  // Get data from I2S mic, will get 18-bit samples in a 32-bit format, but output 16-bit samples over the radio

#if AUDIO_DEBUG > 0  // If debug is enabled, print a sample from each batch of samples
    int32_t sample = 0;
    memcpy(&sample, &aaAudio.adcBuffer16[0], 4);
    Serial.print((int16_t)sample >> 2);
    Serial.print(",");
    Serial.println(0);
#endif

    uint16_t counter2 = 0;
    for (int i = 0; i < BUFFER_SIZE / 2; i++) {
      memcpy(&audioBuffer[i], &aaAudio.adcBuffer16[counter2], 4);  // Loop to copy data from 16-bit incoming buffer to a 32-bit integer buffer
      audioBuffer[i] >>= 2;                                        // Change from 18-bit to 16-bit
      memcpy(&aaAudio.dacBuffer16[i], &audioBuffer[i], 2);         // Copy to 16-bit output buffer from 32-bit format
      counter2 += 2;
    }
    int16_t test = 0;
    memcpy(&test,&aaAudio.dacBuffer16[0],2);
    Serial.print(test);
    Serial.print(",");
    Serial.println(0);

    radio.startWrite(&aaAudio.dacBuffer16[0], BUFFER_SIZE, 1);  // Send 62 16-bit samples over the radio (124 bytes)

  } else {

    if (txToggled) {      // If I2S output has been toggled
      setupOutput();      // Setup I2S amplifier output
      txToggled = false;  // Only needs to be done once
    }
    if (radio.available()) {  // Check for incoming radio data

      radio.read(&aaAudio.dacBuffer16[0], BUFFER_SIZE);  // Read incoming radio data, 124 bytes or 62 16-bit samples

      for (int i = 0; i < BUFFER_SIZE / 2; i++) {  // Loop for the number of actual samples
        int16_t test = aaAudio.dacBuffer16[i];     // Adjust volume for incoming data
        test *= volumeControl;
        aaAudio.dacBuffer16[i] = test;
      }
#if AUDIO_DEBUG > 0
      int16_t sampleOut = 0;
      memcpy(&sampleOut,&aaAudio.dacBuffer16[0], 2);
      Serial.print(sampleOut);  // Print one sample from each batch if data received & debug enabled
      Serial.print(",");
      Serial.println(0);
#endif      
      aaAudio.feedDAC(0, BUFFER_SIZE / 2);  // Feed the I2S amplifier with the actual number of samples
      lastRadio = millis();                 // Timer to silence the radio & toggle the LED off/on
      digitalWrite(LED_BUILTIN, HIGH);      // Toggle red LED on
    } else {
      if (millis() - lastRadio > 100) {               // If no data for 100ms
        memset(aaAudio.dacBuffer16, 0, BUFFER_SIZE);  // Set our output buffer to all 0s
        aaAudio.feedDAC(0, BUFFER_SIZE / 2);          // Feed it into the I2S peripheral
        digitalWrite(LED_BUILTIN, LOW);               // Turn off the red LED
      }
    }
  }
}

/*****************************************/
