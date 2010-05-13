#pragma once
#include "effect_wrapper.hpp"

enum Cmd
{
	kCmdLoadMp3,
	kCmdStartMp3,
	kCmdPauseMp3,
	kCmdIncLod,
	kCmdDecLod,
	kCmdIncRange,
	kCmdDecRange,
	kCmdSetCutoff,
	kCmdIncPage,
	kCmdDecPage,
	kCmdQuit,
};

struct Command
{
	Command() {}
	Command(const Cmd cmd, const boost::any& param = 0) : _cmd(cmd), _param(param) {}
	Cmd _cmd;
	boost::any _param;
};

struct Renderer;

class App
{
public:

	App();
	~App();
	static bool is_created();
	static App& instance();
	void add_command(const Command& cmd);

	bool load_mp3(const WCHAR *filename);
	void close();
	void run(HWND hwnd, int width, int height);
private:
	void	create_buffers();
	void	render();
	void	draw_text();
	bool	init();
  void  create_layout();
  void  process_cutoff(const float v);
	bool process_command(const Command& cmd);
	void report_error(const std::string& str);
	bool tick();
	static DWORD WINAPI d3d_thread(void *params);

	struct ThreadParams
	{
		HWND hwnd;
		int width;
		int height;
	};

  Renderer *_renderer;

	bool _loaded;
	EffectWrapper _vs;
	EffectWrapper _ps;

	EffectWrapper _fs_vs;
	EffectWrapper _fs_ps;

	ThreadParams _params;
	HANDLE _thread_handle;
	DWORD _thread_id;

	static App *_instance;
	Concurrency::concurrent_queue<Command>	_command_queue;
	CComPtr<ID3D11InputLayout> _layout;
  CComPtr<ID3D11Buffer> _vb_db_lines;
	CComPtr<ID3D11SamplerState> _sampler;
	CComPtr<ID3D11DepthStencilState> _fs_depth_state;
	CComPtr<ID3D11DepthStencilState> _lines_depth_state;
  uint32_t _db_vertex_count;
	int	_cur_lod;
  int _cur_range;


};
