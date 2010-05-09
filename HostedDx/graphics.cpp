#include "stdafx.h"
#include "graphics.hpp"

#pragma comment (lib, "D3D11.lib")
#pragma comment (lib, "D3D10_1.lib")
#pragma comment (lib, "D2D1.lib")
#pragma comment (lib, "DXErr.lib")
#pragma comment (lib, "DXGI.lib")

Graphics* Graphics::_instance = NULL;

Graphics& Graphics::instance()
{
	if (_instance == NULL) {
		_instance = new Graphics();
	}

	return *_instance;
}

Graphics::Graphics()
	: _width(-1)
	, _height(-1)
{
}

Graphics::~Graphics()
{
}

extern ID2D1RenderTarget *m_pBackBufferRT;
extern ID2D1SolidColorBrush *m_pBackBufferTextBrush;
extern IDWriteFactory *m_pDWriteFactory;
extern IDWriteTextFormat *m_pTextFormat;

bool Graphics::init_directx(const HWND hwnd, const int width, const int height)
{
	_width = width;
	_height = height;
	_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 2;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = _buffer_format;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;


	// Create DXGI factory to enumerate adapters
	CComPtr<IDXGIFactory1> dxgi_factory;
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&dxgi_factory)))
		return false;

	// Use the first adapter
	UINT i = 0; 
	IDXGIAdapter1 * pAdapter; 
	std::vector <IDXGIAdapter1*> vAdapters; 
	while(dxgi_factory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) { 
		vAdapters.push_back(pAdapter); 
		++i; 
	}

	const int flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	// Create the DX11 device
	RETURN_ON_FAIL_BOOL(D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &sd, &_swap_chain, &_device, &_feature_level, &_immediate_context),
    ErrorPredicate<HRESULT>, LOG_ERROR_LN);

	if (_feature_level < D3D_FEATURE_LEVEL_9_3) {
    LOG_ERROR_LN("Card must support at least D3D_FEATURE_LEVEL_9_3");
		return false;
	}

	CComPtr<ID3D11Texture2D> back_buffer;
	RETURN_ON_FAIL_BOOL(_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)), ErrorPredicate<HRESULT>, LOG_ERROR_LN);
	RETURN_ON_FAIL_BOOL(_device->CreateRenderTargetView(back_buffer, NULL, &_render_target_view), ErrorPredicate<HRESULT>, LOG_ERROR_LN);

	D3D11_TEXTURE2D_DESC back_buffer_desc;
	back_buffer->GetDesc(&back_buffer_desc);

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

	RETURN_ON_FAIL_BOOL(_device->CreateTexture2D(&depthBufferDesc, NULL, &_depth_stencil), ErrorPredicate<HRESULT>, LOG_ERROR_LN);
	RETURN_ON_FAIL_BOOL(_device->CreateDepthStencilView(_depth_stencil, NULL, &_depth_stencil_view), ErrorPredicate<HRESULT>, LOG_ERROR_LN);

	_viewport = CD3D11_VIEWPORT (0.0f, 0.0f, (float)_width, (float)_height);
	set_default_render_target();

	//if (!create_d3d10_device(NULL, back_buffer_desc))
		//return false;

	return true;
}

