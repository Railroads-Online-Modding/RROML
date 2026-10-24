#include <windows.h>
#include <metahost.h>
#include <dxgi1_4.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <d3dcompiler.h>
#include <strsafe.h>
#include <mmsystem.h>
#include <string.h>

#pragma comment(lib, "ole32.lib")

typedef HRESULT (STDAPICALLTYPE *CLRCreateInstanceFn)(REFCLSID, REFIID, LPVOID*);
typedef HRESULT (WINAPI *D3D11On12CreateDeviceFn)(IUnknown*, UINT, const D3D_FEATURE_LEVEL*, UINT, IUnknown* const*, UINT, UINT, ID3D11Device**, ID3D11DeviceContext**, D3D_FEATURE_LEVEL*);
typedef DWORD (WINAPI *XInputGetStateFn)(DWORD, LPVOID);
typedef DWORD (WINAPI *XInputSetStateFn)(DWORD, LPVOID);
typedef DWORD (WINAPI *XInputGetCapabilitiesFn)(DWORD, DWORD, LPVOID);
typedef DWORD (WINAPI *XInputGetDSoundAudioDeviceGuidsFn)(DWORD, GUID*, GUID*);
typedef DWORD (WINAPI *XInputGetBatteryInformationFn)(DWORD, BYTE, LPVOID);
typedef DWORD (WINAPI *XInputGetKeystrokeFn)(DWORD, DWORD, LPVOID);
typedef VOID (WINAPI *XInputEnableFn)(BOOL);
typedef DWORD (WINAPI *XInputGetStateExFn)(DWORD, LPVOID);
typedef DWORD (WINAPI *XInputUnusedOrdinalFn)(void);
typedef BOOL (WINAPI *QueryPerformanceCounterFn)(LARGE_INTEGER*);
typedef BOOL (WINAPI *QueryPerformanceFrequencyFn)(LARGE_INTEGER*);
typedef ULONGLONG (WINAPI *GetTickCount64Fn)(void);
typedef DWORD (WINAPI *GetTickCountFn)(void);
typedef DWORD (WINAPI *TimeGetTimeFn)(void);

typedef HRESULT (WINAPI *CreateDXGIFactoryFn)(REFIID, void**);
typedef HRESULT (WINAPI *CreateDXGIFactory2Fn)(UINT, REFIID, void**);
typedef HRESULT (WINAPI *D3DCompileFn)(const void*, SIZE_T, const char*, const D3D_SHADER_MACRO*, ID3DInclude*, const char*, const char*, UINT, UINT, ID3DBlob**, ID3DBlob**);
typedef HRESULT (STDMETHODCALLTYPE *FactoryCreateSwapChainFn)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
typedef HRESULT (STDMETHODCALLTYPE *FactoryCreateSwapChainForHwndFn)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
typedef HRESULT (STDMETHODCALLTYPE *FactoryCreateSwapChainForCompositionFn)(IDXGIFactory2*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**);
typedef HRESULT (STDMETHODCALLTYPE *SwapChainPresentFn)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT (STDMETHODCALLTYPE *SwapChainPresent1Fn)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);

enum OverlayBackend
{
    OverlayBackend_None = 0,
    OverlayBackend_DX11 = 1,
    OverlayBackend_DX12 = 2
};

struct OverlayRenderer
{
    OverlayBackend backend;
    IDXGISwapChain* swapChain;
    IUnknown* deviceArg;
    ID2D1Factory1* d2dFactory;
    IDWriteFactory* writeFactory;
    IDWriteTextFormat* textFormat;
    ID3D11Device* d3d11Device;
    ID3D11DeviceContext* d3d11Context;
    ID3D11Texture2D* overlayTexture;
    ID3D11ShaderResourceView* overlayTextureView;
    ID2D1RenderTarget* overlayD2dTarget;
    ID2D1SolidColorBrush* overlayBgBrush;
    ID2D1SolidColorBrush* overlayTextBrush;
    ID3D11VertexShader* overlayVertexShader;
    ID3D11PixelShader* overlayPixelShader;
    ID3D11InputLayout* overlayInputLayout;
    ID3D11Buffer* overlayVertexBuffer;
    ID3D11Buffer* overlayConstants;
    ID3D11SamplerState* overlaySampler;
    ID3D11BlendState* overlayBlendState;
    ID3D11RasterizerState* overlayRasterizerState;
    ID3D12CommandQueue* d3d12Queue;
    ID3D12Device* d3d12Device;
    ID3D11On12Device* d3d11On12;
    ID2D1Device* d2dDevice;
    ID2D1DeviceContext* d2dContext;
    ID2D1SolidColorBrush* d2dBrush;
    IDXGISwapChain3* swapChain3;
    UINT bufferCount;
    ID3D11Resource* wrappedBackBuffers[8];
    ID2D1Bitmap1* d2dBitmaps[8];
};

struct D3D11StateBackup
{
    ID3D11RenderTargetView* renderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* depthView;
    ID3D11BlendState* blendState;
    FLOAT blendFactor[4];
    UINT sampleMask;
    ID3D11DepthStencilState* depthState;
    UINT stencilRef;
    ID3D11RasterizerState* rasterizerState;
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT viewportCount;
    ID3D11InputLayout* inputLayout;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    ID3D11Buffer* vertexBuffer;
    UINT vertexStride;
    UINT vertexOffset;
    ID3D11VertexShader* vertexShader;
    ID3D11HullShader* hullShader;
    ID3D11DomainShader* domainShader;
    ID3D11GeometryShader* geometryShader;
    ID3D11PixelShader* pixelShader;
    ID3D11Buffer* vertexConstants;
    ID3D11ShaderResourceView* pixelResource;
    ID3D11SamplerState* pixelSampler;
    ID3D11Predicate* predicate;
    BOOL predicateValue;
};

static HMODULE g_realXInput = NULL;
static HMODULE g_d3dCompiler = NULL;
static D3DCompileFn g_d3dCompile = NULL;
static XInputGetStateFn g_xinputGetState = NULL;
static XInputSetStateFn g_xinputSetState = NULL;
static XInputGetCapabilitiesFn g_xinputGetCapabilities = NULL;
static XInputGetDSoundAudioDeviceGuidsFn g_xinputGetDSoundAudioDeviceGuids = NULL;
static XInputGetBatteryInformationFn g_xinputGetBatteryInformation = NULL;
static XInputGetKeystrokeFn g_xinputGetKeystroke = NULL;
static XInputEnableFn g_xinputEnable = NULL;
static XInputGetStateExFn g_xinputGetStateEx = NULL;
static XInputUnusedOrdinalFn g_xinputOrdinal101 = NULL;
static XInputUnusedOrdinalFn g_xinputOrdinal102 = NULL;
static XInputUnusedOrdinalFn g_xinputOrdinal103 = NULL;
static LONG g_bootstrapStarted = 0;
static LONG g_dxgiHookInstalled = 0;
static LONG g_presentHookLogged = 0;
static LONG g_overlayRenderFailureLogged = 0;
static LONG g_overlayStateLogged = 0;
static CRITICAL_SECTION g_rendererLock;
static LONG g_rendererLockReady = 0;
static CRITICAL_SECTION g_overlayLock;
static LONG g_overlayLockReady = 0;
static wchar_t g_overlayText[256] = L"";
static int g_overlayStatusKind = 0;
static DWORD g_overlayHideTick = 0;
static int g_overlayPosition = 0;
static OverlayRenderer g_renderer = {};

static CreateDXGIFactoryFn g_realCreateDXGIFactory = NULL;
static CreateDXGIFactoryFn g_realCreateDXGIFactory1 = NULL;
static CreateDXGIFactory2Fn g_realCreateDXGIFactory2 = NULL;
static FactoryCreateSwapChainFn g_realFactoryCreateSwapChain = NULL;
static FactoryCreateSwapChainForHwndFn g_realFactoryCreateSwapChainForHwnd = NULL;
static FactoryCreateSwapChainForCompositionFn g_realFactoryCreateSwapChainForComposition = NULL;
static SwapChainPresentFn g_realSwapChainPresent = NULL;
static SwapChainPresent1Fn g_realSwapChainPresent1 = NULL;
static QueryPerformanceCounterFn g_realQueryPerformanceCounter = NULL;
static QueryPerformanceFrequencyFn g_realQueryPerformanceFrequency = NULL;
static GetTickCount64Fn g_realGetTickCount64 = NULL;
static GetTickCountFn g_realGetTickCount = NULL;
static TimeGetTimeFn g_realTimeGetTime = NULL;
static CRITICAL_SECTION g_timeLock;
static LONG g_timeLockReady = 0;
static LONG g_timeHooksInstalled = 0;
static double g_timeScale = 1.0;
static BOOL g_timeStateInitialized = FALSE;
static LARGE_INTEGER g_qpcBaseReal = {};
static LARGE_INTEGER g_qpcBaseVirtual = {};
static LARGE_INTEGER g_qpcFrequency = {};
static ULONGLONG g_tick64BaseReal = 0;
static ULONGLONG g_tick64BaseVirtual = 0;
static DWORD g_tickBaseReal = 0;
static DWORD g_tickBaseVirtual = 0;
static DWORD g_timeGetBaseReal = 0;
static DWORD g_timeGetBaseVirtual = 0;

static bool ResolveOverlayState(wchar_t* text, int* statusKind, int* position);
template <typename T>
static void SafeRelease(T** value)
{
    if (value != NULL && *value != NULL)
    {
        (*value)->Release();
        *value = NULL;
    }
}

static void CaptureD3D11State(ID3D11DeviceContext* context, D3D11StateBackup* state)
{
    context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, state->renderTargets, &state->depthView);
    context->OMGetBlendState(&state->blendState, state->blendFactor, &state->sampleMask);
    context->OMGetDepthStencilState(&state->depthState, &state->stencilRef);
    context->RSGetState(&state->rasterizerState);
    state->viewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    context->RSGetViewports(&state->viewportCount, state->viewports);
    context->IAGetInputLayout(&state->inputLayout);
    context->IAGetPrimitiveTopology(&state->topology);
    context->IAGetVertexBuffers(0, 1, &state->vertexBuffer, &state->vertexStride, &state->vertexOffset);
    context->VSGetShader(&state->vertexShader, NULL, NULL);
    context->HSGetShader(&state->hullShader, NULL, NULL);
    context->DSGetShader(&state->domainShader, NULL, NULL);
    context->GSGetShader(&state->geometryShader, NULL, NULL);
    context->PSGetShader(&state->pixelShader, NULL, NULL);
    context->VSGetConstantBuffers(0, 1, &state->vertexConstants);
    context->PSGetShaderResources(0, 1, &state->pixelResource);
    context->PSGetSamplers(0, 1, &state->pixelSampler);
    context->GetPredication(&state->predicate, &state->predicateValue);
}

