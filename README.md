# SerialExpanderLib

Arduino library that turns a pair of dual 2-channel analog multiplexers
(**74HC4052**) into a **serial port expander**: it fans a single hardware serial
port out to up to **four virtual channels**, each with its own baud rate,
timeouts and read/write policy.

> **Legacy library (2019).** Kept for reference; docs refreshed in 2026,
> library behavior unchanged.

Each virtual channel is exposed as a `Stream` (via a `PipedStream` pair), so your
sketch talks to a channel exactly like it would talk to `Serial`, while the
library drives the mux address pins and shares the single physical UART between
all channels.

## Features

- Up to 4 virtual serial channels over one hardware UART and a 74HC4052 pair.
- Per-channel baud rate, switching the physical UART automatically on rotation.
- Per-channel timeouts: channel time, first-character time and init time.
- Automatic cyclic rotation across channels, or manual channel selection.
- Read-only channels (mux output only, no writes back to the device).
- Broken-channel detection (a channel that fails to init twice is skipped).
- Each channel exposed as a standard Arduino `Stream`.

## Dependencies

This library uses [**PipedStream**](https://github.com/paulo-raca/ArduinoBufferedStreams)
(`PipedStream` / `PipedStreamPair`) to provide the in-memory `Stream` pipe for
each channel. Install it alongside this library.

## Installation

- **Manual**: copy this folder into your Arduino `libraries/` directory
  (alongside `PipedStream`) and restart the IDE.
- **PlatformIO**: add the repository to `lib_deps`.

## Wiring

Two 74HC4052 (dual 1-of-4 analog mux) are wired so the Arduino TX/RX line is
routed to one of four physical serial pairs. The two address lines `R0`/`R1`
select the active channel and the `Enable` pin (active low) turns the mux on:

| Channel | R1 | R0 |
|--------:|:--:|:--:|
| 1       | 0  | 0  |
| 2       | 0  | 1  |
| 3       | 1  | 0  |
| 4       | 1  | 1  |

## Usage

```cpp
#include <SerialExpanderLib.h>

// Expander on Serial1, address pins R0=4, R1=5, enable=6.
SerialExpanderLib expander(Serial1, 4, 5, 6);

// Channel(num, baudrate, read_only, channel_timeout, first_char_timeout,
//         init_timeout, buffer_size)
SerialExpanderChannel ch1(1, 9600);
SerialExpanderChannel ch2(2, 115200);

void setup() {
  Serial.begin(115200);

  expander.add_channel(&ch1);
  expander.add_channel(&ch2);

  expander.begin();   // enabled, cyclic rotation by default
}

void loop() {
  expander.loop();          // pump bytes between the UART and the active channel
  expander.channel_loop();  // run the active channel's own loop()

  // Talk to a channel through its device-side Stream:
  Stream& dev = expander.currentChannel()->device_end();
  if (dev.available()) {
    Serial.write(dev.read());
  }
}
```

See [`examples/PortExpander`](examples/PortExpander) for the full sketch.

## API

### `SerialExpanderLib`

```cpp
SerialExpanderLib(HardwareSerial& serial, unsigned char r0Pin,
                  unsigned char r1Pin, unsigned char enablePin);

void begin(bool enabled = true);
void loop();                         // move bytes between UART and active channel
void channel_loop();                 // run active channel's loop()

void add_channel(SerialExpanderChannel* channel);
void channel(unsigned char num);     // manually select a channel (1-based)
SerialExpanderChannel* currentChannel();
unsigned char munChannels();         // number of channel slots (4)

void manual();                       // disable automatic rotation
void cyclic();                       // enable automatic rotation
void rotate();

void enable();
void disable();
bool isEnabled();
```

### `SerialExpanderChannel`

```cpp
SerialExpanderChannel(uint8_t num,
                      uint32_t baudrate                 = 115200,
                      bool     read_only                = false,
                      uint16_t channel_timeout          = 2000,
                      uint16_t first_character_timeout  = 500,
                      uint16_t inicialization_timeout   = 5000,
                      uint16_t buffer_size              = 64);

virtual bool begin();    // override to implement a channel handshake
virtual void loop();     // override to process device-side data

Stream& device_end();    // sketch side of the pipe
Stream& expander_end();  // library side of the pipe

bool   broken();
bool   is_init();
bool   is_finish();
bool   read_only();
uint32_t baudrate();
uint8_t  channel();
```

Subclass `SerialExpanderChannel` and override `begin()` / `loop()` to add a
per-device handshake or protocol.

## License

[MIT](LICENSE) © 2019–2026 mdps

---

_Un proyecto de mdps · 2026 · desarrollado en Murcia._
