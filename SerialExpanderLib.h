// SerialExpanderLib.h

#ifndef _SERIALEXPANDERLIB_h
#define _SERIALEXPANDERLIB_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "SerialExpanderChannel.h"


//#define DEBUG_EXPANDER
//#define PROFILE_CHANNELS
#define SERIAL_EXPANDER_NUM_CHANNELS 4

//http://assets.nexperia.com/documents/data-sheet/74HC_HCT4052.pdf
class SerialExpanderLib
{
public:
	SerialExpanderLib(HardwareSerial&, unsigned char, unsigned char, unsigned char);
	
	void begin(bool = true);
	void loop(void);

	inline void manual(void) __attribute__((always_inline))
	{
		_manual_mode = true;
	}

	inline void cyclic(void) __attribute__((always_inline))
	{
		_manual_mode = false;
	}
	
	void channel_loop(void);
	void enable(void);
	void disable();
	bool isEnabled();

	void rotate(void);

	void channel(unsigned char);
	SerialExpanderChannel* currentChannel();
	unsigned char munChannels();

	void add_channel(SerialExpanderChannel*);
private:
	HardwareSerial& _physical_serial;

	unsigned char _r0Pin;
	unsigned char _r1Pin;
	unsigned char _enablePin;
	
	bool _is_enabled;
	bool _is_init;

	uint32_t _channel_activated_at;
	uint32_t _last_byte_at; //Last recived byte
	uint32_t _channel_inicialization_start_at;
	bool _first_character_after_change_arrived; //True if the port just changed

	SerialExpanderChannel* _available_channels[SERIAL_EXPANDER_NUM_CHANNELS];
	SerialExpanderChannel* _current_channel;
	
	uint8_t _ready_channel_count;
	uint16_t _bufferPos;

	bool _manual_mode;

	void _rotate_next_channel(void);

	void _inicialize_channel(uint8_t);
	bool _begin_virtual_channel(void);
	void _set_physical_channel(uint8_t);

	void _update_channel_count(void);
	
	bool _fired_channel_timeout(void);
	bool _fired_first_character_timeout(void);
	bool _fired_inicialization_timeout(void);

#ifdef DEBUG_EXPANDER
	void __debug_print_channel_status(void);
#endif
};

#endif