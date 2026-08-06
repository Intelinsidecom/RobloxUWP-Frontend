#include "pch.h"
#include "CubeRenderer.h"
#include "..\Components\InputManager.h"
#include "..\Components\GameParameters.h"

#include <d3dcompiler.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace Roblox::Rendering;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

static void LogFailure(const char* step, HRESULT hr)
{
    char buffer[192];
    sprintf_s(buffer, "CubeRenderer: %s failed, hr=0x%08X\n", step, hr);
    OutputDebugStringA(buffer);
}

struct CubeVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct CubeConstants
{
    DirectX::XMFLOAT4X4 model;
    DirectX::XMFLOAT4X4 viewProj;
};

static const char kVertexShaderSource[] =
    "#pragma pack_matrix(row_major)\n"
    "cbuffer CubeConstants : register(b0)\n"
    "{\n"
    "    float4x4 model;\n"
    "    float4x4 viewProj;\n"
    "};\n"
    "struct VertexInput\n"
    "{\n"
    "    float3 position : POSITION;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct PixelInput\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "PixelInput VSMain(VertexInput input)\n"
    "{\n"
    "    PixelInput output;\n"
    "    float4 worldPosition = mul(float4(input.position, 1.0f), model);\n"
    "    output.position = mul(worldPosition, viewProj);\n"
    "    output.color = input.color;\n"
    "    return output;\n"
    "}\n";

static const char kPixelShaderSource[] =
    "struct PixelInput\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 PSMain(PixelInput input) : SV_Target\n"
    "{\n"
    "    return input.color;\n"
    "}\n";

struct TextVertex
{
    DirectX::XMFLOAT2 position;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT4 color;
};

struct TextConstants
{
    DirectX::XMFLOAT4X4 viewProj;
};

static const char kTextVertexShaderSource[] =
    "#pragma pack_matrix(row_major)\n"
    "cbuffer TextConstants : register(b0)\n"
    "{\n"
    "    float4x4 viewProj;\n"
    "};\n"
    "struct VertexInput\n"
    "{\n"
    "    float2 position : POSITION;\n"
    "    float2 uv : TEXCOORD;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct PixelInput\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float2 uv : TEXCOORD;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "PixelInput VSMain(VertexInput input)\n"
    "{\n"
    "    PixelInput output;\n"
    "    output.position = mul(float4(input.position, 0.0f, 1.0f), viewProj);\n"
    "    output.uv = input.uv;\n"
    "    output.color = input.color;\n"
    "    return output;\n"
    "}\n";

static const char kTextPixelShaderSource[] =
    "Texture2D fontTexture : register(t0);\n"
    "SamplerState fontSampler : register(s0);\n"
    "struct PixelInput\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float2 uv : TEXCOORD;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 PSMain(PixelInput input) : SV_Target\n"
    "{\n"
    "    float alpha = fontTexture.Sample(fontSampler, input.uv).r;\n"
    "    return float4(input.color.rgb * alpha, input.color.a * alpha);\n"
    "}\n";

