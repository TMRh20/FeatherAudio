
/*
* Requires https://github.com/TMRh20/microDecoder library
*/

#include <SD.h>
#include <AutoAnalogAudio.h>
#include "mp3.h"  // decoder
#include "pcm.h"

mp3 MP3;
pcm audio;
AutoAnalog aaAudio;


/************ USER CONFIGURATION ***************/
const char* audioFilename = "noceil.mp3";
uint8_t SD_CS_PIN = 2;                      // Set this to your CS pin for the SD card/module

float volumeControl = 0.2;
#define AUDIO_BUFFER_SIZE 1600  // Should be a multiple of 64
/**********************************************/

void setup() {

  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.print("Init SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("SD init ok");

  aaAudio.I2S_PIN_LRCK = 28;  // Use different LRCK pin for Feather Express 52840
  aaAudio.maxBufferSize = AUDIO_BUFFER_SIZE;
  aaAudio.begin(0,2);

  playAudio(audioFilename);
}

void loop() {
  
  loadBuffer();

  // Control via Serial for testing
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '=') {
      volumeControl += 0.1;
    } else if (c == '-') {
      volumeControl -= 0.1;
      volumeControl = max(0.0, volumeControl);
    } else if (c == 'p') {
      playAudio("atliens.mp3");
    } else if( c == 'c') {
      playAudio("dopeBoys.mp3");
    }
    Serial.println(volumeControl);
  }

}


File myFile;

void playAudio(const char* audioFile) {

  if (myFile) {
    MP3.end();
    myFile.close();    
  }

  memset(aaAudio.dacBuffer16,0,sizeof(aaAudio.dacBuffer16));
  aaAudio.feedDAC(0, 64);

  //Open the designated file
  myFile = SD.open(audioFile);

  MP3.begin(myFile);
  MP3.getMetadata();
  aaAudio.setSampleRate(MP3.Fs, 1);
  Serial.println(MP3.Fs);
  aaAudio.dacBitsPerSample = MP3.bitsPerSample;
  Serial.println(MP3.bitsPerSample);
}

void loadBuffer() {

  uint32_t sampleCounter = 0;
  if (myFile.available() >= AUDIO_BUFFER_SIZE) {
    for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE/64; i++) {
      audio = MP3.decode();
      memcpy(&aaAudio.dacBuffer16[sampleCounter], audio.interleaved, 128);  // 128 bytes
      sampleCounter += 64;                                                  // 64 samples per go
    }
    for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; i++) {
      int16_t sample = aaAudio.dacBuffer16[i];
      sample *= volumeControl;
      aaAudio.dacBuffer16[i] = sample;
    }
    aaAudio.feedDAC(0, sampleCounter);
  } else {
    myFile.seek(0);
  }
}