static void RestoreD3D11State(ID3D11DeviceContext* context, D3D11StateBackup* state)
{
    context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, state->renderTargets, state->depthView);
    context->OMSetBlendState(state->blendState, state->blendFactor, state->sampleMask);
    context->OMSetDepthStencilState(state->depthState, state->stencilRef);
    context->RSSetState(state->rasterizerState);
    context->RSSetViewports(state->viewportCount, state->viewports);
    context->IASetInputLayout(state->inputLayout);
    context->IASetPrimitiveTopology(state->topology);
    context->IASetVertexBuffers(0, 1, &state->vertexBuffer, &state->vertexStride, &state->vertexOffset);
    context->VSSetShader(state->vertexShader, NULL, 0);
    context->HSSetShader(state->hullShader, NULL, 0);
    context->DSSetShader(state->domainShader, NULL, 0);
    context->GSSetShader(state->geometryShader, NULL, 0);
    context->PSSetShader(state->pixelShader, NULL, 0);
    context->VSSetConstantBuffers(0, 1, &state->vertexConstants);
    context->PSSetShaderResources(0, 1, &state->pixelResource);
    context->PSSetSamplers(0, 1, &state->pixelSampler);
    context->SetPredication(state->predicate, state->predicateValue);

    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) SafeRelease(&state->renderTargets[i]);
    SafeRelease(&state->depthView);
    SafeRelease(&state->blendState);
    SafeRelease(&state->depthState);
    SafeRelease(&state->rasterizerState);
    SafeRelease(&state->inputLayout);
    SafeRelease(&state->vertexBuffer);
    SafeRelease(&state->vertexShader);
    SafeRelease(&state->hullShader);
    SafeRelease(&state->domainShader);
    SafeRelease(&state->geometryShader);
    SafeRelease(&state->pixelShader);
    SafeRelease(&state->vertexConstants);
    SafeRelease(&state->pixelResource);
    SafeRelease(&state->pixelSampler);
    SafeRelease(&state->predicate);
}

static void EnsureOverlayLock()
{
    if (InterlockedCompareExchange(&g_overlayLockReady, 1, 0) == 0)
    {
        InitializeCriticalSection(&g_overlayLock);
    }
}

static void EnsureRendererLock()
{
    if (InterlockedCompareExchange(&g_rendererLockReady, 1, 0) == 0)
    {
        InitializeCriticalSection(&g_rendererLock);
    }
}

static void WriteProxyLog(const wchar_t* message)
{
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    while (length > 0 && path[length - 1] != L'\\')
    {
        --length;
    }
    path[length] = L'\0';
    lstrcatW(path, L"RROML_proxy_error.txt");

    HANDLE file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetFilePointer(file, 0, NULL, FILE_END);
    DWORD ignored = 0;
    WriteFile(file, message, lstrlenW(message) * sizeof(wchar_t), &ignored, NULL);
    WriteFile(file, L"\r\n", 4, &ignored, NULL);
    CloseHandle(file);
}

static void WriteProxyHresult(const wchar_t* operation, HRESULT hr)
{
    wchar_t message[256];
    StringCchPrintfW(message, 256, L"%s failed with HRESULT 0x%08X.", operation, (unsigned int)hr);
    WriteProxyLog(message);
}

static void GetManagedAssemblyPath(wchar_t* buffer)
{
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    while (lstrlenW(buffer) > 0 && buffer[lstrlenW(buffer) - 1] != L'\\')
    {
        buffer[lstrlenW(buffer) - 1] = L'\0';
    }

    lstrcatW(buffer, L"..\\..\\..\\RROML\\RROML.Core.dll");
}

static void LoadRealXInput()
{
    if (g_realXInput != NULL)
    {
        return;
    }

    wchar_t systemDirectory[MAX_PATH];
    GetSystemDirectoryW(systemDirectory, MAX_PATH);
    lstrcatW(systemDirectory, L"\\XINPUT1_3.dll");
    g_realXInput = LoadLibraryW(systemDirectory);

    if (g_realXInput == NULL)
    {
        WriteProxyLog(L"Could not load the real System32 XINPUT1_3.dll");
        return;
    }

    g_xinputGetState = (XInputGetStateFn)GetProcAddress(g_realXInput, "XInputGetState");
    g_xinputSetState = (XInputSetStateFn)GetProcAddress(g_realXInput, "XInputSetState");
    g_xinputGetCapabilities = (XInputGetCapabilitiesFn)GetProcAddress(g_realXInput, "XInputGetCapabilities");
    g_xinputGetDSoundAudioDeviceGuids = (XInputGetDSoundAudioDeviceGuidsFn)GetProcAddress(g_realXInput, "XInputGetDSoundAudioDeviceGuids");
    g_xinputGetBatteryInformation = (XInputGetBatteryInformationFn)GetProcAddress(g_realXInput, "XInputGetBatteryInformation");
    g_xinputGetKeystroke = (XInputGetKeystrokeFn)GetProcAddress(g_realXInput, "XInputGetKeystroke");
    g_xinputEnable = (XInputEnableFn)GetProcAddress(g_realXInput, "XInputEnable");
    g_xinputGetStateEx = (XInputGetStateExFn)GetProcAddress(g_realXInput, (LPCSTR)100);
    g_xinputOrdinal101 = (XInputUnusedOrdinalFn)GetProcAddress(g_realXInput, (LPCSTR)101);
    g_xinputOrdinal102 = (XInputUnusedOrdinalFn)GetProcAddress(g_realXInput, (LPCSTR)102);
    g_xinputOrdinal103 = (XInputUnusedOrdinalFn)GetProcAddress(g_realXInput, (LPCSTR)103);
}

static void ResetRendererResources()
{
    for (UINT i = 0; i < 8; ++i)
    {
        SafeRelease(&g_renderer.d2dBitmaps[i]);
        SafeRelease(&g_renderer.wrappedBackBuffers[i]);
    }

    SafeRelease(&g_renderer.d2dBrush);
    SafeRelease(&g_renderer.d2dContext);
    SafeRelease(&g_renderer.d2dDevice);
    SafeRelease(&g_renderer.swapChain3);
    SafeRelease(&g_renderer.d3d11On12);
    SafeRelease(&g_renderer.d3d12Queue);
    SafeRelease(&g_renderer.d3d12Device);
    SafeRelease(&g_renderer.d3d11Context);
    SafeRelease(&g_renderer.overlayTexture);
    SafeRelease(&g_renderer.overlayTextureView);
    SafeRelease(&g_renderer.overlayD2dTarget);
    SafeRelease(&g_renderer.overlayBgBrush);
    SafeRelease(&g_renderer.overlayTextBrush);
    SafeRelease(&g_renderer.overlayVertexShader);
    SafeRelease(&g_renderer.overlayPixelShader);
    SafeRelease(&g_renderer.overlayInputLayout);
    SafeRelease(&g_renderer.overlayVertexBuffer);
    SafeRelease(&g_renderer.overlayConstants);
    SafeRelease(&g_renderer.overlaySampler);
    SafeRelease(&g_renderer.overlayBlendState);
    SafeRelease(&g_renderer.overlayRasterizerState);
    SafeRelease(&g_renderer.d3d11Device);
    SafeRelease(&g_renderer.textFormat);
    SafeRelease(&g_renderer.writeFactory);
    SafeRelease(&g_renderer.d2dFactory);
    SafeRelease(&g_renderer.swapChain);
    SafeRelease(&g_renderer.deviceArg);
    g_renderer.backend = OverlayBackend_None;
    g_renderer.bufferCount = 0;
}

static bool EnsureFactories()
{
    if (g_renderer.d2dFactory == NULL)
    {
        D2D1_FACTORY_OPTIONS options = {};
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&g_renderer.d2dFactory);
        if (FAILED(hr) || g_renderer.d2dFactory == NULL)
        {
            WriteProxyLog(L"D2D1CreateFactory failed.");
            return false;
        }
    }

    if (g_renderer.writeFactory == NULL)
    {
        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_renderer.writeFactory);
        if (FAILED(hr) || g_renderer.writeFactory == NULL)
        {
            WriteProxyLog(L"DWriteCreateFactory failed.");
            return false;
        }
    }

    if (g_renderer.textFormat == NULL)
    {
        HRESULT hr = g_renderer.writeFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.0f, L"en-us", &g_renderer.textFormat);
        if (FAILED(hr) || g_renderer.textFormat == NULL)
        {
            WriteProxyLog(L"CreateTextFormat failed.");
            return false;
        }

        g_renderer.textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        g_renderer.textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return true;
}

static bool ResolveOverlayState(wchar_t* text, int* statusKind, int* position)
{
    EnsureOverlayLock();

    EnterCriticalSection(&g_overlayLock);
    if (g_overlayHideTick != 0 && GetTickCount() > g_overlayHideTick)
    {
        g_overlayText[0] = L'\0';
        g_overlayHideTick = 0;
    }

    StringCchCopyW(text, 256, g_overlayText);
    *statusKind = g_overlayStatusKind;
    *position = g_overlayPosition;
    LeaveCriticalSection(&g_overlayLock);
    if (text[0] != L'\0' && InterlockedCompareExchange(&g_overlayStateLogged, 1, 0) == 0)
    {
        WriteProxyLog(L"DXGI Present observed active RROML overlay state.");
    }
    return text[0] != L'\0';
}

