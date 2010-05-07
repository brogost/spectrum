#pragma once

class FmodHelper
{
public:
	FmodHelper();
	~FmodHelper();
	static FmodHelper& instance();
	bool init();

	bool load(const TCHAR *filename);

	void start();
	void stop();
	bool get_paused();
	void pause(const bool state);

	uint32_t sample_rate() const { return _sample_rate; }
	int	bits() const { return _bits; }
	int	channels() const { return _channels; }
	uint32_t num_samples() const { return _num_samples; }
	uint8_t*	samples() const { return _samples; }

	uint32_t pos_in_ms();

private:

	void	extract_data();

	uint32_t	_sample_rate;
	int	_bits;
	int	_channels;
	uint32_t	_num_samples;
	uint8_t*			_samples;

	FMOD_SOUND_TYPE _type;
	FMOD_SOUND_FORMAT _format;

	static FmodHelper *_instance;
	FMOD::System* _fmod_system;
	FMOD::Channel* _channel;
	FMOD::Sound* _sound;
};