bool Graphics::create_d3d10_device(IDXGIAdapter1* adapter, const D3D11_TEXTURE2D_DESC& back_buffer_desc)
{

	// Create the DX10 device
	if (FAILED(D3D10CreateDevice1(
		NULL,
		D3D10_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D10_CREATE_DEVICE_DEBUG |
		D3D10_CREATE_DEVICE_BGRA_SUPPORT,
		D3D10_FEATURE_LEVEL_9_3,
		D3D10_1_SDK_VERSION,
		&_device10)))
		return false;

	// Create the shared texture to draw D2D content to
	D3D11_TEXTURE2D_DESC sharedTextureDesc;
	CComPtr<ID3D11Texture2D> pSharedTexture11;

	ZeroMemory(&sharedTextureDesc, sizeof(sharedTextureDesc));
	sharedTextureDesc.Width = back_buffer_desc.Width;
	sharedTextureDesc.Height = back_buffer_desc.Height;
	sharedTextureDesc.MipLevels = 1;
	sharedTextureDesc.ArraySize = 1;
	sharedTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sharedTextureDesc.SampleDesc.Count = 1;
	sharedTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sharedTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	sharedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

	HRESULT hResult = _device->CreateTexture2D(&sharedTextureDesc, NULL, &pSharedTexture11);

	// Get the keyed mutex for the shared texture (for D3D11)
	CComPtr<IDXGIKeyedMutex> pKeyedMutex11;
	hResult = pSharedTexture11->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&pKeyedMutex11);

	// Get the shared handle needed to open the shared texture in D3D10.1
	CComPtr<IDXGIResource> pSharedResource11;
	HANDLE hSharedHandle11;
	hResult = pSharedTexture11->QueryInterface(__uuidof(IDXGIResource), (void**)&pSharedResource11);
	hResult = pSharedResource11->GetSharedHandle(&hSharedHandle11);

	// Open the surface for the shared texture in D3D10.1
	CComPtr<IDXGISurface1> pSharedSurface10;
	hResult = _device10->OpenSharedResource(hSharedHandle11, __uuidof(IDXGISurface1), (void**)&pSharedSurface10);

	// Get the keyed mutex for the shared texture (for D3D10.1)
	CComPtr<IDXGIKeyedMutex> pKeyedMutex10;
	hResult = pSharedSurface10->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&pKeyedMutex10);

	CComPtr<ID2D1Factory> pD2DFactory;
	hResult = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), (void**)&pD2DFactory);

	// Create D2D render target from the surface for the shared texture, which was opened in D3D10.1
	D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties;
	//CComPtr<ID2D1RenderTarget> pD2DRenderTarget;
	ZeroMemory(&renderTargetProperties, sizeof(renderTargetProperties));
	renderTargetProperties.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
	renderTargetProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);
	hResult = pD2DFactory->CreateDxgiSurfaceRenderTarget(pSharedSurface10, &renderTargetProperties, &m_pBackBufferRT);

	//CComPtr<ID2D1SolidColorBrush> pBrush;
	hResult = m_pBackBufferRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f), &m_pBackBufferTextBrush);

	// Create DWrite factory
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown **>(&m_pDWriteFactory) );
	static const WCHAR msc_fontName[] = L"Verdana";
	static const FLOAT msc_fontSize = 50;

	// Create DWrite text format object
	m_pDWriteFactory->CreateTextFormat(
		msc_fontName,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		msc_fontSize,
		L"", //locale
		&m_pTextFormat
		);

	return true;
}

void Graphics::set_default_render_target()
{
	ID3D11RenderTargetView* render_targets[] = { _render_target_view };
	_immediate_context->OMSetRenderTargets(1, render_targets, _depth_stencil_view);
	_immediate_context->RSSetViewports(1, &_viewport);
}

bool Graphics::close()
{
	if (!close_directx()) {
		return false;
	}

	return true;
}

bool Graphics::close_directx()
{
	_depth_stencil_view.Release();
	_depth_stencil.Release();
	_render_target_view.Release();
	_immediate_context.Release();
	_swap_chain.Release();
	_device.Release();
	return true;
}

void Graphics::clear(const D3DXCOLOR& c)
{
	_immediate_context->ClearRenderTargetView(_render_target_view, c);
	_immediate_context->ClearDepthStencilView(_depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0 );
}

void Graphics::present()
{
	_swap_chain->Present(0,0);
}

void Graphics::resize(const int width, const int height)
{
	if (_swap_chain) {
		_swap_chain->ResizeBuffers(1, width, height, _buffer_format, 0);
	}
}
