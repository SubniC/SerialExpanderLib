/*
 * SerialExpanderLib — PortExpander
 *
 * Fans Serial1 out to two virtual channels through a 74HC4052 mux pair and
 * echoes whatever arrives on the active channel to the USB serial monitor.
 *
 * Requires the PipedStream library.
 *
 * Wiring: mux address pins R0=4, R1=5, enable=6 (active low).
 */

#include <SerialExpanderLib.h>

// Expander on Serial1, address pins R0=4, R1=5, enable=6.
SerialExpanderLib expander(Serial1, 4, 5, 6);

// Channel(num, baudrate). Channel numbers are 1-based.
SerialExpanderChannel ch1(1, 9600);
SerialExpanderChannel ch2(2, 115200);

void setup() {
  Serial.begin(115200);

  expander.add_channel(&ch1);
  expander.add_channel(&ch2);

  expander.begin();   // enabled, cyclic rotation by default
}

void loop() {
  expander.loop();          // move bytes between the UART and the active channel
  expander.channel_loop();  // run the active channel's loop()

  // Read whatever the active channel received and print it.
  SerialExpanderChannel* current = expander.currentChannel();
  if (current != nullptr) {
    Stream& dev = current->device_end();
    while (dev.available()) {
      Serial.write(dev.read());
    }
  }
}
