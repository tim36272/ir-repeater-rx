#include <Manchester.h>

#define TX_PIN 2  //pin where your RF transmitter is connected
#define RX_PIN 0 //pin where your RF receiver is located
#define IR_RX_PIN 3 //pin where your IR receiver is connected


void setup() {
  man.setupTransmit(TX_PIN, MAN_1200);
  setPinMode(IR_RX_PIN, INPUT);
}


}
