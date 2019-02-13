// 
// 
// 

#include "SerialExpanderLib.h"

SerialExpanderLib::SerialExpanderLib(HardwareSerial& serial_in, unsigned char r0Pin, unsigned char r1Pin, unsigned char _enablePin) :
	_physical_serial(serial_in),
	_r0Pin(r0Pin),
	_r1Pin(r1Pin),
	_enablePin(_enablePin ),
	_first_character_after_change_arrived(false ),
	_current_channel( nullptr ),
	_is_init( false ),
	_manual_mode( false ),
	_last_byte_at(0)
{
	//Configuramos los pines
	pinMode(_r0Pin, OUTPUT);
	pinMode(_r1Pin, OUTPUT);
	pinMode(_enablePin, OUTPUT);
	_set_physical_channel(1);
	disable();
}

void SerialExpanderLib::begin(bool enabled)
{
	_is_init = true;
	//Activamos el cacharro
	if (enabled)
		enable();

#ifdef DEBUG_EXPANDER
	Serial.println("SerialExpanderLib::begin()");
	__debug_print_channel_status();
#endif
}

void SerialExpanderLib::channel_loop()
{
	if (!_is_enabled)
	{
		return;
	}

	if (_current_channel == nullptr)
	{
#ifdef DEBUG_EXPANDER
		Serial.println("SerialExpanderLib channel_loop() cant loop, _current_channel is null");
#endif
		return;
	}

	if (!_current_channel->is_init())
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib channel_loop(%d) cant loop, ->is_init() return false\n",_current_channel->channel());
#endif
		return;
	}

	_current_channel->loop();
}

void SerialExpanderLib::loop()
{
	if (!_is_enabled)
	{
		return;
	}

	if (_current_channel == nullptr)
	{
#ifdef DEBUG_EXPANDER
		Serial.println("SerialExpanderLib loop() cant loop, _current_channel is null");
#endif
		return;
	}


	//PipedStream& virtual_device_serial = static_cast<PipedStream&>(_current_channel->expander_end());
	Stream& virtual_device_serial = _current_channel->expander_end();
	
	//Si el canal tiene permitida al escritura en el dispositivo
	if (!_current_channel->read_only())
	{
		//Enviamos los datos.
		while (virtual_device_serial.available())
		{
			_physical_serial.write(virtual_device_serial.read());
		}
	}

	while (_physical_serial.available())
	{
		virtual_device_serial.write((char)_physical_serial.read());
		_last_byte_at = millis();

		if (!_first_character_after_change_arrived)
		{
#ifdef DEBUG_EXPANDER
			Serial.printf("SerialExpanderLib loop() first char from channel [%d] activated_at[%lu] inciated_at[%lu] now[%lu]\n", _current_channel->channel(),_channel_activated_at,_channel_inicialization_start_at,millis());
#endif
			//Reiniciamos le timeout global, para que se ejecute completo
			_channel_activated_at = millis();
			_channel_inicialization_start_at = millis();
			_first_character_after_change_arrived = true;
		}
	}

	//Evaluamos si se requiere algun cambio de canal
	if (!_manual_mode && 
		(_current_channel->is_finish() || //El canal ha indicado que no tiene mas que hacer
		(_current_channel->is_init() && _fired_channel_timeout() ) ||  //El canal esta iniciado y ha saltado el timeout normal del canal
		(_current_channel->is_init() && _fired_first_character_timeout()) ))
	{

#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib loop() should rotate from [%d] now[%lu]\n", _current_channel->channel(), millis());
#endif
		_rotate_next_channel();
		return;
	}
}

bool SerialExpanderLib::_fired_channel_timeout()
{
	uint16_t t = _current_channel->get_channel_timeout();
	if (t > 0 && millis() >= (_channel_activated_at + t))
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _fired_channel_timeout(%d) timeout fired! timeout[%lu] activated[%lu] now[%lu]\n", _current_channel->channel(), _current_channel->get_channel_timeout(), _channel_activated_at, millis());
#endif
		return true;
	}
	return false;
}

bool SerialExpanderLib::_fired_first_character_timeout()
{
	if (_first_character_after_change_arrived)
	{
		return false;
	}

	uint16_t t = _current_channel->get_first_character_timeout();
	if (t > 0 && millis() >= (_last_byte_at + t))
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _fired_first_character_timeout(%d) timeout fired! timeout[%lu] activated[%lu] now[%lu]\n", _current_channel->channel(), _current_channel->get_first_character_timeout(), _last_byte_at, millis());
#endif
		return true;
	}
	return false;
}

bool SerialExpanderLib::_fired_inicialization_timeout()
{
	uint16_t t = _current_channel->get_init_timeout();
	if (t > 0 && millis() >= (_channel_inicialization_start_at + t))
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _fired_inicialization_timeout(%d) timeout fired! timeout[%lu] activated[%lu] now[%lu]\n", _current_channel->channel(), _current_channel->get_init_timeout(), _channel_inicialization_start_at, millis());
#endif
		return true;
	}
	return false;
}

