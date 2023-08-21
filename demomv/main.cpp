#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dsound")

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dsound.h>
#include "meshdata.h"
#include <math.h>

#define COMPILE_SHADER_FROM_FILE

#define TITLE "demosandbox"

//#define COMPILE_SHADER_FROM_FILE

struct float3 { float x, y, z; };
struct matrix { float m[4][4]; };

int NUMBER_PARTICLES = 1;

const char* hlslShaderCode = R"(
cbuffer constants : register(b0)
{
    row_major float4x4 transform;
    row_major float4x4 projection;
    float3   lightvector;
}

struct vs_in
{
    float3 position : POS;
    float3 normal   : NOR;
    float2 texcoord : TEX;
    uint Instance   : SV_InstanceID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 normal :   NOR;
    float2 texcoord : TEX;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

#define BLOCK_SIZE 8
StructuredBuffer<float3> Input : register(t0);
RWStructuredBuffer<float3> Data : register(u0);

float hash13(float3 p3)
{
    p3 = frac(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return frac((p3.x + p3.y) * p3.z);
}

float3 hash31(float p)
{
    float3 p3 = frac(float3(p,p,p) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}

vs_out vs_main(vs_in input)
{
    float light = clamp(dot(mul(input.normal, transform), normalize(-lightvector)), 0.0f, 1.0f) * 0.8f + 0.2f;

    vs_out output;

    input.position *= 0.8;
    input.position += 0.1 * Input[input.Instance] + float3(0., 0., 0.);
    output.position = mul(float4(input.position, 1.0f), mul(transform, projection));
    output.texcoord = input.texcoord;
    output.normal = input.normal;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(0.5 * input.normal + 0.5,1.0);
    //return mytexture.Sample(mysampler, input.texcoord) * input.color;
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs_main(uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint GI : SV_GroupIndex)
{
    Data[DTid.x] = float3(DTid.x, 0.0, 0.0);
}
)";

#define MZK_DURATION    200
#define MZK_RATE        44100
#define MZK_NUMCHANNELS 2

#define MZK_NUMSAMPLES  (MZK_DURATION*MZK_RATE)
#define MZK_NUMSAMPLESC (MZK_NUMSAMPLES*MZK_NUMCHANNELS)
// put here your synth
void mzk_init(short* buffer)
{
    for (int i = 0; i < MZK_NUMSAMPLES; i++)
    {
        const float fl = i * (((i >> 12) | (i >> 8)) & (63 & (i >> 4)));
        const float fr = i * (((i >> 12) | (i >> 8)) & (63 & (i >> 4)));
        buffer[2 * i + 0] = fl;
        buffer[2 * i + 1] = fr;
    }
}

static WAVEFORMATEX format = { WAVE_FORMAT_PCM, MZK_NUMCHANNELS, MZK_RATE, MZK_RATE * MZK_NUMCHANNELS * sizeof(short), MZK_NUMCHANNELS * sizeof(short), 8 * sizeof(short) };
static DSBUFFERDESC bufferDesc1 = { sizeof(DSBUFFERDESC), DSBCAPS_PRIMARYBUFFER };
static DSBUFFERDESC bufferDesc2 = { sizeof(DSBUFFERDESC), DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS, MZK_NUMSAMPLESC * sizeof(short), NULL, &format, NULL };


///////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSA wndClass = { 0, DefWindowProcA, 0, 0, 0, 0, 0, 0, 0, TITLE };
    RegisterClassA(&wndClass);
    HWND window = CreateWindowExA(0, TITLE, TITLE, WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ID3D11Device1* device;
    baseDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));
    ID3D11DeviceContext1* deviceContext;
    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));
    ///////////////////////////////////////////////////////////////////////////////////////////////
    IDXGIDevice1* dxgiDevice;
    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgiDevice));
    IDXGIAdapter* dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    IDXGIFactory2* dxgiFactory;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));
    ///////////////////////////////////////////////////////////////////////////////////////////////
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; // use window width
    swapChainDesc.Height = 0; // use window height
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // prefer DXGI_SWAP_EFFECT_FLIP_DISCARD, see Minimal D3D11 pt2 
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;
    IDXGISwapChain1* swapChain;
    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr, nullptr, &swapChain);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ID3D11Texture2D* frameBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
    ID3D11RenderTargetView* frameBufferView;
    device->CreateRenderTargetView(frameBuffer, nullptr, &frameBufferView);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    frameBuffer->GetDesc(&depthBufferDesc); // copy from framebuffer properties
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* depthBuffer;
    device->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);
    ID3D11DepthStencilView* depthBufferView;
    device->CreateDepthStencilView(depthBuffer, nullptr, &depthBufferView);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ID3DBlob* vsBlob;

