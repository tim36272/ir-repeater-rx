#include <Manchester.h>
#include <libHomeMesh.h>
#include <IRremote.h>

#define TX_PIN 2  //pin where your RF transmitter is connected
#define RX_PIN 0 //pin where your RF receiver is located
#define IR_RX_PIN 3 //pin where your IR receiver is connected
#define IR_TX_PIN 4 //pin where your IR LED is connected (via transistor)
#define IR_SIGNAL_TIMEOUT 5000 //maximum length, in microseconds, of a gap in an IR signal. Used to trigger IR sending
#define IR_FREQ 38 // IR Frequency in kilohertz
void setup() {
  man.setupTransmit(TX_PIN, MAN_1200);
  pinMode(IR_RX_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  //Serial.begin(9600);
  //Serial.println("Initialization complete");
  for(uint8_t i=0;i<5;i++) {
    delay(200);
    digitalWrite(13,HIGH);
    delay(200);
    digitalWrite(13, LOW);
  }
}

typedef enum {
  IR_STATE_IDLE,
  IR_STATE_RECORDING_LOW, // a recording is in progress and the last detected state was low
  IR_STATE_RECORDING_HIGH, //similarly, with last state high
} irState_t;

IRsend irsend;

irState_t state = IR_STATE_IDLE;
#define MAX_CODE_LENGTH 140
unsigned int recording[MAX_CODE_LENGTH];
uint8_t rfBuffer[MAX_CODE_LENGTH*2+2];
/*
unsigned int testPattern[47] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11,12,13,14,15,16,17,18,19,20,
  21,22,23,24,25,26,27,28,29,30,
  31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47
};
*/
uint8_t recordingIndex = 0;
unsigned long lastEventTimestamp = 0;
void loop() {
  //The IR receiver is normally HIGH, and goes to LOW when the remote is illuminated.
  //We call this a "mark". Thus, we wait at idle while the receiver is HIGH, then when 
  //it goes LOW (a "mark") we record pulses ("marks" and "spaces") until it goes LOW 
  //for a long time (IR_SIGNAL_TIMEOUT microseconds), then we consider the transmission complete
  if(recordingIndex > MAX_CODE_LENGTH) {
    //Serial.print("Overflow detected: ");
    //Serial.println(recordingIndex,DEC);
  }
  if(state == IR_STATE_RECORDING_LOW) {
    //This is the case where the IR remote is illuminated (i.e. a "mark") so look for a space
    if(digitalRead(IR_RX_PIN) == HIGH) {
      unsigned long thisTimestamp;
      thisTimestamp = micros();
      recording[recordingIndex++] = (thisTimestamp - lastEventTimestamp);
      lastEventTimestamp = thisTimestamp;
      state = IR_STATE_RECORDING_HIGH;
    } 
  } else if (state == IR_STATE_RECORDING_HIGH) {
      //This is the case where the IR remote is not illuminated (i.e. a "space") so look for a "mark" or a timeout
      unsigned long thisTimestamp;
      thisTimestamp = micros();
    if(digitalRead(IR_RX_PIN) == LOW) {
      recording[recordingIndex++] = (thisTimestamp - lastEventTimestamp);
      lastEventTimestamp = thisTimestamp;
      state = IR_STATE_RECORDING_LOW;
    } else if (thisTimestamp - lastEventTimestamp > IR_SIGNAL_TIMEOUT) {
      //we have finished capturing this signal, transmit it via RF
      digitalWrite(13, HIGH);
      //write this to the serial console for debugging
      //for (uint8_t index=0; index < recordingIndex;index++) {
      //  Serial.println(recording[index],DEC);
      //}
      man.transmit(man.encodeMessage(IR_REPEATER_DEVICE_ID, IR_REPEATER_BEGIN_TRANSMISSION));
      //give the receiver a moment to start receiving
      delay(1);
      //transmit the total number of bytes in the stream
      //note that recordingIndex is the number of uint16_t,
      //we want to tell the receiver how many uint8_t are coming
      //Also there are two extra bytes (length and FF footer)
      man.transmit(man.encodeMessage(IR_REPEATER_DEVICE_ID, recordingIndex * 2+2));
      //give the receiver a moment to start receiving
      delay(1);
      //encode the data for transmission via manchester 
      //store the length first
      rfBuffer[0] = recordingIndex;
      rfBuffer[recordingIndex*2+1] = 0;
      for (uint8_t index=0; index < recordingIndex; index++) {
        rfBuffer[index*2+1] = recording[index] >> 8;
        rfBuffer[index*2+2] = recording[index] & 0xFF;
      }
      man.transmitArray(recordingIndex * 2+2, rfBuffer);
      //Serial.println("Transmission complete");
      //Pause the received for a moment to be sure we don't see the command the transmitter sends
      delay(75);
      digitalWrite(13, LOW);
      state = IR_STATE_IDLE;
    }
  } else if (state == IR_STATE_IDLE && digitalRead(IR_RX_PIN)==LOW) {
    //This is the case where we are waiting for a transmission to begin
    lastEventTimestamp = micros();
    state = IR_STATE_RECORDING_LOW;
    recordingIndex = 0;
  }
}