//TODO: Obsoleto, revisar
void SerialExpanderLib::rotate(void)
{
	if (_current_channel == nullptr)
	{
		return;
	}
	_current_channel->deactivate();
}

void SerialExpanderLib::add_channel(SerialExpanderChannel* channel)
{
	if (channel->channel() > SERIAL_EXPANDER_NUM_CHANNELS)
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib ADD CHANNEL ERROR [CH%d] out of bounds, only %d channels\n", channel->channel(), SERIAL_EXPANDER_NUM_CHANNELS);
#endif
		return;
	}

	if (_available_channels[(channel->channel() - 1)] != nullptr)
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib ADD CHANNEL ERROR [CH%d] already in use\n", channel->channel());
#endif
		return;
	}


	_available_channels[(channel->channel() - 1)] = channel;


	_update_channel_count();

	if (_ready_channel_count == 1)
	{
		_physical_serial.begin(channel->baudrate());//TODO: apaño para la primera iteracion
		_inicialize_channel((channel->channel() - 1));
	}

#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib ADD CHANNEL[%d] BAUDRATE[%d]\n", channel->channel(), channel->baudrate());
		__debug_print_channel_status();
#endif
}

#ifdef DEBUG_EXPANDER
void SerialExpanderLib::__debug_print_channel_status()
{
	for (int i = 0; i < SERIAL_EXPANDER_NUM_CHANNELS; i++)
	{
		Serial.printf("CH[%d] INIT[%d] [%p]\n", i + 1, _available_channels[i] == nullptr, _available_channels[i]);
	}
}
#endif

void SerialExpanderLib::channel(unsigned char channel)
{
	if ((uint8_t)channel == _current_channel->channel())
	{
#ifdef DEBUG_EXPANDER
		Serial.println("SerialExpanderLib channel() cant do, alredy on that channel");
#endif
		return;
	}

	if ((uint8_t)channel > SERIAL_EXPANDER_NUM_CHANNELS)
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib channel() [CH%d] out of bounds, only %d channels\n", channel, SERIAL_EXPANDER_NUM_CHANNELS);
#endif
		return;
	}

	if (_available_channels[channel-1] == nullptr)
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib channel() [CH%d] not inicialized\n", channel);
#endif
		return;
	}

	_inicialize_channel(channel - 1);
}

void SerialExpanderLib::_update_channel_count()
{
	_ready_channel_count = SERIAL_EXPANDER_NUM_CHANNELS;
	for (uint8_t i = 0; i < SERIAL_EXPANDER_NUM_CHANNELS; i++)
	{
		if ( _available_channels[i] == nullptr || _available_channels[i]->broken() )
		{
			_ready_channel_count -= 1;
		}
#ifdef DEBUG_EXPANDER
		else
		{
			Serial.printf("SerialExpanderLib _update_channel_count() CH[%d] OK :)\n", i+1);
		}
#endif
	}

	if (_ready_channel_count == 0)
	{
#ifdef DEBUG_EXPANDER
		Serial.println("SerialExpanderLib _update_channel_count() no available channels DISABLING DEVICE! :(");
#endif
		disable();
		return;
	}

#ifdef DEBUG_EXPANDER
	Serial.printf("SerialExpanderLib _update_channel_count() %d ready :)\n", _ready_channel_count);
#endif
}

void SerialExpanderLib::_rotate_next_channel()
{
	//Obtenemos los indices, por un lado el canal actual en el que estamos, que es el numero fisico
	//del canal -1 (que el array empieza en 0)
	uint8_t last_channel = _current_channel->channel()-1;
	//current_index representa el canal que estamos probando ahora
	uint8_t current_index = last_channel;
	SerialExpanderChannel* tmpch = nullptr;
	do {
		//Iteramos sobre los canales, empezando en el siguiente al actual
		current_index += 1;
		//Si el canal actual era el ultimo y el nuevo se sale de rango, nos vamos al 0
		//referencia circular
		if (current_index >= SERIAL_EXPANDER_NUM_CHANNELS)
		{
			current_index = 0;
		}
		//Asignamos el canal
		tmpch = _available_channels[current_index];

		if (tmpch != nullptr && !tmpch->broken())
		{
			break;
		}
	}
	//En caso de que _current_channel no este inicializado (== null) y ademas el canal actual y el nuevo son distintos
	//seguimos iterando
	while (last_channel != current_index);

	//Cuando el bucle termina, habremos comprobado todas las posiciones del arrayd e canales y una de estas 2 situaciones se cumple:
	// - current_index contiene el indice del siguiente canal inicializdo del multiplexor
	// - current_index contiene el indice del mismo el mento que teniamos activo ya que no existen mas canales validos
	_inicialize_channel(current_index);
}

void SerialExpanderLib::_set_physical_channel(uint8_t ch)
{
	switch (ch)
	{
	case 2:
		digitalWrite(_r0Pin, 1);
		digitalWrite(_r1Pin, 0);
		break;
	case 3:
		digitalWrite(_r0Pin, 0);
		digitalWrite(_r1Pin, 1);
		break;
	case 4:
		digitalWrite(_r0Pin, 1);
		digitalWrite(_r1Pin, 1);
		break;
	default://Channel 1
		digitalWrite(_r0Pin, 0);
		digitalWrite(_r1Pin, 0);
	}
}

