// HostedDx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "graphics.hpp"
#include "effect_wrapper.hpp"

struct Command
{
  Command() {}
  Command(const std::string& cmd, const boost::any& param = 0) : _cmd(cmd), _param(param) {}
  std::string _cmd;
  boost::any _param;
};

const char *kCmdLoadMp3 = "load_mp3";
const char *kCmdStartMp3 = "start_mp3";
const char *kCmdPauseMp3 = "pause_mp3";
const char *kCmdIncLod = "inc_lod";
const char *kCmdDecLod = "dec_lod";

class FmodWrapper
{
public:
	FmodWrapper();
	~FmodWrapper();
	static FmodWrapper& instance();
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

	static FmodWrapper *_instance;
	FMOD::System* _fmod_system;
	FMOD::Channel* _channel;
	FMOD::Sound* _sound;
};

FmodWrapper::FmodWrapper()
	: _fmod_system(NULL)
	, _channel(NULL)
	, _sound(NULL)
	, _samples(NULL)
{
}

FmodWrapper::~FmodWrapper()
{
	delete [] _samples;
}

FmodWrapper& FmodWrapper::instance()
{
	if (_instance == NULL)
		_instance = new FmodWrapper();
	return *_instance;
}

uint32_t FmodWrapper::pos_in_ms()
{
	if (!_channel)
		return 0;

	uint32_t pos;
	_channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
	return pos;
}

void FmodWrapper::start()
{
  // TODO: unload old
  if (_channel)
    return;

  if (!_sound)
    return;

  _fmod_system->playSound(FMOD_CHANNEL_FREE, _sound, false, &_channel);
}

void FmodWrapper::stop()
{
  if (!_channel)
    return;

  _channel->stop();
}

bool FmodWrapper::get_paused()
{
  if (!_channel)
    return false;

  bool res; 
  _channel->getPaused(&res);
  return res;
}

void FmodWrapper::pause(const bool state)
{
  if (!_channel)
    return;

  _channel->setPaused(state);
}

bool FmodWrapper::load(const TCHAR *filename)
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

	_sample_rate = (long)(1000 * _num_samples) / (long)ms_len;

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

bool FmodWrapper::init()
{
	if (FMOD::System_Create(&_fmod_system) != FMOD_OK)
		return false;

	if (_fmod_system->init(32, FMOD_INIT_NORMAL, 0) != FMOD_OK)
		return false;

	return true;
}

FmodWrapper *FmodWrapper::_instance = NULL;

struct Lod
{
	Lod() : _vertex_count(0), _stride(0) {}
	uint32_t	_vertex_count;
	uint32_t	_stride;
	CComPtr<ID3D11Buffer>	_vb_left;
	CComPtr<ID3D11Buffer>	_vb_right;
};

struct DXWrapper
{
	static DXWrapper *_instance;

	static bool is_created() { return _instance != NULL; }

	static DXWrapper& instance()
	{
		if (!_instance)
			_instance = new DXWrapper();
		return *_instance;
	}

	DXWrapper()
		: _loaded(false)
		, _cur_lod(0)
	{
	}

	struct ThreadParams
	{
		HWND hwnd;
		int width;
		int height;
	};

  bool process_command(const Command& cmd)
  {
    if (cmd._cmd == kCmdLoadMp3) {
      if (!load_mp3(boost::any_cast<std::wstring>(cmd._param).c_str()))
        return false;
    } else if (cmd._cmd == kCmdStartMp3) {

    } else if (cmd._cmd == kCmdPauseMp3) {

    } else if (cmd._cmd == kCmdIncLod) {
			_cur_lod = std::min<int>(_cur_lod + 1, _lods.size() - 1);
		} else if (cmd._cmd == kCmdDecLod) {
			_cur_lod = std::max<int>(_cur_lod - 1, 0);
		}

    return true;
  }

  void report_error(const std::string& str)
  {
  }

  void add_command(const Command& cmd)
  {
    _command_queue.push(cmd);
  }

	void tick()
	{

		Graphics& g = Graphics::instance();
		ID3D11DeviceContext *context = g.context();

		// process any commands
		Command cmd;
		while (_command_queue.try_pop(cmd)) {
			if (!process_command(cmd))
				report_error("error running cmd");
		}

		D3DXCOLOR c(0.1f, 0.1f, 0.1f, 0);
		g.clear(c);

		if (_loaded) {
			context->VSSetShader(_vs.vertex_shader(), NULL, 0);
			context->PSSetShader(_ps.pixel_shader(), NULL, 0);
			context->GSSetShader(NULL, NULL, 0);

			context->IASetInputLayout(_layout);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

			D3DXMATRIX mtx;
			D3DXMatrixIdentity(&mtx);

			uint32_t ms = FmodWrapper::instance().pos_in_ms();
			D3DXMatrixTranslation(&mtx, -(ms / 10000.0f), 0, 0);
			D3DXMatrixTranspose(&mtx, &mtx);
			_vs.set_variable("mtx", mtx);
			_vs.unmap_buffers();
			_vs.set_cbuffer();

			UINT ofs = 0;
			UINT strides = sizeof(D3DXVECTOR3);
			const Lod& cur = _lods[_cur_lod];
			ID3D11Buffer* bufs[] = { cur._vb_left };
			context->IASetVertexBuffers(0, 1, &bufs[0], &strides, &ofs);

			context->Draw(cur._vertex_count, 0);
		}
		g.present();
	}

