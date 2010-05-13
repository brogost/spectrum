#include "stdafx.h"
#include "fmod_helper.hpp"

FmodHelper::FmodHelper()
	: _fmod_system(NULL)
	, _channel(NULL)
	, _sound(NULL)
	, _samples(NULL)
{
}

FmodHelper::~FmodHelper()
{
	delete [] _samples;
}

FmodHelper& FmodHelper::instance()
{
	if (_instance == NULL)
		_instance = new FmodHelper();
	return *_instance;
}

int32_t FmodHelper::pos_in_ms()
{
	if (!_channel)
		return 0;

	uint32_t pos;
	_channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
	return pos;
}

void FmodHelper::change_pos(const int32_t ofs)
{
	if (!_channel)
		return;

	uint32_t cur;
	_channel->getPosition(&cur, FMOD_TIMEUNIT_MS);

	uint32_t len;
	_sound->getLength(&len, FMOD_TIMEUNIT_MS);

	int32_t new_pos = std::min<int32_t>(len, std::max<int32_t>(0, (int32_t)cur + ofs));

	_channel->setPosition(new_pos, FMOD_TIMEUNIT_MS);
}

void FmodHelper::start()
{
	// TODO: unload old
	if (_channel)
		return;

	if (!_sound)
		return;

	_fmod_system->playSound(FMOD_CHANNEL_FREE, _sound, false, &_channel);
	//_channel->setVolume(0);
}

void FmodHelper::stop()
{
	if (!_channel)
		return;

	_channel->stop();
}

bool FmodHelper::get_paused()
{
	if (!_channel)
		return false;

	bool res; 
	_channel->getPaused(&res);
	return res;
}

void FmodHelper::pause(const bool state)
{
	if (!_channel)
		return;

	_channel->setPaused(state);
}

bool FmodHelper::load(const TCHAR *filename)
{
	// load file
	char buf[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, filename, -1, buf, MAX_PATH, NULL, NULL);
	if (_fmod_system->createSound(buf, FMOD_HARDWARE, 0, &_sound) != FMOD_OK)
		return false;


	// get format, length etc
	if (_sound->getFormat(&_type, &_format, &_channels, &_bits) != FMOD_OK)
		return false;

	uint32_t raw_len = 0;
	uint32_t ms_len = 0;
	_sound->getLength(&ms_len, FMOD_TIMEUNIT_MS);
	_sound->getLength(&raw_len, FMOD_TIMEUNIT_PCMBYTES);
	_sound->getLength(&_num_samples, FMOD_TIMEUNIT_PCM);

	_sample_rate = 1000 * (uint64_t)_num_samples / ms_len;

	// copy the raw bytes
	_samples = new uint8_t[raw_len];
	void *ptr1, *ptr2;
	uint32_t len1, len2;
	if (_sound->lock(0, raw_len, &ptr1, &ptr2, &len1, &len2) != FMOD_OK)
		return false;

	memcpy(_samples, ptr1, len1);

	_sound->unlock(ptr1, ptr2, len1, len2);

	return true;
}

bool FmodHelper::init()
{
	if (FMOD::System_Create(&_fmod_system) != FMOD_OK)
		return false;

	if (_fmod_system->init(32, FMOD_INIT_NORMAL, 0) != FMOD_OK)
		return false;

	return true;
}

FmodHelper *FmodHelper::_instance = NULL;
