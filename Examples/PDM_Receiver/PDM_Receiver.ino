/* Example of receiving audio over radio and playing it back. 
 * Note: This example uses PWM output. Modify the USE_I2S #define
 * below to enable I2S output.
 */ 

#include <AutoAnalogAudio.h>
#include <nrf_to_nrf.h>

AutoAnalog aaAudio;
nrf_to_nrf radio;

uint8_t address[][6] = { "1Node", "2Node" };
#define BUFFER_SIZE 124
#define USE_I2S 0 // Change to 1 to enable I2S output instead of PWM

void setup() {

  Serial.begin(115200);

  radio.begin();
  radio.setChannel(50);
  radio.setPayloadSize(BUFFER_SIZE);
  radio.setAutoAck(0);
  radio.setDataRate(NRF_2MBPS);
  radio.openReadingPipe(1,address[1]);
  radio.startListening();

  Serial.println("Analog Audio Begin");
  aaAudio.I2S_PIN_SCK = 3; //BCLK
  aaAudio.I2S_PIN_LRCK = 28; //WS
  aaAudio.begin(0, 2);  //Setup aaAudio using DAC and PWM. Change the third value to a 1 to enable I2S

  //Setup for audio: Use 8-bit, mono WAV files @ 16kHz
  aaAudio.dacBitsPerSample = 16;    // 8-bit
  aaAudio.setSampleRate(32000, 0); // 16khz, mono


}

void loop() {
  
  if(radio.available()){ //Serial.println("x");
    radio.read(&aaAudio.dacBuffer16[0],BUFFER_SIZE);
    aaAudio.feedDAC(0,BUFFER_SIZE/2);
  }

}