static const unsigned char kFont5x7[95][5] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x5F, 0x00, 0x00 },
    { 0x00, 0x07, 0x00, 0x07, 0x00 },
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 },
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 },
    { 0x23, 0x13, 0x08, 0x64, 0x62 },
    { 0x36, 0x49, 0x55, 0x22, 0x50 },
    { 0x00, 0x05, 0x03, 0x00, 0x00 },
    { 0x00, 0x1C, 0x22, 0x41, 0x00 },
    { 0x00, 0x41, 0x22, 0x1C, 0x00 },
    { 0x08, 0x2A, 0x1C, 0x2A, 0x08 },
    { 0x08, 0x08, 0x3E, 0x08, 0x08 },
    { 0x00, 0x50, 0x30, 0x00, 0x00 },
    { 0x08, 0x08, 0x08, 0x08, 0x08 },
    { 0x00, 0x60, 0x60, 0x00, 0x00 },
    { 0x20, 0x10, 0x08, 0x04, 0x02 },
    { 0x3E, 0x51, 0x49, 0x45, 0x3E },
    { 0x00, 0x42, 0x7F, 0x40, 0x00 },
    { 0x42, 0x61, 0x51, 0x49, 0x46 },
    { 0x21, 0x41, 0x45, 0x4B, 0x31 },
    { 0x18, 0x14, 0x12, 0x7F, 0x10 },
    { 0x27, 0x45, 0x45, 0x45, 0x39 },
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 },
    { 0x01, 0x71, 0x09, 0x05, 0x03 },
    { 0x36, 0x49, 0x49, 0x49, 0x36 },
    { 0x06, 0x49, 0x49, 0x29, 0x1E },
    { 0x00, 0x36, 0x36, 0x00, 0x00 },
    { 0x00, 0x56, 0x36, 0x00, 0x00 },
    { 0x00, 0x08, 0x14, 0x22, 0x41 },
    { 0x14, 0x14, 0x14, 0x14, 0x14 },
    { 0x41, 0x22, 0x14, 0x08, 0x00 },
    { 0x02, 0x01, 0x51, 0x09, 0x06 },
    { 0x32, 0x49, 0x79, 0x41, 0x3E },
    { 0x7E, 0x11, 0x11, 0x11, 0x7E },
    { 0x7F, 0x49, 0x49, 0x49, 0x36 },
    { 0x3E, 0x41, 0x41, 0x41, 0x22 },
    { 0x7F, 0x41, 0x41, 0x22, 0x1C },
    { 0x7F, 0x49, 0x49, 0x49, 0x41 },
    { 0x7F, 0x09, 0x09, 0x01, 0x01 },
    { 0x3E, 0x41, 0x41, 0x51, 0x32 },
    { 0x7F, 0x08, 0x08, 0x08, 0x7F },
    { 0x00, 0x41, 0x7F, 0x41, 0x00 },
    { 0x20, 0x40, 0x41, 0x3F, 0x01 },
    { 0x7F, 0x08, 0x14, 0x22, 0x41 },
    { 0x7F, 0x40, 0x40, 0x40, 0x40 },
    { 0x7F, 0x02, 0x04, 0x02, 0x7F },
    { 0x7F, 0x04, 0x08, 0x10, 0x7F },
    { 0x3E, 0x41, 0x41, 0x41, 0x3E },
    { 0x7F, 0x09, 0x09, 0x09, 0x06 },
    { 0x3E, 0x41, 0x51, 0x21, 0x5E },
    { 0x7F, 0x09, 0x19, 0x29, 0x46 },
    { 0x46, 0x49, 0x49, 0x49, 0x31 },
    { 0x01, 0x01, 0x7F, 0x01, 0x01 },
    { 0x3F, 0x40, 0x40, 0x40, 0x3F },
    { 0x1F, 0x20, 0x40, 0x20, 0x1F },
    { 0x7F, 0x20, 0x18, 0x20, 0x7F },
    { 0x63, 0x14, 0x08, 0x14, 0x63 },
    { 0x03, 0x04, 0x78, 0x04, 0x03 },
    { 0x61, 0x51, 0x49, 0x45, 0x43 },
    { 0x00, 0x00, 0x7F, 0x41, 0x41 },
    { 0x02, 0x04, 0x08, 0x10, 0x20 },
    { 0x41, 0x41, 0x7F, 0x00, 0x00 },
    { 0x04, 0x02, 0x01, 0x02, 0x04 },
    { 0x40, 0x40, 0x40, 0x40, 0x40 },
    { 0x00, 0x01, 0x02, 0x04, 0x00 },
    { 0x20, 0x54, 0x54, 0x54, 0x78 },
    { 0x7F, 0x48, 0x44, 0x44, 0x38 },
    { 0x38, 0x44, 0x44, 0x44, 0x20 },
    { 0x38, 0x44, 0x44, 0x48, 0x7F },
    { 0x38, 0x54, 0x54, 0x54, 0x18 },
    { 0x08, 0x7E, 0x09, 0x01, 0x02 },
    { 0x0C, 0x52, 0x52, 0x52, 0x3E },
    { 0x7F, 0x08, 0x04, 0x04, 0x78 },
    { 0x00, 0x44, 0x7D, 0x40, 0x00 },
    { 0x20, 0x40, 0x44, 0x3D, 0x00 },
    { 0x00, 0x7F, 0x10, 0x28, 0x44 },
    { 0x00, 0x41, 0x7F, 0x40, 0x00 },
    { 0x7C, 0x04, 0x18, 0x04, 0x78 },
    { 0x7C, 0x08, 0x04, 0x04, 0x78 },
    { 0x38, 0x44, 0x44, 0x44, 0x38 },
    { 0x7C, 0x14, 0x14, 0x14, 0x08 },
    { 0x08, 0x14, 0x14, 0x18, 0x7C },
    { 0x7C, 0x08, 0x04, 0x04, 0x08 },
    { 0x48, 0x54, 0x54, 0x54, 0x20 },
    { 0x04, 0x3F, 0x44, 0x40, 0x20 },
    { 0x3C, 0x40, 0x40, 0x20, 0x7C },
    { 0x1C, 0x20, 0x40, 0x20, 0x1C },
    { 0x3C, 0x40, 0x30, 0x40, 0x3C },
    { 0x44, 0x28, 0x10, 0x28, 0x44 },
    { 0x0C, 0x50, 0x50, 0x50, 0x3C },
    { 0x44, 0x64, 0x54, 0x4C, 0x44 },
    { 0x00, 0x08, 0x36, 0x41, 0x00 },
    { 0x00, 0x00, 0x7F, 0x00, 0x00 },
    { 0x00, 0x41, 0x36, 0x08, 0x00 },
    { 0x02, 0x01, 0x02, 0x01, 0x02 },
};