static void FillOverlayRect(float width, float* left, float* top, float* right, float* bottom, int position)
{
    *left = 20.0f;
    *top = 20.0f;
    *right = 500.0f;
    *bottom = 64.0f;
    if (position == 1)
    {
        *right = width - 20.0f;
        *left = *right - 480.0f;
    }
}
static bool EnsureDX11OverlayResources(IDXGISwapChain* swapChain, UINT width, UINT height)
{
    if (g_renderer.overlayTexture != NULL)
    {
        return g_renderer.overlayD2dTarget != NULL && g_renderer.overlayBgBrush != NULL && g_renderer.overlayTextBrush != NULL &&
            g_renderer.overlayInputLayout != NULL && g_renderer.overlayVertexBuffer != NULL && g_renderer.overlayConstants != NULL &&
            g_renderer.overlayVertexShader != NULL && g_renderer.overlayPixelShader != NULL && g_renderer.overlaySampler != NULL &&
            g_renderer.overlayBlendState != NULL && g_renderer.overlayRasterizerState != NULL;
    }

    if (!EnsureFactories()) return false;

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = 500;
    textureDesc.Height = 64;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = g_renderer.d3d11Device->CreateTexture2D(&textureDesc, NULL, &g_renderer.overlayTexture);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay texture", hr); return false; }
    hr = g_renderer.d3d11Device->CreateShaderResourceView(g_renderer.overlayTexture, NULL, &g_renderer.overlayTextureView);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay texture view", hr); return false; }

    IDXGISurface* surface = NULL;
    hr = g_renderer.overlayTexture->QueryInterface(__uuidof(IDXGISurface), (void**)&surface);
    if (FAILED(hr)) { WriteProxyHresult(L"Get overlay texture surface", hr); return false; }
    D2D1_RENDER_TARGET_PROPERTIES targetProperties = {};
    targetProperties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    targetProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    targetProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    targetProperties.dpiX = 96.0f;
    targetProperties.dpiY = 96.0f;
    ID2D1RenderTarget* target = NULL;
    hr = g_renderer.d2dFactory->CreateDxgiSurfaceRenderTarget(surface, &targetProperties, &target);
    surface->Release();
    if (FAILED(hr)) { WriteProxyHresult(L"Create compatible overlay render target", hr); return false; }
    g_renderer.overlayD2dTarget = target;
    g_renderer.overlayD2dTarget->CreateSolidColorBrush(D2D1::ColorF(0.10f, 0.10f, 0.10f, 0.82f), &g_renderer.overlayBgBrush);
    g_renderer.overlayD2dTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &g_renderer.overlayTextBrush);
    if (g_renderer.overlayBgBrush == NULL || g_renderer.overlayTextBrush == NULL)
    {
        WriteProxyLog(L"Could not create overlay text brushes.");
        return false;
    }

    static const char* shader =
        "struct V { float2 p : POSITION; float2 uv : TEXCOORD0; };"
        "struct O { float4 p : SV_POSITION; float2 uv : TEXCOORD0; };"
        "cbuffer C : register(b0) { float4 rect; float2 screen; float2 pad; };"
        "O vs(V i) { O o; float2 pixel=rect.xy+i.p*rect.zw; o.p=float4(pixel.x/screen.x*2-1, 1-pixel.y/screen.y*2, 0, 1); o.uv=i.uv; return o; }"
        "Texture2D t : register(t0); SamplerState s : register(s0);"
        "float4 ps(O i) : SV_TARGET { return t.Sample(s, i.uv); }";
    ID3DBlob* vsCode = NULL;
    ID3DBlob* psCode = NULL;
    ID3DBlob* errors = NULL;
    if (g_d3dCompile == NULL)
    {
        g_d3dCompiler = LoadLibraryW(L"D3DCOMPILER_47.dll");
        if (g_d3dCompiler != NULL) g_d3dCompile = (D3DCompileFn)GetProcAddress(g_d3dCompiler, "D3DCompile");
    }
    if (g_d3dCompile == NULL) { WriteProxyLog(L"The Windows Direct3D shader compiler is unavailable."); return false; }
    hr = g_d3dCompile(shader, strlen(shader), NULL, NULL, NULL, "vs", "vs_4_0", 0, 0, &vsCode, &errors);
    if (FAILED(hr)) { if (errors) errors->Release(); WriteProxyHresult(L"Compile overlay vertex shader", hr); return false; }
    hr = g_d3dCompile(shader, strlen(shader), NULL, NULL, NULL, "ps", "ps_4_0", 0, 0, &psCode, &errors);
    if (FAILED(hr)) { SafeRelease(&vsCode); if (errors) errors->Release(); WriteProxyHresult(L"Compile overlay pixel shader", hr); return false; }
    hr = g_renderer.d3d11Device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), NULL, &g_renderer.overlayVertexShader);
    if (SUCCEEDED(hr)) hr = g_renderer.d3d11Device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(), NULL, &g_renderer.overlayPixelShader);
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    if (SUCCEEDED(hr)) hr = g_renderer.d3d11Device->CreateInputLayout(layout, 2, vsCode->GetBufferPointer(), vsCode->GetBufferSize(), &g_renderer.overlayInputLayout);
    SafeRelease(&vsCode);
    SafeRelease(&psCode);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay shaders", hr); return false; }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = 6 * sizeof(float) * 4;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = g_renderer.d3d11Device->CreateBuffer(&bufferDesc, NULL, &g_renderer.overlayVertexBuffer);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay vertex buffer", hr); return false; }

    D3D11_BUFFER_DESC constantsDesc = {};
    constantsDesc.ByteWidth = 32;
    constantsDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = g_renderer.d3d11Device->CreateBuffer(&constantsDesc, NULL, &g_renderer.overlayConstants);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay constants", hr); return false; }

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_renderer.d3d11Device->CreateSamplerState(&samplerDesc, &g_renderer.overlaySampler);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay sampler", hr); return false; }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = g_renderer.d3d11Device->CreateBlendState(&blendDesc, &g_renderer.overlayBlendState);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay blend state", hr); return false; }

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    hr = g_renderer.d3d11Device->CreateRasterizerState(&rasterDesc, &g_renderer.overlayRasterizerState);
    if (FAILED(hr)) { WriteProxyHresult(L"Create overlay rasterizer", hr); return false; }

    (void)swapChain;
    (void)width;
    (void)height;
    return true;
}

static bool RenderOverlayDX11(IDXGISwapChain* swapChain)
{
    wchar_t text[256];
    int statusKind = 0;
    int position = 0;
    if (!ResolveOverlayState(text, &statusKind, &position)) return true;

    DXGI_SWAP_CHAIN_DESC desc = {};
    if (FAILED(swapChain->GetDesc(&desc))) return false;
    UINT bufferIndex = 0;
    if (g_renderer.swapChain3 == NULL)
    {
        swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&g_renderer.swapChain3);
    }
    if (g_renderer.swapChain3 != NULL) bufferIndex = g_renderer.swapChain3->GetCurrentBackBufferIndex();
    ID3D11Texture2D* backBuffer = NULL;
    HRESULT hr = swapChain->GetBuffer(bufferIndex, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || backBuffer == NULL) { WriteProxyHresult(L"Get current swap-chain buffer", hr); return false; }
    D3D11_TEXTURE2D_DESC backBufferDesc = {};
    backBuffer->GetDesc(&backBufferDesc);
    if (!EnsureDX11OverlayResources(swapChain, backBufferDesc.Width, backBufferDesc.Height)) { backBuffer->Release(); return false; }
    ID3D11RenderTargetView* targetView = NULL;
    hr = g_renderer.d3d11Device->CreateRenderTargetView(backBuffer, NULL, &targetView);
    backBuffer->Release();
    if (FAILED(hr)) { WriteProxyHresult(L"Create current backbuffer view", hr); return false; }

    D3D11StateBackup previousState = {};
    CaptureD3D11State(g_renderer.d3d11Context, &previousState);
    g_renderer.d3d11Context->SetPredication(NULL, FALSE);

    D2D1_COLOR_F textColor = D2D1::ColorF(0.839f, 0.431f, 0.133f, 1.0f);
    if (statusKind == 1) textColor = D2D1::ColorF(0.439f, 0.804f, 0.486f, 1.0f);
    else if (statusKind == 2) textColor = D2D1::ColorF(0.788f, 0.329f, 0.329f, 1.0f);
    else if (statusKind == 3) textColor = D2D1::ColorF(0.910f, 0.773f, 0.282f, 1.0f);
    g_renderer.overlayTextBrush->SetColor(textColor);
    g_renderer.overlayD2dTarget->BeginDraw();
    g_renderer.overlayD2dTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
    D2D1_RECT_F box = D2D1::RectF(0, 0, 500, 64);
    D2D1_RECT_F label = D2D1::RectF(14, 0, 486, 64);
    g_renderer.overlayD2dTarget->FillRectangle(box, g_renderer.overlayBgBrush);
    g_renderer.overlayD2dTarget->DrawText(text, (UINT32)lstrlenW(text), g_renderer.textFormat, label, g_renderer.overlayTextBrush);
    hr = g_renderer.overlayD2dTarget->EndDraw();
    if (FAILED(hr))
    {
        WriteProxyHresult(L"Draw overlay text", hr);
        RestoreD3D11State(g_renderer.d3d11Context, &previousState);
        targetView->Release();
        return false;
    }

    struct OverlayVertex { float x, y, u, v; };
    float left = position == 1 ? (float)backBufferDesc.Width - 520.0f : 20.0f;
    float top = 20.0f;
    OverlayVertex vertices[6] = {
        { 0, 0, 0, 0 }, { 1, 0, 1, 0 }, { 0, 1, 0, 1 },
        { 0, 1, 0, 1 }, { 1, 0, 1, 0 }, { 1, 1, 1, 1 }
    };
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = g_renderer.d3d11Context->Map(g_renderer.overlayVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, vertices, sizeof(vertices));
        g_renderer.d3d11Context->Unmap(g_renderer.overlayVertexBuffer, 0);
        float screenConstants[8] = { left, top, 500.0f, 64.0f, (float)backBufferDesc.Width, (float)backBufferDesc.Height, 0, 0 };
        if (SUCCEEDED(g_renderer.d3d11Context->Map(g_renderer.overlayConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, screenConstants, sizeof(screenConstants));
            g_renderer.d3d11Context->Unmap(g_renderer.overlayConstants, 0);
        }
        UINT stride = sizeof(OverlayVertex), offset = 0;
        g_renderer.d3d11Context->OMSetRenderTargets(1, &targetView, NULL);
        D3D11_VIEWPORT viewport = { 0, 0, (float)backBufferDesc.Width, (float)backBufferDesc.Height, 0, 1 };
        g_renderer.d3d11Context->RSSetViewports(1, &viewport);
        g_renderer.d3d11Context->RSSetState(g_renderer.overlayRasterizerState);
        g_renderer.d3d11Context->OMSetBlendState(g_renderer.overlayBlendState, NULL, 0xffffffff);
        g_renderer.d3d11Context->IASetInputLayout(g_renderer.overlayInputLayout);
        g_renderer.d3d11Context->IASetVertexBuffers(0, 1, &g_renderer.overlayVertexBuffer, &stride, &offset);
        g_renderer.d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_renderer.d3d11Context->VSSetShader(g_renderer.overlayVertexShader, NULL, 0);
        g_renderer.d3d11Context->VSSetConstantBuffers(0, 1, &g_renderer.overlayConstants);
        g_renderer.d3d11Context->PSSetShader(g_renderer.overlayPixelShader, NULL, 0);
        g_renderer.d3d11Context->PSSetShaderResources(0, 1, &g_renderer.overlayTextureView);
        g_renderer.d3d11Context->PSSetSamplers(0, 1, &g_renderer.overlaySampler);
        g_renderer.d3d11Context->Draw(6, 0);
        ID3D11ShaderResourceView* emptyView = NULL;
        g_renderer.d3d11Context->PSSetShaderResources(0, 1, &emptyView);
    }
    else
    {
        WriteProxyHresult(L"Map overlay vertex buffer", hr);
    }

    RestoreD3D11State(g_renderer.d3d11Context, &previousState);
    targetView->Release();
    return SUCCEEDED(hr);
}

