#pragma once
#include "effect_wrapper.hpp"

struct Command
{
	Command() {}
	Command(const std::string& cmd, const boost::any& param = 0) : _cmd(cmd), _param(param) {}
	std::string _cmd;
	boost::any _param;
};

static const char *kCmdLoadMp3 = "load_mp3";
static const char *kCmdStartMp3 = "start_mp3";
static const char *kCmdPauseMp3 = "pause_mp3";
static const char *kCmdIncLod = "inc_lod";
static const char *kCmdDecLod = "dec_lod";
static const char *kCmdQuit = "quit";

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

	ThreadParams _params;
	HANDLE _thread_handle;
	DWORD _thread_id;

	static App *_instance;
	Concurrency::concurrent_queue<Command>	_command_queue;
	CComPtr<ID3D11InputLayout> _layout;
//	std::vector<Lod> _lods;
	int	_cur_lod;
};