static const int kGlyphCount = 95;
static const int kGlyphCellWidth = 6;
static const int kGlyphCellHeight = 8;
static const int kFontTextureWidth = kGlyphCount * kGlyphCellWidth;
static const int kFontTextureHeight = kGlyphCellHeight;

static void DrawTextIntoBuffer(const char* text, float x, float y, float scale, DirectX::XMFLOAT4 color, TextVertex* vertices, unsigned int& count, unsigned int maxCount)
{
    for (; *text != '\0'; text++)
    {
        unsigned char c = static_cast<unsigned char>(*text);
        if (c < 0x20 || c > 0x7E)
        {
            c = '?';
        }

        int glyph = c - 0x20;

        float u0 = static_cast<float>(glyph * kGlyphCellWidth) / static_cast<float>(kFontTextureWidth);
        float u1 = static_cast<float>(glyph * kGlyphCellWidth + 5) / static_cast<float>(kFontTextureWidth);
        float v0 = 0.0f;
        float v1 = 7.0f / static_cast<float>(kFontTextureHeight);

        float w = 5.0f * scale;
        float h = 7.0f * scale;

        if (count + 6 > maxCount)
        {
            break;
        }

        TextVertex tri[6];
        tri[0].position = DirectX::XMFLOAT2(x, y);
        tri[0].uv = DirectX::XMFLOAT2(u0, v0);
        tri[0].color = color;
        tri[1].position = DirectX::XMFLOAT2(x + w, y);
        tri[1].uv = DirectX::XMFLOAT2(u1, v0);
        tri[1].color = color;
        tri[2].position = DirectX::XMFLOAT2(x, y + h);
        tri[2].uv = DirectX::XMFLOAT2(u0, v1);
        tri[2].color = color;
        tri[3].position = DirectX::XMFLOAT2(x + w, y);
        tri[3].uv = DirectX::XMFLOAT2(u1, v0);
        tri[3].color = color;
        tri[4].position = DirectX::XMFLOAT2(x + w, y + h);
        tri[4].uv = DirectX::XMFLOAT2(u1, v1);
        tri[4].color = color;
        tri[5].position = DirectX::XMFLOAT2(x, y + h);
        tri[5].uv = DirectX::XMFLOAT2(u0, v1);
        tri[5].color = color;

        memcpy(&vertices[count], tri, sizeof(tri));
        count += 6;

        x += static_cast<float>(kGlyphCellWidth) * scale;
    }
}

static bool HasValue(Platform::String^ value)
{
    return value != nullptr && value->Length() > 0;
}