static bool EnsureDx12Renderer(IDXGISwapChain* swapChain)
{
    if (g_renderer.d3d11On12 != NULL && g_renderer.swapChain3 != NULL)
    {
        return true;
    }

    if (!EnsureFactories() || g_renderer.deviceArg == NULL)
    {
        return false;
    }

    if (FAILED(g_renderer.deviceArg->QueryInterface(__uuidof(ID3D12CommandQueue), (void**)&g_renderer.d3d12Queue)) || g_renderer.d3d12Queue == NULL)
    {
        WriteProxyLog(L"Could not query ID3D12CommandQueue.");
        return false;
    }

    if (FAILED(g_renderer.d3d12Queue->GetDevice(__uuidof(ID3D12Device), (void**)&g_renderer.d3d12Device)) || g_renderer.d3d12Device == NULL)
    {
        WriteProxyLog(L"Could not query ID3D12Device from command queue.");
        return false;
    }

    HMODULE d3d11Module = LoadLibraryW(L"d3d11.dll");
    if (d3d11Module == NULL)
    {
        WriteProxyLog(L"Could not load d3d11.dll for D3D11On12CreateDevice.");
        return false;
    }

    D3D11On12CreateDeviceFn createOn12 = (D3D11On12CreateDeviceFn)GetProcAddress(d3d11Module, "D3D11On12CreateDevice");
    if (createOn12 == NULL)
    {
        WriteProxyLog(L"D3D11On12CreateDevice export was not found.");
        return false;
    }

    IUnknown* queues[] = { g_renderer.d3d12Queue };
    D3D_FEATURE_LEVEL createdLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = createOn12(g_renderer.d3d12Device, D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, queues, 1, 0, &g_renderer.d3d11Device, &g_renderer.d3d11Context, &createdLevel);
    if (FAILED(hr) || g_renderer.d3d11Device == NULL || g_renderer.d3d11Context == NULL)
    {
        WriteProxyLog(L"D3D11On12CreateDevice failed.");
        return false;
    }

    hr = g_renderer.d3d11Device->QueryInterface(__uuidof(ID3D11On12Device), (void**)&g_renderer.d3d11On12);
    if (FAILED(hr) || g_renderer.d3d11On12 == NULL)
    {
        WriteProxyLog(L"Could not query ID3D11On12Device.");
        return false;
    }

    IDXGIDevice* dxgiDevice = NULL;
    hr = g_renderer.d3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr) || dxgiDevice == NULL)
    {
        WriteProxyLog(L"Could not query IDXGIDevice from D3D11On12 device.");
        return false;
    }

    hr = g_renderer.d2dFactory->CreateDevice(dxgiDevice, &g_renderer.d2dDevice);
    SafeRelease(&dxgiDevice);
    if (FAILED(hr) || g_renderer.d2dDevice == NULL)
    {
        WriteProxyLog(L"D2D CreateDevice failed for D3D12 path.");
        return false;
    }

    hr = g_renderer.d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_renderer.d2dContext);
    if (FAILED(hr) || g_renderer.d2dContext == NULL)
    {
        WriteProxyLog(L"D2D CreateDeviceContext failed for D3D12 path.");
        return false;
    }

    g_renderer.d2dContext->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &g_renderer.d2dBrush);
    hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&g_renderer.swapChain3);
    if (FAILED(hr) || g_renderer.swapChain3 == NULL)
    {
        WriteProxyLog(L"Could not query IDXGISwapChain3 for D3D12 path.");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc = {};
    hr = swapChain->GetDesc(&desc);
    if (FAILED(hr))
    {
        WriteProxyLog(L"GetDesc failed for D3D12 swap chain.");
        return false;
    }

    g_renderer.bufferCount = desc.BufferCount;
    if (g_renderer.bufferCount == 0 || g_renderer.bufferCount > 8)
    {
        g_renderer.bufferCount = 3;
    }

    FLOAT dpiX = 96.0f;
    FLOAT dpiY = 96.0f;
    g_renderer.d2dFactory->GetDesktopDpi(&dpiX, &dpiY);

    for (UINT i = 0; i < g_renderer.bufferCount; ++i)
    {
        ID3D12Resource* backBuffer = NULL;
        hr = g_renderer.swapChain3->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&backBuffer);
        if (FAILED(hr) || backBuffer == NULL)
        {
            return false;
        }

        D3D11_RESOURCE_FLAGS flags = {};
        flags.BindFlags = D3D11_BIND_RENDER_TARGET;
        hr = g_renderer.d3d11On12->CreateWrappedResource(backBuffer, &flags, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT, __uuidof(ID3D11Resource), (void**)&g_renderer.wrappedBackBuffers[i]);
        SafeRelease(&backBuffer);
        if (FAILED(hr) || g_renderer.wrappedBackBuffers[i] == NULL)
        {
            WriteProxyLog(L"CreateWrappedResource failed.");
            return false;
        }

        IDXGISurface* surface = NULL;
        hr = g_renderer.wrappedBackBuffers[i]->QueryInterface(__uuidof(IDXGISurface), (void**)&surface);
        if (FAILED(hr) || surface == NULL)
        {
            WriteProxyLog(L"QueryInterface IDXGISurface failed for wrapped resource.");
            return false;
        }

        D2D1_BITMAP_PROPERTIES1 props = {};
        props.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        props.dpiX = dpiX;
        props.dpiY = dpiY;
        props.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        hr = g_renderer.d2dContext->CreateBitmapFromDxgiSurface(surface, &props, &g_renderer.d2dBitmaps[i]);
        SafeRelease(&surface);
        if (FAILED(hr) || g_renderer.d2dBitmaps[i] == NULL)
        {
            WriteProxyLog(L"CreateBitmapFromDxgiSurface failed for D3D12 path.");
            return false;
        }
    }

    return true;
}
static bool RenderOverlayDX12(IDXGISwapChain* swapChain)
{
    wchar_t text[256];
    int statusKind = 0;
    int position = 0;
    if (!ResolveOverlayState(text, &statusKind, &position))
    {
        return true;
    }

    if (!EnsureDx12Renderer(swapChain))
    {
        return false;
    }

    UINT index = g_renderer.swapChain3->GetCurrentBackBufferIndex();
    if (index >= g_renderer.bufferCount || g_renderer.wrappedBackBuffers[index] == NULL || g_renderer.d2dBitmaps[index] == NULL)
    {
        return false;
    }

    ID3D11Resource* wrapped[] = { g_renderer.wrappedBackBuffers[index] };
    g_renderer.d3d11On12->AcquireWrappedResources(wrapped, 1);
    g_renderer.d2dContext->SetTarget(g_renderer.d2dBitmaps[index]);
    g_renderer.d2dContext->BeginDraw();

    D2D1_COLOR_F textColor = D2D1::ColorF(0.839f, 0.431f, 0.133f, 1.0f);
    if (statusKind == 2)
    {
        textColor = D2D1::ColorF(0.788f, 0.329f, 0.329f, 1.0f);
    }
    else if (statusKind == 3)
    {
        textColor = D2D1::ColorF(0.910f, 0.773f, 0.282f, 1.0f);
    }

    if (g_renderer.d2dBrush != NULL)
    {
        g_renderer.d2dBrush->SetColor(textColor);
    }

    D2D1_SIZE_F size = g_renderer.d2dContext->GetSize();
    float left, top, right, bottom;
    FillOverlayRect(size.width, &left, &top, &right, &bottom, position);
    D2D1_RECT_F boxRect = D2D1::RectF(left, top, right, bottom);
    D2D1_RECT_F textRect = D2D1::RectF(left + 14.0f, top, right - 14.0f, bottom);

    ID2D1SolidColorBrush* bgBrush = NULL;
    g_renderer.d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.10f, 0.10f, 0.10f, 0.82f), &bgBrush);
    if (bgBrush != NULL)
    {
        g_renderer.d2dContext->FillRectangle(boxRect, bgBrush);
    }
    if (g_renderer.d2dBrush != NULL)
    {
        g_renderer.d2dContext->DrawText(text, (UINT32)lstrlenW(text), g_renderer.textFormat, textRect, g_renderer.d2dBrush);
    }

    HRESULT hr = g_renderer.d2dContext->EndDraw();
    SafeRelease(&bgBrush);
    g_renderer.d2dContext->SetTarget(NULL);
    g_renderer.d3d11On12->ReleaseWrappedResources(wrapped, 1);
    g_renderer.d3d11Context->Flush();
    return SUCCEEDED(hr);
}

static void RenderOverlayIfPossible(IDXGISwapChain* swapChain)
{
    EnsureRendererLock();
    EnterCriticalSection(&g_rendererLock);
    if (g_renderer.swapChain == NULL || g_renderer.swapChain != swapChain)
    {
        LeaveCriticalSection(&g_rendererLock);
        return;
    }

    if (g_renderer.backend == OverlayBackend_DX11)
    {
        if (!RenderOverlayDX11(swapChain) && InterlockedCompareExchange(&g_overlayRenderFailureLogged, 1, 0) == 0)
        {
            WriteProxyLog(L"DX11 in-game overlay rendering failed.");
        }
    }
    else if (g_renderer.backend == OverlayBackend_DX12)
    {
        if (!RenderOverlayDX12(swapChain) && InterlockedCompareExchange(&g_overlayRenderFailureLogged, 1, 0) == 0)
        {
            WriteProxyLog(L"DX12 overlay rendering failed.");
        }
    }
    LeaveCriticalSection(&g_rendererLock);
}

static void InitializeSwapChainRenderer(IDXGISwapChain* swapChain, IUnknown* deviceArg)
{
    if (swapChain == NULL)
    {
        return;
    }

    EnsureRendererLock();
    EnterCriticalSection(&g_rendererLock);
    if (g_renderer.swapChain == swapChain)
    {
        LeaveCriticalSection(&g_rendererLock);
        return;
    }

    ResetRendererResources();
    g_renderer.swapChain = swapChain;
    g_renderer.swapChain->AddRef();
    g_renderer.deviceArg = deviceArg;
    if (g_renderer.deviceArg != NULL)
    {
        g_renderer.deviceArg->AddRef();
    }

    ID3D11Device* probe11 = NULL;
    if (deviceArg != NULL && SUCCEEDED(deviceArg->QueryInterface(__uuidof(ID3D11Device), (void**)&probe11)) && probe11 != NULL)
    {
        g_renderer.backend = OverlayBackend_DX11;
        g_renderer.d3d11Device = probe11;
        g_renderer.d3d11Device->GetImmediateContext(&g_renderer.d3d11Context);
        WriteProxyLog(L"RROML overlay detected DX11.");
        LeaveCriticalSection(&g_rendererLock);
        return;
    }
    SafeRelease(&probe11);

    ID3D12CommandQueue* probeQueue = NULL;
    if (deviceArg != NULL && SUCCEEDED(deviceArg->QueryInterface(__uuidof(ID3D12CommandQueue), (void**)&probeQueue)) && probeQueue != NULL)
    {
        g_renderer.backend = OverlayBackend_DX12;
        g_renderer.d3d12Queue = probeQueue;
        g_renderer.d3d12Queue->GetDevice(__uuidof(ID3D12Device), (void**)&g_renderer.d3d12Device);
        WriteProxyLog(L"RROML overlay detected DX12.");
        LeaveCriticalSection(&g_rendererLock);
        return;
    }
    SafeRelease(&probeQueue);

    WriteProxyLog(L"RROML overlay could not determine DX11 or DX12 backend.");
    LeaveCriticalSection(&g_rendererLock);
}

