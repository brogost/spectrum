// HostedDx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <stdint.h>
#include <d3d11.h>
#include <d3dx10.h>
#include <atlbase.h>

namespace
{
  D3D11_VIEWPORT _viewport;
  DXGI_FORMAT _buffer_format;
  D3D_FEATURE_LEVEL _feature_level;
  CComPtr<ID3D11Device> _device;
  CComPtr<IDXGISwapChain> _swap_chain;
  CComPtr<ID3D11DeviceContext> _immediate_context;
  CComPtr<ID3D11RenderTargetView> _render_target_view;
  CComPtr<ID3D11Texture2D> _depth_stencil;
  CComPtr<ID3D11DepthStencilView> _depth_stencil_view;

  DWORD _thread_id = 0;
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

struct ThreadParams
{
  HWND hwnd;
  int width;
  int height;
};

DWORD WINAPI d3d_thread(void *params)
{
  ThreadParams *p = (ThreadParams*)(params);

  if (!init_directx(p->hwnd, p->width, p->height))
    return NULL;

  while (true) {
    D3DXCOLOR c(1, 0, 1, 0);
    _immediate_context->ClearRenderTargetView(_render_target_view, c);
    _immediate_context->ClearDepthStencilView(_depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0 );
    _swap_chain->Present(0,0);
  }

  delete p;

  return 0;
}

extern "C"
{
	__declspec(dllexport) int funky_test()
	{
		return 42;
	}


  LRESULT CALLBACK wnd_proc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) 
  {
    return DefWindowProc( hWnd, message, wParam, lParam );
  }



  __declspec(dllexport) HWND __stdcall create_d3d(int width, int height, HWND parent)
  {
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

    ThreadParams *p = new ThreadParams;
    p->hwnd = hwnd;
    p->width = width;
    p->height = height;
    CreateThread(0, 0, d3d_thread, p, 0, &_thread_id);

    return hwnd;
  }

  __declspec(dllexport) void __stdcall destroy_d3d()
  {

  }

};