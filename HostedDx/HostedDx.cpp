// HostedDx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <concurrent_queue.h>
#include <stdint.h>
#include <d3d11.h>
#include <d3dx10.h>
#include <atlbase.h>
#include <string>
#include <fmod.hpp>
#include <boost/any.hpp>

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

	static DWORD WINAPI d3d_thread(void *params)
	{
		DXWrapper *wrapper = (DXWrapper *)params;
		ThreadParams *p = &wrapper->_params;

		if (!wrapper->init_directx(p->hwnd, p->width, p->height))
			return NULL;

		while (true) {

      // process any commands
      Command cmd;
      while (wrapper->_command_queue.try_pop(cmd)) {
        if (!wrapper->process_command(cmd))
          wrapper->report_error("error running cmd");
      }

			D3DXCOLOR c(1, 0, 1, 0);
			wrapper->_immediate_context->ClearRenderTargetView(wrapper->_render_target_view, c);
			wrapper->_immediate_context->ClearDepthStencilView(wrapper->_depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0 );
			wrapper->_swap_chain->Present(0,0);
		}

		delete p;
		return 0;
	}

	bool load_mp3(const WCHAR *filename)
	{
		if (!FmodWrapper::instance().load(filename))
			return false;

		return true;
	}

	void run(HWND hwnd, int width, int height)
	{
		_params.hwnd = hwnd;
		_params.width = width;
		_params.height = height;
		CreateThread(0, 0, d3d_thread, this, 0, &_thread_id);
	}

	void set_default_render_target()
	{
		ID3D11RenderTargetView* render_targets[] = { _render_target_view };
		_immediate_context->OMSetRenderTargets(1, render_targets, _depth_stencil_view);
		_immediate_context->RSSetViewports(1, &_viewport);
	}

	bool init_directx(const HWND hwnd, const int width, const int height)
	{
		auto buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;

		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = buffer_format;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		const int flags = D3D11_CREATE_DEVICE_DEBUG;

		if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &sd, &_swap_chain, &_device, &_feature_level, &_immediate_context)))
			return false;

		if (_feature_level < D3D_FEATURE_LEVEL_9_3) {
			return false;
		}

		CComPtr<ID3D11Texture2D> back_buffer;
		if (FAILED(_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))))
			return false;

		if (FAILED(_device->CreateRenderTargetView(back_buffer, NULL, &_render_target_view)))
			return false;

		// depth buffer
		D3D11_TEXTURE2D_DESC depthBufferDesc;
		depthBufferDesc.Width = width;
		depthBufferDesc.Height = height;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;

		if (FAILED(_device->CreateTexture2D(&depthBufferDesc, NULL, &_depth_stencil)))
			return false;

		if (FAILED(_device->CreateDepthStencilView(_depth_stencil, NULL, &_depth_stencil_view)))
			return false;

		_viewport = CD3D11_VIEWPORT (0.0f, 0.0f, (float)width, (float)height);
		set_default_render_target();

		return true;
	}

	ThreadParams _params;
	DWORD _thread_id;

	Concurrency::concurrent_queue<Command>	_command_queue;

	D3D11_VIEWPORT _viewport;
	DXGI_FORMAT _buffer_format;
	D3D_FEATURE_LEVEL _feature_level;
	CComPtr<ID3D11Device> _device;
	CComPtr<IDXGISwapChain> _swap_chain;
	CComPtr<ID3D11DeviceContext> _immediate_context;
	CComPtr<ID3D11RenderTargetView> _render_target_view;
	CComPtr<ID3D11Texture2D> _depth_stencil;
	CComPtr<ID3D11DepthStencilView> _depth_stencil_view;
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

};