static void PatchVTableEntry(void** vtable, int index, void* replacement, void** original)
{
    if (*original == NULL)
    {
        *original = vtable[index];
    }

    if (vtable[index] == replacement)
    {
        return;
    }

    DWORD oldProtect;
    if (VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        vtable[index] = replacement;
        VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect);
    }
}

static HRESULT STDMETHODCALLTYPE Hook_SwapChainPresent(IDXGISwapChain* self, UINT syncInterval, UINT flags)
{
    if (InterlockedCompareExchange(&g_presentHookLogged, 1, 0) == 0)
    {
        WriteProxyLog(L"DXGI swap-chain Present hook reached.");
    }
    RenderOverlayIfPossible(self);
    return g_realSwapChainPresent != NULL ? g_realSwapChainPresent(self, syncInterval, flags) : S_OK;
}

static HRESULT STDMETHODCALLTYPE Hook_SwapChainPresent1(IDXGISwapChain1* self, UINT syncInterval, UINT flags, const DXGI_PRESENT_PARAMETERS* parameters)
{
    if (InterlockedCompareExchange(&g_presentHookLogged, 1, 0) == 0)
    {
        WriteProxyLog(L"DXGI swap-chain Present1 hook reached.");
    }
    RenderOverlayIfPossible(self);
    return g_realSwapChainPresent1 != NULL ? g_realSwapChainPresent1(self, syncInterval, flags, parameters) : S_OK;
}

static void PatchSwapChain(IDXGISwapChain* swapChain, IUnknown* deviceArg)
{
    if (swapChain == NULL)
    {
        return;
    }

    InitializeSwapChainRenderer(swapChain, deviceArg);
    void** vtable = *(void***)swapChain;
    PatchVTableEntry(vtable, 8, (void*)Hook_SwapChainPresent, (void**)&g_realSwapChainPresent);

    IDXGISwapChain1* swapChain1 = NULL;
    if (SUCCEEDED(swapChain->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&swapChain1)) && swapChain1 != NULL)
    {
        void** vtable1 = *(void***)swapChain1;
        PatchVTableEntry(vtable1, 22, (void*)Hook_SwapChainPresent1, (void**)&g_realSwapChainPresent1);
        swapChain1->Release();
    }
}

