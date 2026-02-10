#include <AutoAnalogAudio.h>
#include <nrf_to_nrf.h>

AutoAnalog aaAudio;
nrf_to_nrf radio;

uint8_t address[][6] = { "1Node", "2Node" };
#define BUFFER_SIZE 124

#if defined(USE_TINYUSB)
    // Needed for Serial.print on non-MBED enabled or adafruit-based nRF52 cores
    #include "Adafruit_TinyUSB.h"
#endif

/*********************************************************/
/* Tested with MAX98357A I2S breakout on XIAO 52840 Sense
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/

void setup() {


  Serial.begin(115200);
  radio.begin();
  radio.setChannel(50);
  radio.setPayloadSize(BUFFER_SIZE);
  radio.setAutoAck(0);
  radio.setDataRate(NRF_2MBPS);
  radio.openWritingPipe(address[1]);
  radio.stopListening();

  aaAudio.I2S_PIN_LRCK = 28; // WS
  
  aaAudio.begin(1,0);  //Setup aaAudio using DAC & I2S
  aaAudio.adcBitsPerSample = 16; // 16-bit samples input from PDM
  aaAudio.dacBitsPerSample = 16; // 16-bit samples output to I2S
  aaAudio.setSampleRate(32000);  // 32kHz


}

void loop() {
  aaAudio.getADC(BUFFER_SIZE/2); // Get data from PDM microphone
  radio.startWrite(&aaAudio.adcBuffer16[0], BUFFER_SIZE, 1);
}
