#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <PipedStream.h>


class SerialExpanderChannel
{
public:

	SerialExpanderChannel(uint8_t, uint32_t = 115200, bool = false, uint16_t=2000, uint16_t=500, uint16_t = 5000, uint16_t=64);
		
	virtual bool begin(void) { _init = true; return true;  };
	virtual void loop(void);


	inline bool const broken(void) __attribute__((always_inline))
	{
		return (_init_tries>=2);
	}

	inline void reset_tries(void) __attribute__((always_inline))
	{
		_init_tries = 0;
	}

	inline void increment_try(void) __attribute__((always_inline))
	{
		_init_tries += 1;
	}

	inline bool is_init(void) __attribute__((always_inline))
	{
		return _init;
	}

	inline uint16_t const get_channel_timeout(void) __attribute__((always_inline))
	{
		return channel_timeout;
	}

	inline uint16_t const get_init_timeout(void) __attribute__((always_inline))
	{
		return inicialization_timeout;
	}

	inline uint16_t const get_first_character_timeout(void) __attribute__((always_inline))
	{
		return first_character_timeout;
	}

	inline void deactivate(void) __attribute__((always_inline))
	{
		_active = false;
	}

	inline void activate(void) __attribute__((always_inline))
	{
		_active = true;
	}

	inline bool is_finish(void) __attribute__((always_inline))
	{
		return !_active;
	}

	inline bool const read_only() __attribute__((always_inline))
	{
		return _read_only;
	}

	inline uint32_t const baudrate() __attribute__((always_inline))
	{
		return _baudrate;
	}

	inline uint8_t const channel() __attribute__((always_inline))
	{
		return _num_channel;
	}

	inline Stream& device_end(void) __attribute__((always_inline)) {
		return _ch_device;
	}

	inline Stream& expander_end(void) __attribute__((always_inline)) {
		return _ch_expander;
	}
protected:
	//const?
	uint16_t first_character_timeout;
	uint16_t channel_timeout;
	uint16_t inicialization_timeout;
	PipedStream& _ch_device;
	bool _init;
	bool _active;
	uint8_t _init_tries;

private:
	PipedStreamPair _ch_pipes;
	PipedStream& _ch_expander;

	const uint16_t _internal_buffer;

	uint8_t _num_channel;
	uint32_t _baudrate;
	
	bool _read_only;
};