static void StringToChar(Platform::String^ value, char* out, size_t maxLen)
{
    if (value == nullptr || maxLen == 0)
    {
        if (maxLen > 0)
        {
            out[0] = '\0';
        }
        return;
    }
    size_t converted = 0;
    wcstombs_s(&converted, out, maxLen, value->Data(), _TRUNCATE);
    (void)converted;
    out[maxLen - 1] = '\0';
}

static void DrawParamLine(const char* label, Platform::String^ value, float x, float& y, float scale, const DirectX::XMFLOAT4& color, TextVertex* vertices, unsigned int& count)
{
    if (!HasValue(value))
    {
        return;
    }

    char valueText[256];
    StringToChar(value, valueText, sizeof(valueText));

    char line[320];
    sprintf_s(line, "%s: %s", label, valueText);
    DrawTextIntoBuffer(line, x, y, scale, color, vertices, count, 2048);
    y += static_cast<float>(kGlyphCellHeight) * scale;
}

CubeRenderer::CubeRenderer()
    : m_panel(nullptr)
    , m_timer(nullptr)
    , m_selfReference(nullptr)
    , m_inputManager(nullptr)
    , m_gameParameters(nullptr)
    , m_angle(0.0f)
    , m_width(0)
    , m_height(0)
    , m_exitX(0.0f)
    , m_exitY(0.0f)
    , m_exitVX(0.0f)
    , m_exitVY(0.0f)
    , m_exitInitialized(false)
    , m_exitTriggered(false)
{
}

CubeRenderer::~CubeRenderer()
{
    Stop();
}

void CubeRenderer::Initialize(Windows::UI::Xaml::Controls::SwapChainPanel^ panel)
{
    Stop();

    m_panel = panel;
    m_swapChain = nullptr;
    m_width = 0;
    m_height = 0;

    CreateDeviceResources();
    CreateCubeResources();
    CreateTextResources();
}

void CubeRenderer::SetInputManager(Roblox::InputManager^ inputManager)
{
    m_inputManager = inputManager;
}

void CubeRenderer::SetGameParameters(Roblox::Controls::GameParameters^ parameters)
{
    m_gameParameters = parameters;
}

void CubeRenderer::SetLeaveCallback(std::function<void()> callback)
{
    m_leaveCallback = callback;
}

void CubeRenderer::Start()
{
    if (m_timer == nullptr)
    {
        m_timer = ref new DispatcherTimer();
        Windows::Foundation::TimeSpan interval;
        interval.Duration = 166667;
        m_timer->Interval = interval;
        m_timer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &CubeRenderer::OnTick);
    }
    m_selfReference = this;
    m_timer->Start();
}

void CubeRenderer::Stop()
{
    if (m_timer != nullptr)
    {
        m_timer->Stop();
        m_timer = nullptr;
    }
    m_selfReference = nullptr;
}

bool CubeRenderer::CreateDeviceResources()
{
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        nullptr,
        &m_context);

    if (FAILED(hr))
    {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            flags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &m_device,
            nullptr,
            &m_context);
    }

    if (FAILED(hr))
    {
        LogFailure("D3D11CreateDevice", hr);
    }

    return SUCCEEDED(hr);
}

