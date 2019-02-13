#include "SerialExpanderChannel.h"

SerialExpanderChannel::SerialExpanderChannel(uint8_t num, uint32_t baudrate, bool read_only, uint16_t channel_timeout, uint16_t first_character_timeout, uint16_t inicialization_timeout, uint16_t buffer_size) :
	_num_channel(num),
	_baudrate(baudrate),
	_read_only(read_only),
	channel_timeout(channel_timeout),
	first_character_timeout(first_character_timeout),
	inicialization_timeout(inicialization_timeout),
	_internal_buffer(buffer_size),
	_ch_pipes{ buffer_size },
	_ch_expander(_ch_pipes.first),
	_ch_device(_ch_pipes.second),
	_init_tries(0)
{
}

void SerialExpanderChannel::loop()
{
	//TODO: DEBUG TEMPORAL
	while (_ch_device.available())
	{
		//Serial.println("device loop");

		Serial.print((char)_ch_device.read());
		//Serial.printf("DEVICE #%d: %c\n", channel() , _ch_device.read() );
	}
}