void SerialExpanderLib::_inicialize_channel(uint8_t ch)
{
#ifdef DEBUG_EXPANDER
	Serial.printf("SerialExpanderLib _inicialize_channel ch[%d] _current_channel[%p]\n", ch + 1, _current_channel);
#endif

	if (_available_channels[ch] == nullptr)
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _inicialize_channel ERROR [CH%d] is null\n", ch + 1);
#endif
		return;
	}


	if (_current_channel != nullptr && ch == (_current_channel->channel() - 1) && _current_channel->is_init())
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _inicialize_channel[CH%d] already selected and init, reseting timeouts\n", ch+1);
#endif
		_channel_activated_at = millis();
		_last_byte_at = millis();
		_first_character_after_change_arrived = false;
		_current_channel->activate();
		return;
	}

	if (_available_channels[ch]->broken())
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _inicialize_channel ERROR [CH%d] is BROKEN\n", ch + 1);
#endif
		//El canal esta roto es como si no existiera
		_rotate_next_channel();
		return;
	}

	_set_physical_channel(_available_channels[ch]->channel());

	if (_current_channel != nullptr)
	{
#ifdef PROFILE_CHANNELS
		Serial.printf("\nSerialExpanderLib in CH[%d] for [%lu]ms now on CH[%d]\n\n", _current_channel->channel(), (millis() - _channel_activated_at), _available_channels[ch]->channel());
#endif
		_current_channel->deactivate();

		if (_current_channel->baudrate() != _available_channels[ch]->baudrate())
		{
			_physical_serial.end();
			_physical_serial.begin(_available_channels[ch]->baudrate());
		}	
	}
	_current_channel = _available_channels[ch];
	
	delay(10);
	while (_physical_serial.available()) { _physical_serial.read(); };
	_physical_serial.flush();

	_channel_activated_at = millis();
	_last_byte_at = millis();
	_first_character_after_change_arrived = false;
	_current_channel->activate();

	if ( !_current_channel->is_init() && !_begin_virtual_channel())
	{
		//Si hemos llegado aqui y ahora el canal esta roto
		//quiere decir que ha apsado en este intento del begin, un canal roto no se puede usar y
		//no se procesara
		if (_current_channel->broken())
		{
			//recalculamos los canales de que disponemos
			_update_channel_count();
		}
		//no se inicia, lo desactivamos y volveremos a probar si toca...
		_current_channel->deactivate();
	}
}


bool SerialExpanderLib::_begin_virtual_channel()
{
#ifdef DEBUG_EXPANDER
	Serial.printf("SerialExpanderLib _begin_channel(%d) iniciando canal now[%lu]\n", _current_channel->channel(), millis());
#endif

#ifdef DEBUG_EXPANDER
	uint32_t start_t = millis();
#endif
	
	_channel_inicialization_start_at = millis();
	_last_byte_at = millis();
	_first_character_after_change_arrived = false;
	while (!_current_channel->begin() && !_fired_inicialization_timeout() && !_fired_first_character_timeout())
	{
		loop();
	}
	
	if (_current_channel->is_init())
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _begin_channel(%d) Inicializacion OK in [%lu]ms now[%lu]\n", _current_channel->channel(),(uint32_t)((millis()-start_t)), millis());
#endif
		//Marcamos el inicio de actividad del canal para que no salte el timeout de canal
		//por haber estado un rato inicializando
		while (_physical_serial.read() >= 0);
		_channel_activated_at = millis();
		_last_byte_at = millis();
		_first_character_after_change_arrived = false;
		_current_channel->reset_tries();
		return true;
	}
	else
	{
#ifdef DEBUG_EXPANDER
		Serial.printf("SerialExpanderLib _begin_channel(%d) Inicializacion FAILED in [%lu]ms now[%lu]\n", _current_channel->channel(), (uint32_t)((millis() - start_t)), millis());
#endif
		_current_channel->increment_try();
		return false;
	}
}

void SerialExpanderLib::enable()
{
	//Enable the muxer chip
	digitalWrite(_enablePin, 0);
	_is_enabled = true;
	//Reactivamos el tiemr general
	_channel_activated_at = millis();
	//Reactivamos el tiemr de primer caracter
	_last_byte_at = millis();
	_first_character_after_change_arrived = false;
}

SerialExpanderChannel* SerialExpanderLib::currentChannel()
{
	return _current_channel;
}

unsigned char SerialExpanderLib::munChannels()
{
	return SERIAL_EXPANDER_NUM_CHANNELS;
}

void SerialExpanderLib::disable()
{
	_is_enabled = false;
	digitalWrite(_enablePin, 1);
}

bool SerialExpanderLib::isEnabled()
{
	return _is_enabled;
	//return !digitalRead(_enablePin);
}