void CubeRenderer::CreateSwapChain()
{
    if (!m_device || m_panel == nullptr)
    {
        return;
    }

    float scaleX = m_panel->CompositionScaleX;
    float scaleY = m_panel->CompositionScaleY;

    float widthDips = static_cast<float>(m_panel->ActualWidth) * scaleX;
    float heightDips = static_cast<float>(m_panel->ActualHeight) * scaleY;

    m_width = static_cast<unsigned int>(widthDips);
    m_height = static_cast<unsigned int>(heightDips);

    if (m_width == 0 || m_height == 0)
    {
        return;
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    m_device.As(&dxgiDevice);

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);

    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    adapter->GetParent(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    m_swapChain = nullptr;
    HRESULT hr = factory->CreateSwapChainForComposition(m_device.Get(), &desc, nullptr, &m_swapChain);
    if (FAILED(hr))
    {
        LogFailure("CreateSwapChainForComposition", hr);
        return;
    }

    Microsoft::WRL::ComPtr<ISwapChainPanelNative> panelNative;
    ::IUnknown* panelUnknown = reinterpret_cast<::IUnknown*>(m_panel);
    hr = panelUnknown->QueryInterface(IID_PPV_ARGS(&panelNative));
    if (FAILED(hr))
    {
        LogFailure("QueryInterface ISwapChainPanelNative", hr);
        return;
    }

    hr = panelNative->SetSwapChain(m_swapChain.Get());
    if (FAILED(hr))
    {
        LogFailure("SetSwapChain", hr);
        return;
    }
}

bool CubeRenderer::CreateCubeResources()
{
    if (!m_device)
    {
        return false;
    }

    const float s = 1.0f;

    const CubeVertex vertices[] =
    {
        // Front (+z), red
        { { -s,  s,  s }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  s,  s,  s }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  s, -s,  s }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -s, -s,  s }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        // Back (-z), green
        { {  s,  s, -s }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -s,  s, -s }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -s, -s, -s }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { {  s, -s, -s }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        // Left (-x), blue
        { { -s,  s, -s }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -s,  s,  s }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -s, -s,  s }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -s, -s, -s }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        // Right (+x), yellow
        { {  s,  s,  s }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { {  s,  s, -s }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { {  s, -s, -s }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { {  s, -s,  s }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        // Top (+y), magenta
        { { -s,  s, -s }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { {  s,  s, -s }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { {  s,  s,  s }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { -s,  s,  s }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        // Bottom (-y), cyan
        { { -s, -s,  s }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { {  s, -s,  s }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { {  s, -s, -s }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { -s, -s, -s }, { 0.0f, 1.0f, 1.0f, 1.0f } },
    };

    const unsigned short indices[] =
    {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    HRESULT hr;

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    hr = D3DCompile(
        kVertexShaderSource,
        sizeof(kVertexShaderSource) - 1,
        nullptr,
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        0,
        0,
        &vsBlob,
        &errorBlob);
    if (FAILED(hr))
    {
        LogFailure("D3DCompile vertex shader", hr);
        return false;
    }

    hr = D3DCompile(
        kPixelShaderSource,
        sizeof(kPixelShaderSource) - 1,
        nullptr,
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        0,
        0,
        &psBlob,
        &errorBlob);
    if (FAILED(hr))
    {
        LogFailure("D3DCompile pixel shader", hr);
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    if (FAILED(hr))
    {
        return false;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_device->CreateInputLayout(
        inputLayout,
        ARRAYSIZE(inputLayout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_inputLayout);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    hr = m_device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;

    hr = m_device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(CubeConstants);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;

    hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerState);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

bool CubeRenderer::CreateTextResources()
{
    if (!m_device)
    {
        return false;
    }

    std::vector<unsigned char> pixels(static_cast<size_t>(kFontTextureWidth) * kFontTextureHeight, 0);
    for (int g = 0; g < kGlyphCount; g++)
    {
        for (int col = 0; col < 5; col++)
        {
            unsigned char bits = kFont5x7[g][col];
            for (int row = 0; row < 7; row++)
            {
                if ((bits & (1u << row)) != 0)
                {
                    int x = g * kGlyphCellWidth + col;
                    pixels[static_cast<size_t>(row) * kFontTextureWidth + x] = 255;
                }
            }
        }
    }

    HRESULT hr;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = kFontTextureWidth;
    td.Height = kFontTextureHeight;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8_UNORM;
    td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = pixels.data();
    init.SysMemPitch = kFontTextureWidth;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> fontTexture;
    hr = m_device->CreateTexture2D(&td, &init, &fontTexture);
    if (FAILED(hr))
    {
        LogFailure("CreateTexture2D font", hr);
        return false;
    }

    hr = m_device->CreateShaderResourceView(fontTexture.Get(), nullptr, &m_fontTextureView);
    if (FAILED(hr))
    {
        LogFailure("CreateShaderResourceView font", hr);
        return false;
    }

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = m_device->CreateSamplerState(&sd, &m_fontSampler);
    if (FAILED(hr))
    {
        LogFailure("CreateSamplerState font", hr);
        return false;
    }

    D3D11_BLEND_DESC bd = {};
    bd.AlphaToCoverageEnable = FALSE;
    bd.IndependentBlendEnable = FALSE;
    D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D11_BLEND_ONE;
    rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D11_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&bd, &m_textBlendState);
    if (FAILED(hr))
    {
        LogFailure("CreateBlendState text", hr);
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dd.StencilEnable = FALSE;

    hr = m_device->CreateDepthStencilState(&dd, &m_textDepthState);
    if (FAILED(hr))
    {
        LogFailure("CreateDepthStencilState text", hr);
        return false;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    hr = D3DCompile(
        kTextVertexShaderSource,
        sizeof(kTextVertexShaderSource) - 1,
        nullptr,
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        0,
        0,
        &vsBlob,
        &errorBlob);
    if (FAILED(hr))
    {
        LogFailure("D3DCompile text vertex shader", hr);
        return false;
    }

    hr = D3DCompile(
        kTextPixelShaderSource,
        sizeof(kTextPixelShaderSource) - 1,
        nullptr,
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        0,
        0,
        &psBlob,
        &errorBlob);
    if (FAILED(hr))
    {
        LogFailure("D3DCompile text pixel shader", hr);
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_textVertexShader);
    if (FAILED(hr))
    {
        return false;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_textPixelShader);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC textLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_device->CreateInputLayout(
        textLayout,
        ARRAYSIZE(textLayout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_textInputLayout);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC tvbDesc = {};
    tvbDesc.Usage = D3D11_USAGE_DYNAMIC;
    tvbDesc.ByteWidth = 2048 * sizeof(TextVertex);
    tvbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    tvbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer(&tvbDesc, nullptr, &m_textVertexBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC tcbDesc = {};
    tcbDesc.Usage = D3D11_USAGE_DEFAULT;
    tcbDesc.ByteWidth = sizeof(TextConstants);
    tcbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = m_device->CreateBuffer(&tcbDesc, nullptr, &m_textConstantBuffer);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void CubeRenderer::Resize(unsigned int width, unsigned int height)
{
    if (!m_swapChain || !m_device || !m_context)
    {
        return;
    }

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    m_renderTargetView = nullptr;
    m_depthStencilView = nullptr;

    HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr))
    {
        return;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
    {
        return;
    }

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr))
    {
        return;
    }

    D3D11_TEXTURE2D_DESC dsDesc = {};
    dsDesc.Width = width;
    dsDesc.Height = height;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer;
    hr = m_device->CreateTexture2D(&dsDesc, nullptr, &depthBuffer);
    if (FAILED(hr))
    {
        return;
    }

    hr = m_device->CreateDepthStencilView(depthBuffer.Get(), nullptr, &m_depthStencilView);
    if (FAILED(hr))
    {
        return;
    }

    m_width = width;
    m_height = height;
}

void CubeRenderer::Render()
{
    if (!m_device || !m_context || !m_panel)
    {
        return;
    }

    if (m_swapChain == nullptr)
    {
        CreateSwapChain();
        if (m_swapChain == nullptr)
        {
            return;
        }
        Resize(m_width, m_height);
        if (!m_renderTargetView || !m_depthStencilView)
        {
            return;
        }
    }

    if (!m_context || !m_vertexBuffer || !m_indexBuffer || !m_constantBuffer ||
        !m_vertexShader || !m_pixelShader || !m_inputLayout || !m_rasterizerState)
    {
        return;
    }

    float scaleX = m_panel->CompositionScaleX;
    float scaleY = m_panel->CompositionScaleY;

    unsigned int width = static_cast<unsigned int>(m_panel->ActualWidth * scaleX);
    unsigned int height = static_cast<unsigned int>(m_panel->ActualHeight * scaleY);

    if (width == 0 || height == 0)
    {
        return;
    }

    if (width != m_width || height != m_height)
    {
        Resize(width, height);
        if (!m_renderTargetView || !m_depthStencilView)
        {
            return;
        }
    }

    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    float clearColor[4] = { 0.12f, 0.14f, 0.18f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    m_context->RSSetViewports(1, &viewport);
    m_context->RSSetState(m_rasterizerState.Get());

    using namespace DirectX;

    XMMATRIX model = XMMatrixMultiply(XMMatrixRotationY(m_angle), XMMatrixRotationX(m_angle * 0.5f));
    model = XMMatrixMultiply(model, XMMatrixScaling(1.2f, 1.2f, 1.2f));

    XMVECTOR eye = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    XMVECTOR at = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(eye, at, up);

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspect, 0.1f, 100.0f);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    CubeConstants constants = {};
    XMStoreFloat4x4(&constants.model, model);
    XMStoreFloat4x4(&constants.viewProj, viewProj);

    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &constants, 0, 0);

    unsigned int stride = sizeof(CubeVertex);
    unsigned int offset = 0;

    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    m_context->DrawIndexed(36, 0, 0);

    RenderText();

    m_swapChain->Present(1, 0);

    static bool s_firstFrameLogged = false;
    if (!s_firstFrameLogged)
    {
        s_firstFrameLogged = true;
    }
}

void CubeRenderer::OnTick(Platform::Object^ sender, Platform::Object^ e)
{
    (void)sender;
    (void)e;

    m_angle += 0.03f;
    UpdateExitButton();
    Render();
}

void CubeRenderer::UpdateExitButton()
{
    if (m_width == 0 || m_height == 0)
    {
        return;
    }

    const float buttonScale = 4.0f;
    const float buttonWidth = 4.0f * kGlyphCellWidth * buttonScale;
    const float buttonHeight = kGlyphCellHeight * buttonScale;

    if (!m_exitInitialized)
    {
        m_exitInitialized = true;
        m_exitX = (static_cast<float>(m_width) - buttonWidth) * 0.5f;
        m_exitY = (static_cast<float>(m_height) - buttonHeight) * 0.5f;
        m_exitVX = 4.5f;
        m_exitVY = 3.25f;
    }

    m_exitX += m_exitVX;
    m_exitY += m_exitVY;

    if (m_exitX <= 0.0f)
    {
        m_exitX = 0.0f;
        m_exitVX = -m_exitVX;
    }
    else if (m_exitX >= static_cast<float>(m_width) - buttonWidth)
    {
        m_exitX = static_cast<float>(m_width) - buttonWidth;
        m_exitVX = -m_exitVX;
    }

    if (m_exitY <= 0.0f)
    {
        m_exitY = 0.0f;
        m_exitVY = -m_exitVY;
    }
    else if (m_exitY >= static_cast<float>(m_height) - buttonHeight)
    {
        m_exitY = static_cast<float>(m_height) - buttonHeight;
        m_exitVY = -m_exitVY;
    }

    if (m_exitTriggered || m_inputManager == nullptr || !m_inputManager->MousePressed)
    {
        return;
    }

    float mouseX = static_cast<float>(m_inputManager->MouseX);
    float mouseY = static_cast<float>(m_inputManager->MouseY);

    if (mouseX >= m_exitX && mouseX <= m_exitX + buttonWidth &&
        mouseY >= m_exitY && mouseY <= m_exitY + buttonHeight)
    {
        m_exitTriggered = true;
        if (m_leaveCallback != nullptr)
        {
            m_leaveCallback();
        }
    }
}

void CubeRenderer::RenderText()
{
    if (!m_context || !m_textVertexBuffer || !m_textVertexShader || !m_textPixelShader ||
        !m_textInputLayout || !m_textConstantBuffer || !m_fontTextureView || !m_fontSampler ||
        !m_textBlendState || !m_textDepthState)
    {
        return;
    }

    if (m_width == 0 || m_height == 0)
    {
        return;
    }

    static TextVertex s_vertices[2048];
    unsigned int vertexCount = 0;

    char line[192];

    float x = 8.0f;
    float y = 8.0f;
    float scale = 2.0f;

    const DirectX::XMFLOAT4 headerColor = { 1.0f, 0.95f, 0.4f, 1.0f };
    const DirectX::XMFLOAT4 bodyColor = { 0.85f, 0.9f, 1.0f, 1.0f };

    DrawTextIntoBuffer("InputManager", x, y, scale, headerColor, s_vertices, vertexCount, 2048);
    y += static_cast<float>(kGlyphCellHeight) * scale;

    sprintf_s(line, "listeners: %s", (m_inputManager != nullptr && m_inputManager->ListenersRegistered) ? "registered" : "not registered");
    DrawTextIntoBuffer(line, x, y, scale, bodyColor, s_vertices, vertexCount, 2048);
    y += static_cast<float>(kGlyphCellHeight) * scale;

    sprintf_s(line, "mouse: (%d, %d)", m_inputManager != nullptr ? m_inputManager->MouseX : 0, m_inputManager != nullptr ? m_inputManager->MouseY : 0);
    DrawTextIntoBuffer(line, x, y, scale, bodyColor, s_vertices, vertexCount, 2048);
    y += static_cast<float>(kGlyphCellHeight) * scale;

    sprintf_s(line, "lastKey: %d", m_inputManager != nullptr ? m_inputManager->LastKeyCode : 0);
    DrawTextIntoBuffer(line, x, y, scale, bodyColor, s_vertices, vertexCount, 2048);
    y += static_cast<float>(kGlyphCellHeight) * scale;

    if (m_gameParameters != nullptr)
    {
        const DirectX::XMFLOAT4 paramHeaderColor = { 0.3f, 1.0f, 0.6f, 1.0f };
        const DirectX::XMFLOAT4 paramBodyColor = { 0.85f, 1.0f, 0.85f, 1.0f };

        DrawTextIntoBuffer("GameParameters", x, y, scale, paramHeaderColor, s_vertices, vertexCount, 2048);
        y += static_cast<float>(kGlyphCellHeight) * scale;

        DrawParamLine("placeID", m_gameParameters->placeID, x, y, scale, paramBodyColor, s_vertices, vertexCount);
        DrawParamLine("instanceID", m_gameParameters->instanceID, x, y, scale, paramBodyColor, s_vertices, vertexCount);
        DrawParamLine("userID", m_gameParameters->userID, x, y, scale, paramBodyColor, s_vertices, vertexCount);
        DrawParamLine("accessCode", m_gameParameters->accessCode, x, y, scale, paramBodyColor, s_vertices, vertexCount);
        DrawParamLine("partyGuid", m_gameParameters->partyGuid, x, y, scale, paramBodyColor, s_vertices, vertexCount);
        DrawParamLine("browserTrackerID", m_gameParameters->browserTrackerID, x, y, scale, paramBodyColor, s_vertices, vertexCount);

        sprintf_s(line, "joinRequestType: %d", m_gameParameters->joinRequestType);
        DrawTextIntoBuffer(line, x, y, scale, paramBodyColor, s_vertices, vertexCount, 2048);
        y += static_cast<float>(kGlyphCellHeight) * scale;

        sprintf_s(line, "isPartyLeader: %s", m_gameParameters->isPartyLeader ? "true" : "false");
        DrawTextIntoBuffer(line, x, y, scale, paramBodyColor, s_vertices, vertexCount, 2048);
    }

    const DirectX::XMFLOAT4 exitColor = { 0.1f, 0.95f, 0.2f, 1.0f };
    DrawTextIntoBuffer("Exit", m_exitX, m_exitY, 4.0f, exitColor, s_vertices, vertexCount, 2048);

    if (vertexCount == 0)
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_context->Map(m_textVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr) || mapped.pData == nullptr)
    {
        return;
    }
    memcpy(mapped.pData, s_vertices, vertexCount * sizeof(TextVertex));
    m_context->Unmap(m_textVertexBuffer.Get(), 0);

    using namespace DirectX;

    TextConstants constants = {};
    XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 0.0f, 1.0f);
    XMStoreFloat4x4(&constants.viewProj, ortho);
    m_context->UpdateSubresource(m_textConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);

    m_context->OMSetBlendState(m_textBlendState.Get(), nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_textDepthState.Get(), 0);

    unsigned int stride = sizeof(TextVertex);
    unsigned int offset = 0;

    m_context->IASetVertexBuffers(0, 1, m_textVertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetInputLayout(m_textInputLayout.Get());
    m_context->VSSetShader(m_textVertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_textPixelShader.Get(), nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, m_textConstantBuffer.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, m_fontTextureView.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_fontSampler.GetAddressOf());

    m_context->Draw(vertexCount, 0);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    m_context->PSSetShaderResources(0, 1, nullSRV);
    ID3D11SamplerState* nullSampler[1] = { nullptr };
    m_context->PSSetSamplers(0, 1, nullSampler);
    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(nullptr, 0);
}