static HRESULT STDMETHODCALLTYPE Hook_FactoryCreateSwapChain(IDXGIFactory* self, IUnknown* device, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** swapChain)
{
    HRESULT hr = g_realFactoryCreateSwapChain != NULL ? g_realFactoryCreateSwapChain(self, device, desc, swapChain) : E_FAIL;
    if (SUCCEEDED(hr) && swapChain != NULL)
    {
        PatchSwapChain(*swapChain, device);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Hook_FactoryCreateSwapChainForHwnd(IDXGIFactory2* self, IUnknown* device, HWND windowHandle, const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullScreenDesc, IDXGIOutput* restrictToOutput, IDXGISwapChain1** swapChain)
{
    HRESULT hr = g_realFactoryCreateSwapChainForHwnd != NULL ? g_realFactoryCreateSwapChainForHwnd(self, device, windowHandle, desc, fullScreenDesc, restrictToOutput, swapChain) : E_FAIL;
    if (SUCCEEDED(hr) && swapChain != NULL)
    {
        PatchSwapChain(*swapChain, device);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Hook_FactoryCreateSwapChainForComposition(IDXGIFactory2* self, IUnknown* device, const DXGI_SWAP_CHAIN_DESC1* desc, IDXGIOutput* restrictToOutput, IDXGISwapChain1** swapChain)
{
    HRESULT hr = g_realFactoryCreateSwapChainForComposition != NULL ? g_realFactoryCreateSwapChainForComposition(self, device, desc, restrictToOutput, swapChain) : E_FAIL;
    if (SUCCEEDED(hr) && swapChain != NULL)
    {
        PatchSwapChain(*swapChain, device);
    }
    return hr;
}
static void PatchFactory(void* factoryPointer)
{
    if (factoryPointer == NULL)
    {
        return;
    }

    void** vtable = *(void***)factoryPointer;
    PatchVTableEntry(vtable, 10, (void*)Hook_FactoryCreateSwapChain, (void**)&g_realFactoryCreateSwapChain);
    PatchVTableEntry(vtable, 15, (void*)Hook_FactoryCreateSwapChainForHwnd, (void**)&g_realFactoryCreateSwapChainForHwnd);
    PatchVTableEntry(vtable, 24, (void*)Hook_FactoryCreateSwapChainForComposition, (void**)&g_realFactoryCreateSwapChainForComposition);
}

static HRESULT WINAPI Hook_CreateDXGIFactory(REFIID riid, void** factory)
{
    HRESULT hr = g_realCreateDXGIFactory != NULL ? g_realCreateDXGIFactory(riid, factory) : E_FAIL;
    if (SUCCEEDED(hr) && factory != NULL)
    {
        PatchFactory(*factory);
    }
    return hr;
}

static HRESULT WINAPI Hook_CreateDXGIFactory1(REFIID riid, void** factory)
{
    HRESULT hr = g_realCreateDXGIFactory1 != NULL ? g_realCreateDXGIFactory1(riid, factory) : E_FAIL;
    if (SUCCEEDED(hr) && factory != NULL)
    {
        PatchFactory(*factory);
    }
    return hr;
}

static HRESULT WINAPI Hook_CreateDXGIFactory2(UINT flags, REFIID riid, void** factory)
{
    HRESULT hr = g_realCreateDXGIFactory2 != NULL ? g_realCreateDXGIFactory2(flags, riid, factory) : E_FAIL;
    if (SUCCEEDED(hr) && factory != NULL)
    {
        PatchFactory(*factory);
    }
    return hr;
}

static bool PatchImportAddress(const char* dllName, const char* importName, void* replacement, void** original)
{
    HMODULE module = GetModuleHandleW(NULL);
    if (module == NULL)
    {
        return false;
    }

    BYTE* base = (BYTE*)module;
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(base + dosHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY importDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDirectory.VirtualAddress == 0)
    {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR* descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + importDirectory.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor)
    {
        const char* importedDll = (const char*)(base + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0)
        {
            continue;
        }

        IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)(base + descriptor->FirstThunk);
        IMAGE_THUNK_DATA* originalThunk = descriptor->OriginalFirstThunk != 0 ? (IMAGE_THUNK_DATA*)(base + descriptor->OriginalFirstThunk) : thunk;
        for (; originalThunk->u1.AddressOfData != 0; ++originalThunk, ++thunk)
        {
            if (IMAGE_SNAP_BY_ORDINAL(originalThunk->u1.Ordinal))
            {
                continue;
            }

            IMAGE_IMPORT_BY_NAME* importByName = (IMAGE_IMPORT_BY_NAME*)(base + originalThunk->u1.AddressOfData);
            if (strcmp((const char*)importByName->Name, importName) != 0)
            {
                continue;
            }

            if (*original == NULL)
            {
                *original = (void*)thunk->u1.Function;
            }

            DWORD oldProtect;
            if (VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &oldProtect))
            {
                thunk->u1.Function = (ULONG_PTR)replacement;
                VirtualProtect(&thunk->u1.Function, sizeof(void*), oldProtect, &oldProtect);
                return true;
            }
            return false;
        }
    }

    return false;
}

static void EnsureTimeLock()
{
    if (InterlockedCompareExchange(&g_timeLockReady, 1, 0) == 0)
    {
        InitializeCriticalSection(&g_timeLock);
    }
}

static double ClampTimeScale(double value)
{
    if (value < 0.0)
    {
        return 0.0;
    }

    if (value > 4.0)
    {
        return 4.0;
    }

    return value;
}

static BOOL ReadRealQpc(LARGE_INTEGER* value)
{
    if (g_realQueryPerformanceCounter != NULL)
    {
        return g_realQueryPerformanceCounter(value);
    }

    return QueryPerformanceCounter(value);
}

static BOOL ReadRealQpcFrequency(LARGE_INTEGER* value)
{
    if (g_realQueryPerformanceFrequency != NULL)
    {
        return g_realQueryPerformanceFrequency(value);
    }

    return QueryPerformanceFrequency(value);
}

static ULONGLONG ReadRealTick64()
{
    if (g_realGetTickCount64 != NULL)
    {
        return g_realGetTickCount64();
    }

    return GetTickCount64();
}

static DWORD ReadRealTick32()
{
    if (g_realGetTickCount != NULL)
    {
        return g_realGetTickCount();
    }

    return GetTickCount();
}

static DWORD ReadRealTimeGetTime()
{
    if (g_realTimeGetTime != NULL)
    {
        return g_realTimeGetTime();
    }

    return timeGetTime();
}

static void EnsureTimeStateInitializedLocked()
{
    if (g_timeStateInitialized)
    {
        return;
    }

    ReadRealQpcFrequency(&g_qpcFrequency);
    ReadRealQpc(&g_qpcBaseReal);
    g_qpcBaseVirtual = g_qpcBaseReal;
    g_tick64BaseReal = ReadRealTick64();
    g_tick64BaseVirtual = g_tick64BaseReal;
    g_tickBaseReal = ReadRealTick32();
    g_tickBaseVirtual = g_tickBaseReal;
    g_timeGetBaseReal = ReadRealTimeGetTime();
    g_timeGetBaseVirtual = g_timeGetBaseReal;
    g_timeScale = 1.0;
    g_timeStateInitialized = TRUE;
}

static LONGLONG ScaleQpcValueLocked(LARGE_INTEGER realValue)
{
    EnsureTimeStateInitializedLocked();
    double scaled = (double)(realValue.QuadPart - g_qpcBaseReal.QuadPart) * g_timeScale;
    return g_qpcBaseVirtual.QuadPart + (LONGLONG)(scaled + (scaled >= 0.0 ? 0.5 : -0.5));
}

static ULONGLONG ScaleTick64ValueLocked(ULONGLONG realValue)
{
    EnsureTimeStateInitializedLocked();
    double scaled = (double)(realValue - g_tick64BaseReal) * g_timeScale;
    return g_tick64BaseVirtual + (ULONGLONG)(scaled + 0.5);
}

static DWORD ScaleTick32ValueLocked(DWORD realValue)
{
    EnsureTimeStateInitializedLocked();
    DWORD delta = realValue - g_tickBaseReal;
    double scaled = (double)delta * g_timeScale;
    return g_tickBaseVirtual + (DWORD)(scaled + 0.5);
}

static DWORD ScaleTimeGetTimeValueLocked(DWORD realValue)
{
    EnsureTimeStateInitializedLocked();
    DWORD delta = realValue - g_timeGetBaseReal;
    double scaled = (double)delta * g_timeScale;
    return g_timeGetBaseVirtual + (DWORD)(scaled + 0.5);
}

static BOOL WINAPI Hook_QueryPerformanceCounter(LARGE_INTEGER* value)
{
    LARGE_INTEGER realValue = {};
    if (!ReadRealQpc(&realValue))
    {
        return FALSE;
    }

    EnsureTimeLock();
    EnterCriticalSection(&g_timeLock);
    LONGLONG scaledValue = ScaleQpcValueLocked(realValue);
    LeaveCriticalSection(&g_timeLock);

    if (value != NULL)
    {
        value->QuadPart = scaledValue;
    }

    return TRUE;
}

static ULONGLONG WINAPI Hook_GetTickCount64()
{
    ULONGLONG realValue = ReadRealTick64();
    EnsureTimeLock();
    EnterCriticalSection(&g_timeLock);
    ULONGLONG scaledValue = ScaleTick64ValueLocked(realValue);
    LeaveCriticalSection(&g_timeLock);
    return scaledValue;
}

static DWORD WINAPI Hook_GetTickCount()
{
    DWORD realValue = ReadRealTick32();
    EnsureTimeLock();
    EnterCriticalSection(&g_timeLock);
    DWORD scaledValue = ScaleTick32ValueLocked(realValue);
    LeaveCriticalSection(&g_timeLock);
    return scaledValue;
}

static DWORD WINAPI Hook_TimeGetTime()
{
    DWORD realValue = ReadRealTimeGetTime();
    EnsureTimeLock();
    EnterCriticalSection(&g_timeLock);
    DWORD scaledValue = ScaleTimeGetTimeValueLocked(realValue);
    LeaveCriticalSection(&g_timeLock);
    return scaledValue;
}

static void InstallTimeHooks()
{
    if (InterlockedCompareExchange(&g_timeHooksInstalled, 1, 0) != 0)
    {
        return;
    }

    EnsureTimeLock();

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE kernelBase = GetModuleHandleW(L"KernelBase.dll");
    HMODULE winmm = LoadLibraryW(L"winmm.dll");

    if (kernel32 != NULL)
    {
        if (g_realQueryPerformanceCounter == NULL)
        {
            g_realQueryPerformanceCounter = (QueryPerformanceCounterFn)GetProcAddress(kernel32, "QueryPerformanceCounter");
        }
        if (g_realQueryPerformanceFrequency == NULL)
        {
            g_realQueryPerformanceFrequency = (QueryPerformanceFrequencyFn)GetProcAddress(kernel32, "QueryPerformanceFrequency");
        }
        if (g_realGetTickCount64 == NULL)
        {
            g_realGetTickCount64 = (GetTickCount64Fn)GetProcAddress(kernel32, "GetTickCount64");
        }
        if (g_realGetTickCount == NULL)
        {
            g_realGetTickCount = (GetTickCountFn)GetProcAddress(kernel32, "GetTickCount");
        }
    }

    if (kernelBase != NULL)
    {
        if (g_realQueryPerformanceCounter == NULL)
        {
            g_realQueryPerformanceCounter = (QueryPerformanceCounterFn)GetProcAddress(kernelBase, "QueryPerformanceCounter");
        }
        if (g_realQueryPerformanceFrequency == NULL)
        {
            g_realQueryPerformanceFrequency = (QueryPerformanceFrequencyFn)GetProcAddress(kernelBase, "QueryPerformanceFrequency");
        }
        if (g_realGetTickCount64 == NULL)
        {
            g_realGetTickCount64 = (GetTickCount64Fn)GetProcAddress(kernelBase, "GetTickCount64");
        }
        if (g_realGetTickCount == NULL)
        {
            g_realGetTickCount = (GetTickCountFn)GetProcAddress(kernelBase, "GetTickCount");
        }
    }

    if (winmm != NULL && g_realTimeGetTime == NULL)
    {
        g_realTimeGetTime = (TimeGetTimeFn)GetProcAddress(winmm, "timeGetTime");
    }

    PatchImportAddress("kernel32.dll", "QueryPerformanceCounter", (void*)Hook_QueryPerformanceCounter, (void**)&g_realQueryPerformanceCounter);
    PatchImportAddress("kernel32.dll", "GetTickCount64", (void*)Hook_GetTickCount64, (void**)&g_realGetTickCount64);
    PatchImportAddress("kernel32.dll", "GetTickCount", (void*)Hook_GetTickCount, (void**)&g_realGetTickCount);
    PatchImportAddress("KernelBase.dll", "QueryPerformanceCounter", (void*)Hook_QueryPerformanceCounter, (void**)&g_realQueryPerformanceCounter);
    PatchImportAddress("KernelBase.dll", "GetTickCount64", (void*)Hook_GetTickCount64, (void**)&g_realGetTickCount64);
    PatchImportAddress("KernelBase.dll", "GetTickCount", (void*)Hook_GetTickCount, (void**)&g_realGetTickCount);
    PatchImportAddress("winmm.dll", "timeGetTime", (void*)Hook_TimeGetTime, (void**)&g_realTimeGetTime);

    EnterCriticalSection(&g_timeLock);
    EnsureTimeStateInitializedLocked();
    LeaveCriticalSection(&g_timeLock);
}

static void InstallDxgiHooks()
{
    if (InterlockedCompareExchange(&g_dxgiHookInstalled, 1, 0) != 0)
    {
        return;
    }

    HMODULE dxgiModule = GetModuleHandleW(L"dxgi.dll");
    if (dxgiModule == NULL)
    {
        dxgiModule = LoadLibraryW(L"dxgi.dll");
    }
    if (dxgiModule == NULL)
    {
        WriteProxyLog(L"Could not load dxgi.dll for hook installation.");
        return;
    }

    g_realCreateDXGIFactory = (CreateDXGIFactoryFn)GetProcAddress(dxgiModule, "CreateDXGIFactory");
    g_realCreateDXGIFactory1 = (CreateDXGIFactoryFn)GetProcAddress(dxgiModule, "CreateDXGIFactory1");
    g_realCreateDXGIFactory2 = (CreateDXGIFactory2Fn)GetProcAddress(dxgiModule, "CreateDXGIFactory2");
    bool hooked = false;
    hooked |= PatchImportAddress("dxgi.dll", "CreateDXGIFactory", (void*)Hook_CreateDXGIFactory, (void**)&g_realCreateDXGIFactory);
    hooked |= PatchImportAddress("dxgi.dll", "CreateDXGIFactory1", (void*)Hook_CreateDXGIFactory1, (void**)&g_realCreateDXGIFactory1);
    hooked |= PatchImportAddress("dxgi.dll", "CreateDXGIFactory2", (void*)Hook_CreateDXGIFactory2, (void**)&g_realCreateDXGIFactory2);
    WriteProxyLog(hooked ? L"DXGI factory import hook installed." : L"No DXGI factory imports found in the game executable.");
}

static void StartManagedBootstrap()
{
    HRESULT hr;
    HMODULE mscoree = LoadLibraryW(L"mscoree.dll");
    if (mscoree == NULL)
    {
        WriteProxyLog(L"Could not load mscoree.dll");
        return;
    }

    CLRCreateInstanceFn clrCreateInstance = (CLRCreateInstanceFn)GetProcAddress(mscoree, "CLRCreateInstance");
    if (clrCreateInstance == NULL)
    {
        WriteProxyLog(L"Could not find CLRCreateInstance");
        return;
    }

    ICLRMetaHost* metaHost = NULL;
    ICLRRuntimeInfo* runtimeInfo = NULL;
    ICLRRuntimeHost* runtimeHost = NULL;
    DWORD returnValue = 0;
    wchar_t assemblyPath[MAX_PATH];

    hr = clrCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&metaHost);
    if (FAILED(hr) || metaHost == NULL)
    {
        WriteProxyLog(L"Could not create CLRMetaHost");
        return;
    }

    hr = metaHost->GetRuntime(L"v4.0.30319", IID_ICLRRuntimeInfo, (LPVOID*)&runtimeInfo);
    if (FAILED(hr) || runtimeInfo == NULL)
    {
        WriteProxyLog(L"Could not get CLR runtime info");
        metaHost->Release();
        return;
    }

    hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&runtimeHost);
    if (FAILED(hr) || runtimeHost == NULL)
    {
        WriteProxyLog(L"Could not get CLR runtime host");
        runtimeInfo->Release();
        metaHost->Release();
        return;
    }

    hr = runtimeHost->Start();
    if (FAILED(hr))
    {
        WriteProxyLog(L"Could not start CLR runtime");
        runtimeHost->Release();
        runtimeInfo->Release();
        metaHost->Release();
        return;
    }

    GetManagedAssemblyPath(assemblyPath);
    hr = runtimeHost->ExecuteInDefaultAppDomain(assemblyPath, L"RROML.Core.Bootstrap", L"InitializeForProxy", L"", &returnValue);
    if (FAILED(hr))
    {
        WriteProxyLog(L"ExecuteInDefaultAppDomain failed");
    }

    runtimeHost->Release();
    runtimeInfo->Release();
    metaHost->Release();
}

static void EnsureBootstrapStarted()
{
    if (InterlockedCompareExchange(&g_bootstrapStarted, 1, 0) == 0)
    {
        EnsureOverlayLock();
        EnterCriticalSection(&g_overlayLock);
        StringCchCopyW(g_overlayText, 256, L"RROML Loading...");
        g_overlayStatusKind = 0;
        g_overlayHideTick = 0;
        LeaveCriticalSection(&g_overlayLock);

        HANDLE threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartManagedBootstrap, NULL, 0, NULL);
        if (threadHandle != NULL)
        {
            CloseHandle(threadHandle);
        }
    }
}

static bool GetMainModuleInfoInternal(BYTE** baseAddress, DWORD* imageSize)
{
    HMODULE module = GetModuleHandleW(NULL);
    if (module == NULL)
    {
        return false;
    }

    BYTE* base = (BYTE*)module;
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)base;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }

    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(base + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }

    if (baseAddress != NULL)
    {
        *baseAddress = base;
    }

    if (imageSize != NULL)
    {
        *imageSize = ntHeaders->OptionalHeader.SizeOfImage;
    }

    return true;
}

