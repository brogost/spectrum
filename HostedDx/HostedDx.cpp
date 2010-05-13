// HostedDx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "graphics.hpp"
#include "effect_wrapper.hpp"
#include "fmod_helper.hpp"
#include "app.hpp"


LRESULT CALLBACK wnd_proc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) 
{
	return DefWindowProc( hWnd, message, wParam, lParam );
}

extern "C"
{

#define HOST_EXPORT __declspec(dllexport) 

  HOST_EXPORT HWND __stdcall create_d3d(int width, int height, HWND parent)
  {
		if (!FmodHelper::instance().init())
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

		App::instance().run(hwnd, width, height);

    return hwnd;
  }

  HOST_EXPORT void __stdcall destroy_d3d()
  {
		App::instance().close();

  }

	HOST_EXPORT bool __stdcall load_mp3(const TCHAR *filename)
	{
		if (!App::is_created())
			return false;

    App::instance().add_command(Command(kCmdLoadMp3, std::wstring(filename)));

		return true;
	}

	HOST_EXPORT bool __stdcall start_mp3()
	{
    App::instance().add_command(Command(kCmdStartMp3));
		return true;
	}

	HOST_EXPORT bool __stdcall stop_mp3()
	{
    FmodHelper::instance().stop();
		return true;
	}

	HOST_EXPORT bool __stdcall get_paused()
	{
    return FmodHelper::instance().get_paused();
	}

	HOST_EXPORT void __stdcall set_paused(bool value)
	{
    FmodHelper::instance().pause(value);
	}

	HOST_EXPORT void __stdcall inc_lod()
	{
		App::instance().add_command(Command(kCmdIncLod));
	}

	HOST_EXPORT void __stdcall dec_lod()
	{
		App::instance().add_command(Command(kCmdDecLod));
	}

  HOST_EXPORT void __stdcall inc_range()
  {
    App::instance().add_command(Command(kCmdIncRange));
  }

  HOST_EXPORT void __stdcall dec_range()
  {
    App::instance().add_command(Command(kCmdDecRange));
  }

	HOST_EXPORT void __stdcall inc_page()
	{
		App::instance().add_command(Command(kCmdIncPage));
	}

	HOST_EXPORT void __stdcall dec_page()
	{
		App::instance().add_command(Command(kCmdDecPage));
	}

  HOST_EXPORT void __stdcall set_cutoff(float value)
  {
    App::instance().add_command(Command(kCmdSetCutoff, value));
  }

};
