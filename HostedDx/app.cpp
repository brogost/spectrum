#include "stdafx.h"
#include "app.hpp"
#include "effect_wrapper.hpp"
#include "graphics.hpp"
#include "fmod_helper.hpp"
#include "renderer.hpp"

namespace
{
	struct Lod
	{
		Lod() : _vertex_count(0), _stride(0) {}
		uint32_t	_vertex_count;
		uint32_t	_stride;
		CComPtr<ID3D11Buffer>	_vb_left;
		CComPtr<ID3D11Buffer>	_vb_right;
	};
}


bool App::is_created() 
{ 
	return _instance != NULL; 
}

App& App::instance()
{
	if (!_instance)
		_instance = new App();
	return *_instance;
}

App::App()
	: _loaded(false)
	, _cur_lod(0)
	, _thread_id(0xffff)
	, _thread_handle(INVALID_HANDLE_VALUE)
	, _renderer(new Renderer())
{
}

bool App::process_command(const Command& cmd)
{
	if (cmd._cmd == kCmdLoadMp3) {
		if (!load_mp3(boost::any_cast<std::wstring>(cmd._param).c_str()))
			return false;
	} else if (cmd._cmd == kCmdStartMp3) {

	} else if (cmd._cmd == kCmdPauseMp3) {

	} else if (cmd._cmd == kCmdIncLod) {
		//_cur_lod = std::min<int>(_cur_lod + 1, _lods.size() - 1);
	} else if (cmd._cmd == kCmdDecLod) {
		//_cur_lod = std::max<int>(_cur_lod - 1, 0);
	} 

	return true;
}

void App::report_error(const std::string& str)
{
}

void App::add_command(const Command& cmd)
{
	_command_queue.push(cmd);
}

bool App::tick()
{
	Graphics& g = Graphics::instance();
	ID3D11DeviceContext *context = g.context();

	// process any commands
	Command cmd;
	while (_command_queue.try_pop(cmd)) {
		if (cmd._cmd == kCmdQuit)
			return false;
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

		uint32_t ms = FmodHelper::instance().pos_in_ms();
		/*
		D3DXMatrixTranslation(&mtx, -(ms / 10000.0f), 0, 0);
		D3DXMatrixTranspose(&mtx, &mtx);
		_vs.set_variable("mtx", mtx);
		_vs.unmap_buffers();
		_vs.set_cbuffer();
		*/
		_renderer->render_at_time(&_vs, context, 0, 1);
	}
	g.present();
	return true;
}

DWORD WINAPI App::d3d_thread(void *params)
{
	App *wrapper = (App *)params;
	ThreadParams *p = &wrapper->_params;

	Graphics& g = Graphics::instance();

	if (!g.init_directx(p->hwnd, p->width, p->height))
		return NULL;

	while (true) {
		if (!wrapper->tick())
			break;
	}

	return 0;
}

bool App::load_mp3(const WCHAR *filename)
{
	FmodHelper& f = FmodHelper::instance();
	if (!f.load(filename))
		return false;

	int16_t *pcm = (int16_t *)f.samples();

	// split the stream into a number of 5 second chunks
	const int len_ms = 1000 * (int64_t)f.num_samples() / f.sample_rate();
	const int chunk_ms = 5000;
	const int sample_rate = f.sample_rate();
	int cur_ms = 0;
	int idx = 0;
	int stride = 10;

	// The vertices are scaled so that the unit along the x-axis is seconds (relative _start_ms), and
	// the y-axis is scaled between -1 and 1
	while (cur_ms < len_ms) {
		int len = std::min<int>(chunk_ms, (len_ms - cur_ms));
		int num_samples = sample_rate * len / 1000;
		D3DXVECTOR3 *v_left = new D3DXVECTOR3[num_samples];
		D3DXVECTOR3 *v_right = new D3DXVECTOR3[num_samples];
		int j = 0;
		for (int i = 0; i < num_samples; i += stride, ++j) {
			// scale to -1..1
			float left = pcm[i*2+0] / 32768.0f;
			float right = pcm[i*2+1] / 32768.0f;
			v_left[j] = D3DXVECTOR3((float)(i / (double)sample_rate), left, 0);
			v_right[j] = D3DXVECTOR3((float)(i / (double)sample_rate), right, 0);
		}

		TimeSlice s;
		s._vertex_count = j;
		s._start_ms = cur_ms;
		s._end_ms = cur_ms + len;
		create_static_vertex_buffer(Graphics::instance().device(), j, sizeof(D3DXVECTOR3), (const uint8_t *)v_left, &s._vb_left);
		create_static_vertex_buffer(Graphics::instance().device(), j, sizeof(D3DXVECTOR3), (const uint8_t *)v_right, &s._vb_right);

		_renderer->_slices.push_back(s);

		SAFE_ADELETE(v_left);
		SAFE_ADELETE(v_right);

		cur_ms += chunk_ms;
	}

#if 0
	// create a number of lods
	const uint32_t cNumLods = 10;
	uint32_t stride = 2;

	for (uint32_t i = 0; i < cNumLods; ++i) {

		const uint32_t c = f.num_samples() / stride;
		D3DXVECTOR3 *v_left = new D3DXVECTOR3[c];
		D3DXVECTOR3 *v_right = new D3DXVECTOR3[c];
		float mm = FLT_MAX;
		for (uint32_t i = 0, j = 0; j < c; i += stride, ++j) {
			// scale to -1..1
			float left = pcm[i*2+0] / 32768.0f;
			float right = pcm[i*2+1] / 32768.0f;
			v_left[j] = D3DXVECTOR3(i / 44100.0f, left, 0);
			v_right[j] = D3DXVECTOR3(i / 44100.0f, right, 0);
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
#endif
	_vs.load_vertex_shader("hosteddx/stuff.fx", "vsMain");
	_ps.load_pixel_shader("hosteddx/stuff.fx", "psMain");

	D3D11_INPUT_ELEMENT_DESC desc[] = { 
		//CD3D11_INPUT_ELEMENT_DESC("SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0),
		CD3D11_INPUT_ELEMENT_DESC("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0),
	};
	_layout.Attach(_vs.create_input_layout(desc, ELEMS_IN_ARRAY(desc)));


	_loaded = true;
	return true;
}

App::~App()
{
	SAFE_DELETE(_renderer);
	if (_thread_handle != INVALID_HANDLE_VALUE) {
		_command_queue.push(Command(kCmdQuit));
		WaitForSingleObject(_thread_handle, INFINITE);
	}
	Graphics::instance().close();
	_layout = NULL;
	_instance = NULL;
}

void App::close()
{
	delete this;
}

void App::run(HWND hwnd, int width, int height)
{
	_params.hwnd = hwnd;
	_params.width = width;
	_params.height = height;
	_thread_handle = CreateThread(0, 0, d3d_thread, this, 0, &_thread_id);
}


App *App::_instance = NULL;