static bool FindTextSectionRange(BYTE** sectionStart, BYTE** sectionEnd)
{
    BYTE* base = NULL;
    DWORD imageSize = 0;
    if (!GetMainModuleInfoInternal(&base, &imageSize))
    {
        return false;
    }

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(base + dosHeader->e_lfanew);
    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section)
    {
        if (memcmp(section->Name, ".text", 5) == 0)
        {
            if (sectionStart != NULL)
            {
                *sectionStart = base + section->VirtualAddress;
            }
            if (sectionEnd != NULL)
            {
                *sectionEnd = base + section->VirtualAddress + section->Misc.VirtualSize;
            }
            return true;
        }
    }

    return false;
}

static DWORD CollectAddressValueReferences(ULONGLONG targetAddress, ULONGLONG* matchAddresses, DWORD maxCount)
{
    BYTE* base = NULL;
    DWORD imageSize = 0;
    if (targetAddress == 0 || !GetMainModuleInfoInternal(&base, &imageSize) || imageSize < sizeof(ULONGLONG))
    {
        return 0;
    }

    DWORD matchCount = 0;
    for (DWORD i = 0; i <= imageSize - sizeof(ULONGLONG); ++i)
    {
        ULONGLONG value = 0;
        memcpy(&value, base + i, sizeof(value));
        if (value != targetAddress)
        {
            continue;
        }

        if (matchAddresses != NULL && matchCount < maxCount)
        {
            matchAddresses[matchCount] = (ULONGLONG)(base + i);
        }

        ++matchCount;
    }

    return matchCount;
}

static DWORD CollectRipRelativeCodeReferences(ULONGLONG targetAddress, ULONGLONG* instructionAddresses, DWORD maxCount)
{
    BYTE* textStart = NULL;
    BYTE* textEnd = NULL;
    if (targetAddress == 0 || !FindTextSectionRange(&textStart, &textEnd))
    {
        return 0;
    }

    DWORD matchCount = 0;
    for (BYTE* current = textStart; current + 7 < textEnd; ++current)
    {
        SIZE_T prefixLength = 0;
        if (current[0] >= 0x40 && current[0] <= 0x4F)
        {
            prefixLength = 1;
        }

        BYTE* opcode = current + prefixLength;
        if (opcode + 5 >= textEnd)
        {
            continue;
        }

        if (opcode[0] != 0x8B && opcode[0] != 0x8D)
        {
            continue;
        }

        BYTE modrm = opcode[1];
        if ((modrm & 0xC7) != 0x05)
        {
            continue;
        }

        INT32 displacement = 0;
        memcpy(&displacement, opcode + 2, sizeof(displacement));
        BYTE* resolved = opcode + 6 + displacement;
        if ((ULONGLONG)resolved == targetAddress)
        {
            if (instructionAddresses != NULL && matchCount < maxCount)
            {
                instructionAddresses[matchCount] = (ULONGLONG)current;
            }

            ++matchCount;
        }
    }

    return matchCount;
}


static DWORD CollectRipRelativeCodeReferencesInRange(BYTE* rangeStart, BYTE* rangeEnd, ULONGLONG targetAddress, ULONGLONG* instructionAddresses, DWORD maxCount)
{
    if (targetAddress == 0 || rangeStart == NULL || rangeEnd == NULL || rangeEnd <= rangeStart)
    {
        return 0;
    }

    BYTE* textStart = NULL;
    BYTE* textEnd = NULL;
    if (!FindTextSectionRange(&textStart, &textEnd))
    {
        return 0;
    }

    if (rangeStart < textStart)
    {
        rangeStart = textStart;
    }
    if (rangeEnd > textEnd)
    {
        rangeEnd = textEnd;
    }
    if (rangeEnd <= rangeStart)
    {
        return 0;
    }

    DWORD matchCount = 0;
    for (BYTE* current = rangeStart; current + 7 < rangeEnd; ++current)
    {
        SIZE_T prefixLength = 0;
        if (current[0] >= 0x40 && current[0] <= 0x4F)
        {
            prefixLength = 1;
        }

        BYTE* opcode = current + prefixLength;
        if (opcode + 5 >= rangeEnd)
        {
            continue;
        }

        if (opcode[0] != 0x8B && opcode[0] != 0x8D)
        {
            continue;
        }

        BYTE modrm = opcode[1];
        if ((modrm & 0xC7) != 0x05)
        {
            continue;
        }

        INT32 displacement = 0;
        memcpy(&displacement, opcode + 2, sizeof(displacement));
        BYTE* resolved = opcode + 6 + displacement;
        if ((ULONGLONG)resolved == targetAddress)
        {
            if (instructionAddresses != NULL && matchCount < maxCount)
            {
                instructionAddresses[matchCount] = (ULONGLONG)current;
            }
            ++matchCount;
        }
    }

    return matchCount;
}
static int HexNibble(char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }

    if (value >= 'A' && value <= 'F')
    {
        return 10 + (value - 'A');
    }

    if (value >= 'a' && value <= 'f')
    {
        return 10 + (value - 'a');
    }

    return -1;
}

static DWORD ParseIdaPattern(const char* pattern, BYTE* bytes, BOOL* wildcards, DWORD maxBytes)
{
    if (pattern == NULL)
    {
        return 0;
    }

    DWORD count = 0;
    const char* current = pattern;
    while (*current != '\0' && count < maxBytes)
    {
        while (*current == ' ' || *current == '\t' || *current == '\r' || *current == '\n')
        {
            ++current;
        }

        if (*current == '\0')
        {
            break;
        }

        if (*current == '?')
        {
            bytes[count] = 0;
            wildcards[count] = TRUE;
            ++count;
            ++current;
            if (*current == '?')
            {
                ++current;
            }
            continue;
        }

        int high = HexNibble(*current++);
        if (high < 0 || *current == '\0')
        {
            return 0;
        }

        int low = HexNibble(*current++);
        if (low < 0)
        {
            return 0;
        }

        bytes[count] = (BYTE)((high << 4) | low);
        wildcards[count] = FALSE;
        ++count;
    }

    return count;
}

