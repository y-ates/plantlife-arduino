#include <RCSwitch.h>

RCSwitch sw = RCSwitch();
int i=0;

void setup() {
  sw.enableTransmit(10);
  sw.setProtocol(2);
}

void loop() {
  sw.send(i, 24);
  i++;
  delay(1000);
}