	static DWORD WINAPI d3d_thread(void *params)
	{
		DXWrapper *wrapper = (DXWrapper *)params;
		ThreadParams *p = &wrapper->_params;

		Graphics& g = Graphics::instance();

		if (!g.init_directx(p->hwnd, p->width, p->height))
			return NULL;

		while (true) {
			wrapper->tick();
		}

		delete p;
		return 0;
	}

	bool load_mp3(const WCHAR *filename)
	{
		FmodWrapper& f = FmodWrapper::instance();
		if (!f.load(filename))
			return false;

		int16_t *pcm = (int16_t *)f.samples();

		// create a number of lods
		const uint32_t cNumLods = 10;
		uint32_t stride = 1;
		
		for (uint32_t i = 0; i < cNumLods; ++i) {

			const uint32_t c = f.num_samples() / stride;
			D3DXVECTOR3 *v_left = new D3DXVECTOR3[c];
			D3DXVECTOR3 *v_right = new D3DXVECTOR3[c];
			for (uint32_t i = 0, j = 0; j < c; i += stride, ++j) {
				float left = pcm[i*2+0] / 32768.0f;
				float right = pcm[i*2+1] / 32768.0f;
				v_left[j] = D3DXVECTOR3(-1 + i / 44100.0f, left, 0);
				v_right[j] = D3DXVECTOR3(- 1 + i / 44100.0f, right, 0);
			}
			
			Lod lod;
			lod._stride = stride;
			lod._vertex_count = c;
			create_static_vertex_buffer(Graphics::instance().device(), c, sizeof(D3DXVECTOR3), (const uint8_t *)v_left, &lod._vb_left);
			create_static_vertex_buffer(Graphics::instance().device(), c, sizeof(D3DXVECTOR3), (const uint8_t *)v_right, &lod._vb_right);
			
			SAFE_ADELETE(v_left);
			SAFE_ADELETE(v_right);

			_lods.push_back(lod);
			stride *= 2;
		}
		_vs.load_vertex_shader("c:/projects/spectrum/hosteddx/stuff.fx", "vsMain");
		_ps.load_pixel_shader("c:/projects/spectrum/hosteddx/stuff.fx", "psMain");

		D3D11_INPUT_ELEMENT_DESC desc[] = { 
			//CD3D11_INPUT_ELEMENT_DESC("SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0),
			CD3D11_INPUT_ELEMENT_DESC("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0),
		};
		_layout.Attach(_vs.create_input_layout(desc, ELEMS_IN_ARRAY(desc)));


		_loaded = true;
		return true;
	}

	void run(HWND hwnd, int width, int height)
	{
		_params.hwnd = hwnd;
		_params.width = width;
		_params.height = height;
		CreateThread(0, 0, d3d_thread, this, 0, &_thread_id);
	}

	bool _loaded;
	EffectWrapper _vs;
	EffectWrapper _ps;

	ThreadParams _params;
	DWORD _thread_id;

	Concurrency::concurrent_queue<Command>	_command_queue;
	CComPtr<ID3D11InputLayout> _layout;
	std::vector<Lod> _lods;
	int	_cur_lod;
};

DXWrapper *DXWrapper::_instance = NULL;

LRESULT CALLBACK wnd_proc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) 
{
	return DefWindowProc( hWnd, message, wParam, lParam );
}

extern "C"
{

#define HOST_EXPORT __declspec(dllexport) 

  HOST_EXPORT HWND __stdcall create_d3d(int width, int height, HWND parent)
  {
		if (!FmodWrapper::instance().init())
			return false;

    LPCWSTR kClassName = L"HostedDx";
    LPCWSTR kWindowName = L"Test Window";

    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    HINSTANCE hinstance = GetModuleHandle(NULL);

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = wnd_proc;
    wcex.hInstance      = hinstance;
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = kClassName;

    if (!RegisterClassEx(&wcex))
      return NULL;

    const uint32_t window_style = WS_VISIBLE | WS_CHILD;

    HWND hwnd = CreateWindowW(kClassName, kWindowName, window_style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, parent, NULL, hinstance, NULL);

		DXWrapper::instance().run(hwnd, width, height);

    return hwnd;
  }

  HOST_EXPORT void __stdcall destroy_d3d()
  {

  }

	HOST_EXPORT bool __stdcall load_mp3(const TCHAR *filename)
	{
		if (!DXWrapper::is_created())
			return false;

    DXWrapper::instance().add_command(Command(kCmdLoadMp3, std::wstring(filename)));

		return true;
	}

	HOST_EXPORT bool __stdcall start_mp3()
	{
    FmodWrapper::instance().start();
		return true;
	}

	HOST_EXPORT bool __stdcall stop_mp3()
	{
    FmodWrapper::instance().stop();
		return true;
	}

	HOST_EXPORT bool __stdcall get_paused()
	{
    return FmodWrapper::instance().get_paused();
	}

	HOST_EXPORT bool __stdcall set_paused(bool value)
	{
    FmodWrapper::instance().pause(value);
		return true;
	}

	HOST_EXPORT void __stdcall inc_lod()
	{
		DXWrapper::instance().add_command(Command(kCmdIncLod));
	}

	HOST_EXPORT void __stdcall dec_lod()
	{
		DXWrapper::instance().add_command(Command(kCmdDecLod));
	}

};