#ifdef COMPILE_SHADER_FROM_FILE
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
#else
    D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
#endif
    ID3D11VertexShader* vertexShader;
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = // float3 position, float3 normal, float2 texcoord, float3 color
    {
        { "POS", 0,           DXGI_FORMAT_R32G32B32_FLOAT,    0,                            0,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NOR", 0,           DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEX", 0,           DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT,           0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
    };
    ID3D11InputLayout* inputLayout;
    device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ID3DBlob* psBlob;
#ifdef COMPILE_SHADER_FROM_FILE
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
#else
    D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
#endif    
    ID3D11PixelShader* pixelShader;
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ID3DBlob* csBlob;
#ifdef COMPILE_SHADER_FROM_FILE
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "cs_main", "cs_5_0", 0, 0, &csBlob, nullptr);
#else
    D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "cs_main", "cs_5_0", 0, 0, &csBlob, nullptr);
#endif    
    ID3D11ComputeShader* computeShader;
    device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &computeShader);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ID3D11Buffer* pDataBuffer = nullptr;
    ID3D11ShaderResourceView* pDataBufferSRV = nullptr;
    ID3D11UnorderedAccessView* pDataBufferUAV = nullptr;
    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.ByteWidth = NUMBER_PARTICLES*3*sizeof(float);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = 3*sizeof(float);
    device->CreateBuffer(&buffer_desc, nullptr, &pDataBuffer);
    D3D11_SHADER_RESOURCE_VIEW_DESC srvbuffer_desc = {};
    srvbuffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    srvbuffer_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvbuffer_desc.Buffer.FirstElement = 0;
    srvbuffer_desc.Buffer.NumElements = buffer_desc.ByteWidth / buffer_desc.StructureByteStride;
    device->CreateShaderResourceView(pDataBuffer, &srvbuffer_desc, &pDataBufferSRV);
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavbuffer_desc = {};
    uavbuffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    uavbuffer_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavbuffer_desc.Buffer.FirstElement = 0;
    uavbuffer_desc.Buffer.NumElements = buffer_desc.ByteWidth / buffer_desc.StructureByteStride;
    device->CreateUnorderedAccessView(pDataBuffer, &uavbuffer_desc, &pDataBufferUAV);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D11_RASTERIZER_DESC1 rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    ID3D11RasterizerState1* rasterizerState;
    device->CreateRasterizerState1(&rasterizerDesc, &rasterizerState);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ID3D11SamplerState* samplerState;
    device->CreateSamplerState(&samplerDesc, &samplerState);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* depthStencilState;
    device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    struct Constants
    {
        matrix Transform;
        matrix Projection;
        float Time;
    };
    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(Constants) + 0xf & 0xfffffff0; // round constant buffer size to 16 byte boundary
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* constantBuffer;
    device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData = { VertexData };
    ID3D11Buffer* vertexBuffer;
    device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = sizeof(IndexData);
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData = { IndexData };
    ID3D11Buffer* indexBuffer;
    device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer); 
    ///////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT backgroundColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT stride = 8 * sizeof(float); // vertex size (11 floats: float3 position, float3 normal, float2 texcoord, float3 color)
    UINT offset = 0;
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(depthBufferDesc.Width), static_cast<float>(depthBufferDesc.Height), 0.0f, 1.0f };
    float w = viewport.Width / viewport.Height; // width (aspect ratio)
    float h = 1.0f;                             // height
    float n = 1.0f;                             // near
    float f = 9.0f;                             // far

    short* myMuzik;
    unsigned long        length;
    LPDIRECTSOUNDBUFFER  m_pSecondary;
    LPDIRECTSOUND        m_pDS;
    LPDIRECTSOUNDBUFFER  m_pPrimary;
    DirectSoundCreate(0, &m_pDS, 0);
    m_pDS->SetCooperativeLevel(window, DSSCL_PRIORITY);
    m_pDS->CreateSoundBuffer(&bufferDesc1, &m_pPrimary, NULL);
    m_pDS->CreateSoundBuffer(&bufferDesc2, &m_pSecondary, NULL);
    m_pPrimary->SetFormat(&format);
    m_pSecondary->Lock(0, MZK_NUMSAMPLESC * sizeof(short), (void**)&myMuzik, &length, NULL, NULL, NULL);
    mzk_init(myMuzik);
    m_pSecondary->Unlock(myMuzik, length, NULL, NULL);
    m_pSecondary->Play(0, 0, 0);

    //Global time
    float time = 0.0f;

    //Time autocompile
    float f_autocompilecounter = 0.0;

    while (true)
    {

        time += 0.016f;
        f_autocompilecounter += 0.016f;   
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_KEYDOWN) return 0;
            DispatchMessageA(&msg);
        }

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
        Constants* constants = reinterpret_cast<Constants*>(mappedSubresource.pData);
        constants->Transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 3, 1 };
        constants->Projection = { 2 * n / w, 0, 0, 0, 0, 2 * n / h, 0, 0, 0, 0, f / (f - n), 1, 0, 0, n * f / (n - f), 0 };
        constants->Time = time;
        deviceContext->Unmap(constantBuffer, 0);

        ID3D11ShaderResourceView* pViewnullptr = nullptr;
        ID3D11UnorderedAccessView* pUnorderedViewnullptr = nullptr;

        deviceContext->CSSetUnorderedAccessViews(0, 1, &pDataBufferUAV, nullptr);
        deviceContext->CSSetShader(computeShader, nullptr, 0);
        deviceContext->Dispatch(NUMBER_PARTICLES / 8, 1, 1);

        deviceContext->CSSetShaderResources(0, 1, &pViewnullptr);
        deviceContext->CSSetUnorderedAccessViews(0, 1, &pUnorderedViewnullptr, nullptr);

        deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);
        deviceContext->ClearDepthStencilView(depthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        //Compile when up
        if (f_autocompilecounter>1.0) {
#ifdef COMPILE_SHADER_FROM_FILE
            D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
#else
            D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
#endif
            device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
#ifdef COMPILE_SHADER_FROM_FILE
            D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
#else
            D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
#endif            device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
#ifdef COMPILE_SHADER_FROM_FILE
            D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "cs_main", "cs_5_0", 0, 0, &csBlob, nullptr);
#else
            D3DCompile(hlslShaderCode, strlen(hlslShaderCode), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
#endif            device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &computeShader);
            f_autocompilecounter = 0.0;
        }

        deviceContext->VSSetShaderResources(0, 1, &pViewnullptr);
        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->VSSetShaderResources(0, 1, &pDataBufferSRV);
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        deviceContext->RSSetViewports(1, &viewport);
        deviceContext->RSSetState(rasterizerState);

        deviceContext->PSSetShader(pixelShader, nullptr, 0);
        deviceContext->PSSetSamplers(0, 1, &samplerState);

        deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);
        deviceContext->OMSetDepthStencilState(depthStencilState, 0);
        deviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // use default blend mode (i.e. disable)
        deviceContext -> DrawIndexedInstanced(ARRAYSIZE(IndexData), NUMBER_PARTICLES, 0, 0, 0);

        swapChain->Present(1, 0);
    }
}