static BOOL FindPatternInternal(BYTE* rangeStart, BYTE* rangeEnd, const char* idaPattern, ULONGLONG* address)
{
    BYTE bytes[256] = {};
    BOOL wildcards[256] = {};
    DWORD patternLength = ParseIdaPattern(idaPattern, bytes, wildcards, 256);
    if (patternLength == 0 || rangeStart == NULL || rangeEnd == NULL || rangeEnd <= rangeStart || (SIZE_T)(rangeEnd - rangeStart) < patternLength)
    {
        return FALSE;
    }

    BYTE* lastStart = rangeEnd - patternLength;
    for (BYTE* current = rangeStart; current <= lastStart; ++current)
    {
        BOOL matched = TRUE;
        for (DWORD i = 0; i < patternLength; ++i)
        {
            if (!wildcards[i] && current[i] != bytes[i])
            {
                matched = FALSE;
                break;
            }
        }

        if (matched)
        {
            if (address != NULL)
            {
                *address = (ULONGLONG)current;
            }
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL IsReadableAddressRange(const BYTE* address, SIZE_T length)
{
    if (address == NULL || length == 0)
    {
        return FALSE;
    }

    const BYTE* current = address;
    const BYTE* end = address + length;
    while (current < end)
    {
        MEMORY_BASIC_INFORMATION info = {};
        if (VirtualQuery(current, &info, sizeof(info)) == 0)
        {
            return FALSE;
        }

        if (info.State != MEM_COMMIT)
        {
            return FALSE;
        }

        if ((info.Protect & PAGE_GUARD) != 0 || (info.Protect & PAGE_NOACCESS) != 0)
        {
            return FALSE;
        }

        BYTE* regionEnd = (BYTE*)info.BaseAddress + info.RegionSize;
        if (regionEnd <= current)
        {
            return FALSE;
        }

        current = regionEnd < end ? regionEnd : end;
    }

    return TRUE;
}
extern "C" __declspec(dllexport) DWORD WINAPI RROML_FindAddressValueRefs(ULONGLONG targetAddress, ULONGLONG* matchAddresses, DWORD maxCount)
{
    return CollectAddressValueReferences(targetAddress, matchAddresses, maxCount);
}


extern "C" __declspec(dllexport) DWORD WINAPI RROML_FindRipRelativeCodeRefsInRange(ULONGLONG rangeStart, DWORD rangeLength, ULONGLONG targetAddress, ULONGLONG* instructionAddresses, DWORD maxCount)
{
    if (rangeStart == 0 || rangeLength == 0)
    {
        return 0;
    }

    BYTE* start = (BYTE*)rangeStart;
    BYTE* end = start + rangeLength;
    return CollectRipRelativeCodeReferencesInRange(start, end, targetAddress, instructionAddresses, maxCount);
}
extern "C" __declspec(dllexport) DWORD WINAPI RROML_FindRipRelativeCodeRefs(ULONGLONG targetAddress, ULONGLONG* instructionAddresses, DWORD maxCount)
{
    return CollectRipRelativeCodeReferences(targetAddress, instructionAddresses, maxCount);
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_FindPatternA(const char* idaPattern, BOOL textSectionOnly, ULONGLONG* address)
{
    BYTE* rangeStart = NULL;
    BYTE* rangeEnd = NULL;
    if (textSectionOnly)
    {
        if (!FindTextSectionRange(&rangeStart, &rangeEnd))
        {
            return FALSE;
        }
    }
    else
    {
        BYTE* base = NULL;
        DWORD imageSize = 0;
        if (!GetMainModuleInfoInternal(&base, &imageSize))
        {
            return FALSE;
        }

        rangeStart = base;
        rangeEnd = base + imageSize;
    }

    return FindPatternInternal(rangeStart, rangeEnd, idaPattern, address);
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_ResolveRipRelativeTarget(ULONGLONG instructionAddress, INT displacementOffset, INT instructionLength, ULONGLONG* resolvedAddress)
{
    BYTE* base = NULL;
    DWORD imageSize = 0;
    if (instructionAddress == 0 || !GetMainModuleInfoInternal(&base, &imageSize))
    {
        return FALSE;
    }

    BYTE* instruction = (BYTE*)instructionAddress;
    if (instruction < base || instruction + displacementOffset + (INT)sizeof(INT32) > base + imageSize)
    {
        return FALSE;
    }

    INT32 displacement = 0;
    memcpy(&displacement, instruction + displacementOffset, sizeof(displacement));
    BYTE* target = instruction + instructionLength + displacement;
    if (resolvedAddress != NULL)
    {
        *resolvedAddress = (ULONGLONG)target;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_FindRipRelativeLeaXref(ULONGLONG targetAddress, ULONGLONG* instructionAddress)
{
    ULONGLONG firstMatch = 0;
    if (CollectRipRelativeCodeReferences(targetAddress, &firstMatch, 1) == 0)
    {
        return FALSE;
    }

    if (instructionAddress != NULL)
    {
        *instructionAddress = firstMatch;
    }

    return TRUE;
}


static bool MatchesFunctionPrologue(const BYTE* current, const BYTE* textEnd)
{
    if (current == NULL || current >= textEnd)
    {
        return false;
    }

    if (current + 2 <= textEnd && current[0] == 0x40 && current[1] == 0x53)
    {
        return true;
    }

    if (current + 3 <= textEnd && current[0] == 0x48 && current[1] == 0x83 && current[2] == 0xEC)
    {
        return true;
    }

    if (current + 4 <= textEnd && current[0] == 0x48 && current[1] == 0x89 && current[2] == 0x5C && current[3] == 0x24)
    {
        return true;
    }

    if (current + 4 <= textEnd && current[0] == 0x48 && current[1] == 0x89 && current[2] == 0x74 && current[3] == 0x24)
    {
        return true;
    }

    if (current + 3 <= textEnd && current[0] == 0x4C && current[1] == 0x8B && current[2] == 0xDC)
    {
        return true;
    }

    if (current + 4 <= textEnd && current[0] == 0x48 && current[1] == 0x8B && current[2] == 0xC4 && current[3] == 0x48)
    {
        return true;
    }

    if (current + 2 <= textEnd && current[0] == 0x55 && current[1] == 0x56)
    {
        return true;
    }

    return false;
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_FindLikelyFunctionStart(ULONGLONG instructionAddress, ULONGLONG* functionStart)
{
    BYTE* textStart = NULL;
    BYTE* textEnd = NULL;
    if (instructionAddress == 0 || !FindTextSectionRange(&textStart, &textEnd))
    {
        return FALSE;
    }

    BYTE* instruction = (BYTE*)instructionAddress;
    if (instruction < textStart || instruction >= textEnd)
    {
        return FALSE;
    }

    SIZE_T maxBacktrack = 0x400;
    BYTE* scanStart = instruction > textStart + maxBacktrack ? instruction - maxBacktrack : textStart;
    for (BYTE* current = instruction; current >= scanStart; --current)
    {
        if (MatchesFunctionPrologue(current, textEnd))
        {
            if (functionStart != NULL)
            {
                *functionStart = (ULONGLONG)current;
            }
            return TRUE;
        }

        if (current > scanStart && current[-1] == 0xCC && current[0] != 0xCC)
        {
            if (functionStart != NULL)
            {
                *functionStart = (ULONGLONG)current;
            }
            return TRUE;
        }

        if (current == scanStart)
        {
            break;
        }
    }

    return FALSE;
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_ReadQword(ULONGLONG address, ULONGLONG* value)
{
    if (address == 0 || value == NULL || !IsReadableAddressRange((const BYTE*)address, sizeof(ULONGLONG)))
    {
        return FALSE;
    }

    memcpy(value, (const void*)address, sizeof(ULONGLONG));
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_ReadMemory(ULONGLONG address, BYTE* buffer, DWORD length)
{
    if (address == 0 || buffer == NULL || length == 0 || !IsReadableAddressRange((const BYTE*)address, length))
    {
        return FALSE;
    }

    memcpy(buffer, (const void*)address, length);
    return TRUE;
}
extern "C" __declspec(dllexport) BOOL WINAPI RROML_GetMainModuleInfo(ULONGLONG* baseAddress, DWORD* imageSize)
{
    BYTE* base = NULL;
    DWORD size = 0;
    if (!GetMainModuleInfoInternal(&base, &size))
    {
        return FALSE;
    }

    if (baseAddress != NULL)
    {
        *baseAddress = (ULONGLONG)base;
    }

    if (imageSize != NULL)
    {
        *imageSize = size;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_FindAsciiString(const char* needle, ULONGLONG* address)
{
    BYTE* base = NULL;
    DWORD size = 0;
    if (needle == NULL || needle[0] == '\0' || !GetMainModuleInfoInternal(&base, &size))
    {
        return FALSE;
    }

    size_t needleLength = strlen(needle);
    if (needleLength == 0 || needleLength > size)
    {
        return FALSE;
    }

    for (DWORD i = 0; i <= size - needleLength; ++i)
    {
        if (memcmp(base + i, needle, needleLength) == 0)
        {
            if (address != NULL)
            {
                *address = (ULONGLONG)(base + i);
            }
            return TRUE;
        }
    }

    return FALSE;
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_FindWideStringW(const wchar_t* needle, ULONGLONG* address)
{
    BYTE* base = NULL;
    DWORD size = 0;
    if (needle == NULL || needle[0] == L'\0' || !GetMainModuleInfoInternal(&base, &size))
    {
        return FALSE;
    }

    size_t needleLength = wcslen(needle) * sizeof(wchar_t);
    if (needleLength == 0 || needleLength > size)
    {
        return FALSE;
    }

    for (DWORD i = 0; i <= size - needleLength; ++i)
    {
        if (memcmp(base + i, needle, needleLength) == 0)
        {
            if (address != NULL)
            {
                *address = (ULONGLONG)(base + i);
            }
            return TRUE;
        }
    }

    return FALSE;
}
extern "C" __declspec(dllexport) void WINAPI RROML_SetOverlayStatusW(const wchar_t* text, int statusKind, int autoHideMs)
{
    EnsureOverlayLock();
    EnterCriticalSection(&g_overlayLock);
    if (text == NULL)
    {
        g_overlayText[0] = L'\0';
    }
    else
    {
        StringCchCopyW(g_overlayText, 256, text);
    }
    g_overlayStatusKind = statusKind;
    g_overlayHideTick = autoHideMs > 0 ? GetTickCount() + (DWORD)autoHideMs : 0;
    LeaveCriticalSection(&g_overlayLock);
}

extern "C" __declspec(dllexport) void WINAPI RROML_SetOverlayPosition(int position)
{
    EnsureOverlayLock();
    EnterCriticalSection(&g_overlayLock);
    g_overlayPosition = position;
    LeaveCriticalSection(&g_overlayLock);
}

extern "C" __declspec(dllexport) BOOL WINAPI RROML_SetTimeScale(double scale)
{
    LARGE_INTEGER realQpc = {};
    if (!ReadRealQpc(&realQpc))
    {
        return FALSE;
    }

    ULONGLONG realTick64 = ReadRealTick64();
    DWORD realTick32 = ReadRealTick32();
    DWORD realTimeGet = ReadRealTimeGetTime();

    EnsureTimeLock();
    EnterCriticalSection(&g_timeLock);
    EnsureTimeStateInitializedLocked();

    LARGE_INTEGER currentVirtualQpc = {};
    currentVirtualQpc.QuadPart = ScaleQpcValueLocked(realQpc);
    ULONGLONG currentVirtualTick64 = ScaleTick64ValueLocked(realTick64);
    DWORD currentVirtualTick32 = ScaleTick32ValueLocked(realTick32);
    DWORD currentVirtualTimeGet = ScaleTimeGetTimeValueLocked(realTimeGet);

    g_qpcBaseReal = realQpc;
    g_qpcBaseVirtual = currentVirtualQpc;
    g_tick64BaseReal = realTick64;
    g_tick64BaseVirtual = currentVirtualTick64;
    g_tickBaseReal = realTick32;
    g_tickBaseVirtual = currentVirtualTick32;
    g_timeGetBaseReal = realTimeGet;
    g_timeGetBaseVirtual = currentVirtualTimeGet;
    g_timeScale = ClampTimeScale(scale);

    LeaveCriticalSection(&g_timeLock);
    return TRUE;
}

extern "C" __declspec(dllexport) double WINAPI RROML_GetTimeScale()
{
    EnsureTimeLock();
    EnterCriticalSection(&g_timeLock);
    EnsureTimeStateInitializedLocked();
    double value = g_timeScale;
    LeaveCriticalSection(&g_timeLock);
    return value;
}

extern "C" BOOL WINAPI ExportedDllMain(HINSTANCE, DWORD, LPVOID)
{
    return TRUE;
}

extern "C" DWORD WINAPI XInputGetState(DWORD userIndex, LPVOID state)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputGetState != NULL ? g_xinputGetState(userIndex, state) : ERROR_DEVICE_NOT_CONNECTED;
}

extern "C" DWORD WINAPI XInputSetState(DWORD userIndex, LPVOID vibration)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputSetState != NULL ? g_xinputSetState(userIndex, vibration) : ERROR_DEVICE_NOT_CONNECTED;
}

extern "C" DWORD WINAPI XInputGetCapabilities(DWORD userIndex, DWORD flags, LPVOID capabilities)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputGetCapabilities != NULL ? g_xinputGetCapabilities(userIndex, flags, capabilities) : ERROR_DEVICE_NOT_CONNECTED;
}

extern "C" VOID WINAPI XInputEnable(BOOL enable)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    if (g_xinputEnable != NULL)
    {
        g_xinputEnable(enable);
    }
}

extern "C" DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD userIndex, GUID* renderGuid, GUID* captureGuid)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputGetDSoundAudioDeviceGuids != NULL ? g_xinputGetDSoundAudioDeviceGuids(userIndex, renderGuid, captureGuid) : ERROR_DEVICE_NOT_CONNECTED;
}

extern "C" DWORD WINAPI XInputGetBatteryInformation(DWORD userIndex, BYTE devType, LPVOID batteryInformation)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputGetBatteryInformation != NULL ? g_xinputGetBatteryInformation(userIndex, devType, batteryInformation) : ERROR_DEVICE_NOT_CONNECTED;
}

extern "C" DWORD WINAPI XInputGetKeystroke(DWORD userIndex, DWORD reserved, LPVOID keystroke)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputGetKeystroke != NULL ? g_xinputGetKeystroke(userIndex, reserved, keystroke) : ERROR_EMPTY;
}

extern "C" DWORD WINAPI XInputGetStateEx(DWORD userIndex, LPVOID state)
{
    LoadRealXInput();
    EnsureBootstrapStarted();
    return g_xinputGetStateEx != NULL ? g_xinputGetStateEx(userIndex, state) : ERROR_DEVICE_NOT_CONNECTED;
}

extern "C" DWORD WINAPI XInputOrdinal101(void)
{
    LoadRealXInput();
    return g_xinputOrdinal101 != NULL ? g_xinputOrdinal101() : ERROR_PROC_NOT_FOUND;
}

extern "C" DWORD WINAPI XInputOrdinal102(void)
{
    LoadRealXInput();
    return g_xinputOrdinal102 != NULL ? g_xinputOrdinal102() : ERROR_PROC_NOT_FOUND;
}

extern "C" DWORD WINAPI XInputOrdinal103(void)
{
    LoadRealXInput();
    return g_xinputOrdinal103 != NULL ? g_xinputOrdinal103() : ERROR_PROC_NOT_FOUND;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        EnsureOverlayLock();
        EnsureTimeLock();
        LoadRealXInput();
        InstallDxgiHooks();
        InstallTimeHooks();
        WriteProxyLog(L"DXGI in-game status overlay hooks installed.");
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        ResetRendererResources();
    }

    return TRUE;
}





















