#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <math.h>
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include "meshdata.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <fstream>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define TITLE "demosandbox"

// vector math library
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };
struct float4x4 { float m[4][4]; };

float3 operator *(float a, float2 b) { return { a * b.x,a * b.y }; }
float3 operator *(float a, float3 b) { return { a * b.x,a * b.y,a * b.z }; }
float4 operator *(float a, float4 b) { return { a * b.x,a * b.y,a * b.z,a*b.w }; }
float2 operator +(float2 a, float2 b) { return { a.x + b.x,a.y + b.y }; }
float2 operator -(float2 a, float2 b) { return { a.x - b.x,a.y - b.y }; }
float3 operator +(float3 a, float3 b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
float3 operator -(float3 a, float3 b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
float4 operator +(float4 a, float4 b) { return { a.x + b.x,a.y + b.y,a.z + b.z }; }
float4 operator -(float4 a, float4 b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
float dot3( float3 a, float3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float dot4( float4 a, float4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
float len3( float3 a) { return sqrtf(dot3(a, a)); }
float len4( float4 a) { return sqrtf(dot4(a, a)); }
float3 normalize3( float3 a) { float l = len3(a); return { a.x / l, a.y / l, a.z / l }; }
float4 normalize4( float4 a) { float l = len4(a); return { a.x / l, a.y / l, a.z / l, a.w/l }; }
float3 cross( float3 a, float3 b) { return { a.y * b.z - a.z * b.y, -a.x * b.z + a.z * b.x, a.x * b.y - a.y * b.x }; }

// 3d graphics helper functions library
float4x4 computeProjectionMatrix(float fov, float aspectRatio, float znear, float zfar) 
{
    float4x4 projectionMatrix;

    float h = 1 / tanf(fov * 0.5);
    float w = h / aspectRatio;
    float a = zfar / (zfar - znear);
    float b = (-znear * zfar) / (zfar - znear);

    projectionMatrix.m[0][0] = w;
    projectionMatrix.m[0][1] = 0.0f;
    projectionMatrix.m[0][2] = 0.0f;
    projectionMatrix.m[0][3] = 0.0f;

    projectionMatrix.m[1][0] = 0.0f;
    projectionMatrix.m[1][1] = h;
    projectionMatrix.m[1][2] = 0.0f;
    projectionMatrix.m[1][3] = 0.0f;

    projectionMatrix.m[2][0] = 0.0f;
    projectionMatrix.m[2][1] = 0.0f;
    projectionMatrix.m[2][2] = 1.0f;
    projectionMatrix.m[2][3] = 1.0f;

    projectionMatrix.m[3][0] = 0.0f;
    projectionMatrix.m[3][1] = a;
    projectionMatrix.m[3][2] = b;
    projectionMatrix.m[3][3] = 0.0f;

    return projectionMatrix;
}
float4x4 computeOrthographicMatrix(float width, float height, float znear, float zfar) {
    float4x4 orthographicMatrix;

    float left = -width / 2.0f;
    float right = width / 2.0f;
    float bottom = -height / 2.0f;
    float top = height / 2.0f;

    orthographicMatrix.m[0][0] = 2.0f / width;
    orthographicMatrix.m[0][1] = 0.0f;
    orthographicMatrix.m[0][2] = 0.0f;
    orthographicMatrix.m[0][3] = 0.0f;

    orthographicMatrix.m[1][0] = 0.0f;
    orthographicMatrix.m[1][1] = 2.0f / height;
    orthographicMatrix.m[1][2] = 0.0f;
    orthographicMatrix.m[1][3] = 0.0f;

    orthographicMatrix.m[2][0] = 0.0f;
    orthographicMatrix.m[2][1] = 0.0f;
    orthographicMatrix.m[2][2] = 1.0f / (zfar - znear);
    orthographicMatrix.m[2][3] = 0.0f;

    orthographicMatrix.m[3][0] = 0.0f;
    orthographicMatrix.m[3][1] = 0.0f;
    orthographicMatrix.m[3][2] = -znear / (zfar - znear);
    orthographicMatrix.m[3][3] = 1.0f;

    return orthographicMatrix;
}
float4x4 computeViewMatrix(float3 positionCamera, float3 viewPoint) {

    float3 up = { 0.0f,1.0f,0.0f };
    float3 forward = normalize3(viewPoint-positionCamera);
    float3 right =  normalize3(cross(up, forward));
    float3 newUp =  normalize3(cross(forward, right));

    float4x4 viewMatrix = {
            right.x,    newUp.x,    forward.x,    0.0f,
            right.y,    newUp.y,    forward.y,    0.0f,
            right.z,    newUp.z,    forward.z,    0.0f,
            -dot3(right, positionCamera),   -dot3(newUp, positionCamera),  -dot3(forward, positionCamera),   1.0f
    };

    return viewMatrix;
}
float3 getRightVectorViewMatrix(float4x4 viewMatrix)
{
    return { viewMatrix.m[0][0],viewMatrix.m[1][0], viewMatrix.m[2][0] };
}
float3 getUpVectorViewMatrix(float4x4 viewMatrix)
{
    return { viewMatrix.m[0][1],viewMatrix.m[1][1], viewMatrix.m[2][1] };
}
float3 getForwardVectorViewMatrix(float4x4 viewMatrix)
{
    return { viewMatrix.m[0][2],viewMatrix.m[1][2], viewMatrix.m[2][2] };

}

//  CPU resources

float worldTime;
float3 cameraPos;
float3 cameraDir;
float3 cameraRight;
float3 cameraUp;
float2 mousePos;
float3 lightDir;
float3 lightColor;
float lightAngle1;
float lightAngle2;
bool mouseClicked;
bool shadowsActive;
bool voxelizationViewActive;
int voxelDim;
bool animSceneActive;

//  GPU resources
ID3D11Device1* device;
ID3D11DeviceContext1* deviceContext;
IDXGIAdapter* dxgiAdapter;
IDXGISwapChain1* swapChain;

ID3D11Texture2D* frameBuffer;
ID3D11RenderTargetView* frameBufferView;
ID3D11Texture2D* depthBuffer;
ID3D11DepthStencilView* depthBufferView;
ID3D11ShaderResourceView* depthBufferSRV;
ID3D11Texture2D* shadowMapBuffer;
ID3D11DepthStencilView* shadowMapView;
ID3D11ShaderResourceView* shadowMapSRV;
ID3D11Texture3D* voxelTexture;
ID3D11UnorderedAccessView* voxelUAV;
ID3D11ShaderResourceView* voxelSRV;

ID3D11VertexShader* vertexShader;
ID3D11PixelShader* pixelShader;
ID3D11VertexShader* shadowMapVertexShader;
ID3D11PixelShader* shadowMapPixelShader;
ID3D11VertexShader* voxelizationVertexShader;
ID3D11GeometryShader* voxelizationGeometryShader;
ID3D11PixelShader* voxelizationPixelShader;
ID3D11VertexShader* voxelizationViewerVertexShader;
ID3D11GeometryShader* voxelizationViewerGeometryShader;
ID3D11PixelShader* voxelizationViewerPixelShader;
ID3D11InputLayout* inputLayout;
ID3D11InputLayout* voxelLayout;


ID3D11Buffer* vertexBuffer;
ID3D11Buffer* indexBuffer;
ID3D11Buffer* constantBuffer;

ID3D11ShaderResourceView* pViewnullptr = nullptr;
ID3D11UnorderedAccessView* pUnorderedViewnullptr = nullptr;

D3D11_VIEWPORT viewportScene;
D3D11_VIEWPORT viewportVoxelize;

ID3D11RasterizerState1* sceneRasterizerState;
ID3D11RasterizerState1* sceneWireframeRasterizerState;
ID3D11RasterizerState1* voxelizeRasterizerState;
ID3D11DepthStencilState* sceneDepthStencilState;
ID3D11DepthStencilState* voxelizeDepthStencilState;


struct GlobalShaderConstants
{
    float4x4 ProjMat;
    float4x4 ViewMat;
    float4x4 LightProjMat;
    float4x4 LightViewMat;
    int VoxelDim;
    bool AnimScene;
    float WorldTime;
};
struct GPUProfiling
{
    ID3D11Query* gpuQueryDisjoint;
    ID3D11Query* gpuQueryPoints[20];
    char gpuQueryPointNames[20][256];
    float gpuQueryTimes[20];

    int numQueryPoints;

    GPUProfiling()
    {
        numQueryPoints = 0;
        D3D11_QUERY_DESC queryDesc;
        queryDesc.MiscFlags = 0;
        queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        device->CreateQuery(&queryDesc, &gpuQueryDisjoint);
        queryDesc.MiscFlags = 0;
        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
        for (int i = 0; i < 20; i++)
        {
            device->CreateQuery(&queryDesc, &gpuQueryPoints[i]);
            gpuQueryTimes[i] = 0.0f;
        }
    }

    void beginProfiling()
    {
        numQueryPoints = 0;
        deviceContext->Begin(gpuQueryDisjoint);
    }

    void addPoint(const char* name)
    {
        deviceContext->End(gpuQueryPoints[numQueryPoints]);
        strcpy_s(gpuQueryPointNames[numQueryPoints], name);
        numQueryPoints++;
    }

    void endProfiling()
    {
        deviceContext->End(gpuQueryPoints[numQueryPoints]);
        strcpy_s(gpuQueryPointNames[numQueryPoints], "End");
        deviceContext->End(gpuQueryDisjoint);
        numQueryPoints++;
    }

    void computeTimes()
    {
        UINT64 utimes[20];

        for (int i = 0; i < numQueryPoints; i++)
        {
            while (deviceContext->GetData(gpuQueryPoints[i], &utimes[i], sizeof(utimes[i]), 0) != S_OK);
        }

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointdata;
        while (deviceContext->GetData(gpuQueryDisjoint, &disjointdata, sizeof(disjointdata), 0) != S_OK);
        if (!disjointdata.Disjoint)
        {
            for (int i = 0; i < numQueryPoints - 1; i++) {
                UINT64 Delta = utimes[i + 1] - utimes[i];
                float Frequency = (float)disjointdata.Frequency;

                if (gpuQueryTimes[i] == 0.0f)
                {
                    gpuQueryTimes[i] = (Delta / Frequency) * 1000.0f;
                }
                else
                {
                    gpuQueryTimes[i] = 0.1f * (Delta / Frequency) * 1000.0f + 0.9f * gpuQueryTimes[i];
                }
            }
        }
    }

    void imguiTimes()
    {
        ImGui::Columns(2, "gpuprofiling_times");
        ImGui::SetColumnWidth(0, 200);
        ImGui::SetColumnWidth(1, 100.0f);

        for (int i = 0; i < numQueryPoints - 1; i++)
        {
            char buffer[64];

            ImGui::Text(gpuQueryPointNames[i]);

            ImGui::NextColumn();
            ImGui::Text(" %.2f ms", gpuQueryTimes[i]);

            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }
};

void CompileAllShadersAndCreateLayout()
{
    ID3DBlob* vsBlob; ID3DBlob* psBlob; ID3DBlob* gsBlob;

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = // float3 position, float3 normal, float2 texcoord, float3 color
    {
        { "POSITION",   0,    DXGI_FORMAT_R32G32B32_FLOAT,    0,                            0,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL",     0,    DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",   0,    DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 }
    };

    D3DCompileFromFile(L"shader.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    D3DCompileFromFile(L"shader.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    
    //Shadow maps shaders
    D3DCompileFromFile(L"shader.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "vs_main_light", "vs_5_0", 0, 0, &vsBlob, nullptr);
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &shadowMapVertexShader);
    D3DCompileFromFile(L"shader.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "ps_main_light", "ps_5_0", 0, 0, &psBlob, nullptr);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &shadowMapPixelShader);

    device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    //Voxelization shaders
    D3DCompileFromFile(L"voxelize.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &voxelizationVertexShader);
    D3DCompileFromFile(L"voxelize.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &voxelizationPixelShader);
    D3DCompileFromFile(L"voxelize.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "gs_main", "gs_5_0", 0, 0, &gsBlob, nullptr);
    device->CreateGeometryShader(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &voxelizationGeometryShader);

    device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &voxelLayout);

    //Voxelization viewers
    D3DCompileFromFile(L"voxelizeviewer.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &voxelizationViewerVertexShader);
    D3DCompileFromFile(L"voxelizeviewer.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &voxelizationViewerPixelShader);
    D3DCompileFromFile(L"voxelizeviewer.hlsl", nullptr, ((ID3DInclude*)(UINT_PTR)1), "gs_main", "gs_5_0", 0, 0, &gsBlob, nullptr);
    device->CreateGeometryShader(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &voxelizationViewerGeometryShader);

}
void CreateAllVertexIndexBuffers()
{
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData = { VertexData };
    device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = sizeof(IndexData);
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData = { IndexData };
    device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
}
void CreateConstantsBuffers()
{
    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(GlobalShaderConstants) + 0xf & 0xfffffff0; // round constant buffer size to 16 byte boundary
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
}

void GenerateVoxelResources()
{
    D3D11_TEXTURE3D_DESC voxelTextureDesc;
    {
        voxelTextureDesc.Width = voxelDim;
        voxelTextureDesc.Height = voxelDim;
        voxelTextureDesc.Depth = voxelDim;
        voxelTextureDesc.MipLevels = 0;
        voxelTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        voxelTextureDesc.Usage = D3D11_USAGE_DEFAULT;
        voxelTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
        voxelTextureDesc.CPUAccessFlags = 0;
        voxelTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    device->CreateTexture3D(&voxelTextureDesc, NULL, &voxelTexture);

    D3D11_UNORDERED_ACCESS_VIEW_DESC  voxelUAVDesc;
    {
        voxelUAVDesc.Format = voxelTextureDesc.Format;
        voxelUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        voxelUAVDesc.Texture3D.FirstWSlice = 0;
        voxelUAVDesc.Texture3D.MipSlice = 0;
        voxelUAVDesc.Texture3D.WSize = voxelDim;
        device->CreateUnorderedAccessView(voxelTexture, &voxelUAVDesc, &voxelUAV);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC voxelSRVDesc;
    {
        voxelSRVDesc.Format = voxelTextureDesc.Format;
        voxelSRVDesc.Texture3D.MipLevels = -1;
        voxelSRVDesc.Texture3D.MostDetailedMip = 0;
        voxelSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        device->CreateShaderResourceView(voxelTexture, &voxelSRVDesc, &voxelSRV);
    }

    deviceContext->GenerateMips(voxelSRV);
    viewportVoxelize = { 0.0f, 0.0f, (float)voxelDim, (float)voxelDim, 0.0f, 1.0f };

}
    
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    worldTime = 0.0f;
    cameraPos = { 17.0f,20.0f, 17.0f };
    cameraDir = { -0.5f, -0.5f, -0.5f };
    cameraRight = { 0.0f,0.0f,0.0f };
    cameraUp = { 0.0f,0.0f,0.0f };
    lightDir = { -0.5f, -0.5f, -0.5f };
    lightColor = { 1.0f, 1.0f, 1.0f };
    lightAngle1 = 0.2f;
    lightAngle2 = 0.6f;
    mouseClicked = false;
    shadowsActive = true;
    voxelizationViewActive = false;
    animSceneActive = false;
    voxelDim = 128;

    WNDCLASSA wndClass = { 0, DefWindowProcA, 0, 0, 0, 0, 0, 0, 0, TITLE };
    RegisterClassA(&wndClass);
    HWND window = CreateWindowExA(0, TITLE, TITLE, WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);

    baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&device);
    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&deviceContext);

    IDXGIDevice1* dxgiDevice;
    device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
    dxgiDevice->GetAdapter(&dxgiAdapter);
    IDXGIFactory2* dxgiFactory;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc; {
        swapChainDesc.Width = 0;
        swapChainDesc.Height = 0;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // prefer DXGI_SWAP_EFFECT_FLIP_DISCARD, see Minimal D3D11 pt2 
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;
    }
    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr, nullptr, &swapChain);

    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&frameBuffer);
    device->CreateRenderTargetView(frameBuffer, nullptr, &frameBufferView);

    D3D11_TEXTURE2D_DESC depthBufferDesc;
    frameBuffer->GetDesc(&depthBufferDesc);
    {
        depthBufferDesc.MipLevels = 1;
        depthBufferDesc.ArraySize = 1;
        depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthBufferDesc.SampleDesc.Count = 1;
        depthBufferDesc.SampleDesc.Quality = 0;
        depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthBufferDesc.CPUAccessFlags = 0;
        depthBufferDesc.MiscFlags = 0;

    }
    device->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);

    D3D11_DEPTH_STENCIL_VIEW_DESC depthBufferDSVDesc = {};
    depthBufferDSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(depthBuffer, &depthBufferDSVDesc, &depthBufferView);

    D3D11_SHADER_RESOURCE_VIEW_DESC depthBufferSRVdesc = {};
    depthBufferSRVdesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthBufferSRVdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depthBufferSRVdesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(depthBuffer, &depthBufferSRVdesc, &depthBufferSRV);

    D3D11_TEXTURE2D_DESC shadowmapDepthDesc = {};
    shadowmapDepthDesc.Width = depthBufferDesc.Width;
    shadowmapDepthDesc.Height = depthBufferDesc.Height;
    shadowmapDepthDesc.MipLevels = 1;
    shadowmapDepthDesc.ArraySize = 1;
    shadowmapDepthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    shadowmapDepthDesc.SampleDesc.Count = 1;
    shadowmapDepthDesc.Usage = D3D11_USAGE_DEFAULT;
    shadowmapDepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    device->CreateTexture2D(&shadowmapDepthDesc, nullptr, &shadowMapBuffer);

    D3D11_DEPTH_STENCIL_VIEW_DESC shadowmapDSVdesc = {};
    shadowmapDSVdesc.Format = DXGI_FORMAT_D32_FLOAT;
    shadowmapDSVdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(shadowMapBuffer, &shadowmapDSVdesc, &shadowMapView);

    D3D11_SHADER_RESOURCE_VIEW_DESC shadowmapSRVdesc = {};
    shadowmapSRVdesc.Format = DXGI_FORMAT_R32_FLOAT;
    shadowmapSRVdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shadowmapSRVdesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(shadowMapBuffer, &shadowmapSRVdesc, &shadowMapSRV);


    GenerateVoxelResources();

    ID3D11Buffer* voxelCounterBuffer;
    D3D11_BUFFER_DESC voxelCounterBufferDesc = {};
    {
        voxelCounterBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        voxelCounterBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        voxelCounterBufferDesc.ByteWidth = sizeof(UINT);
    }

    device->CreateBuffer(&voxelCounterBufferDesc, nullptr, &voxelCounterBuffer);

    ID3D11UnorderedAccessView* voxelCounterBufferUAV;
    D3D11_UNORDERED_ACCESS_VIEW_DESC voxelCounterBufferUAVDesc = {};
    {
        voxelCounterBufferUAVDesc.Format = DXGI_FORMAT_R32_UINT;
        voxelCounterBufferUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        voxelCounterBufferUAVDesc.Buffer.FirstElement = 0;
        voxelCounterBufferUAVDesc.Buffer.NumElements = 1;
    }
    device->CreateUnorderedAccessView(voxelCounterBuffer, &voxelCounterBufferUAVDesc, &voxelCounterBufferUAV);

    CompileAllShadersAndCreateLayout();

    D3D11_RASTERIZER_DESC1 sceneRasterizerDesc = {};
    {
        sceneRasterizerDesc.FillMode = D3D11_FILL_SOLID;
        sceneRasterizerDesc.CullMode = D3D11_CULL_BACK;
    }
    device->CreateRasterizerState1(&sceneRasterizerDesc, &sceneRasterizerState);
    D3D11_RASTERIZER_DESC1 voxelizeRasterizerDesc = {};
    {
        voxelizeRasterizerDesc.FillMode = D3D11_FILL_SOLID;
        voxelizeRasterizerDesc.CullMode = D3D11_CULL_NONE;
    }
    device->CreateRasterizerState1(&voxelizeRasterizerDesc, &voxelizeRasterizerState);
    D3D11_DEPTH_STENCIL_DESC voxelizeDepthStencilDesc = {};
    {
        voxelizeDepthStencilDesc.DepthEnable = false;
        voxelizeDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    }
    device->CreateDepthStencilState(&voxelizeDepthStencilDesc, &voxelizeDepthStencilState);
    D3D11_RASTERIZER_DESC1 sceneWireframeRasterizerDesc = {};
    {
        sceneWireframeRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
        sceneWireframeRasterizerDesc.CullMode = D3D11_CULL_NONE;
    }
    device->CreateRasterizerState1(&sceneWireframeRasterizerDesc, &sceneWireframeRasterizerState);
   
    CreateConstantsBuffers();
    CreateAllVertexIndexBuffers();

    float backgroundColor[4] = { 0.1f, 0.05f, 0.02f, 1.0f };
    UINT stride = 8 * sizeof(float); // vertex size (11 floats: float3 position, float3 normal, float2 texcoord, float3 color)
    UINT offset = 0;

    viewportScene = { 0.0f, 0.0f, (float)(depthBufferDesc.Width), (float)(depthBufferDesc.Height), 0.0f, 1.0f };  

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    GlobalShaderConstants* constants = (GlobalShaderConstants*) mappedSubresource.pData;
    deviceContext->Unmap(constantBuffer, 0);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("misc/fonts/DroidSans.ttf", 16.0f);

    ImGui::StyleColorsDark();
    ImGui::GetIO().FontGlobalScale = 1.15f;

    ImGui_ImplDX11_Init(device, deviceContext);
    ImGui_ImplWin32_Init(window);
   
    //Time querys
    GPUProfiling gpuProfiling;

    while (true)
    {
        //Main ImGui logic
        {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.0f, 0.0f, 1.0f);
            ImGui::Begin("Inspector");

            if (ImGui::CollapsingHeader("Camera info", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Columns(2, "camera_info");
                ImGui::SetColumnWidth(0, 200.0f);
                ImGui::SetColumnWidth(1, 200.0f);
                ImGui::Text("Position");
                ImGui::NextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", cameraPos.x, cameraPos.y, cameraPos.z);
                ImGui::NextColumn();
                ImGui::Text("Transform Right");
                ImGui::NextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", cameraRight.x, cameraRight.y, cameraRight.z);
                ImGui::NextColumn();
                ImGui::Text("Transform Up");
                ImGui::NextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", cameraUp.x, cameraUp.y, cameraUp.z);
                ImGui::NextColumn();
                ImGui::Text("Transform Forward");
                ImGui::NextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", cameraDir.x, cameraDir.y, cameraDir.z);
                ImGui::NextColumn();
                ImGui::Columns(1);

                ImGui::ColorEdit3("Background color", backgroundColor);

                static int cameraSelectedType = 1;
                const char* cameraTypes[] = { "Orthographic", "Perspective" };

                static float cameraortho_width = 20.0f;
                static float cameraortho_height = 20.0f;
                static float cameraortho_near = 1.0f;
                static float cameraortho_far = 100.0f;
                static float camerapers_fov = 0.5;
                static float camerapers_near = 1.0f;
                static float camerapers_far = 10.0f;

                ImGui::Combo("", &cameraSelectedType, cameraTypes, IM_ARRAYSIZE(cameraTypes));

                if (cameraSelectedType == 0)
                {
                    ImGui::DragFloat("Width", &cameraortho_width, 0.01f);
                    ImGui::DragFloat("Height", &cameraortho_height, 0.01f);
                    ImGui::DragFloat("Near", &cameraortho_near, 0.01f);
                    ImGui::DragFloat("Far", &cameraortho_far, 0.01f);

                    constants->ProjMat = computeOrthographicMatrix(cameraortho_width * viewportScene.Width / viewportScene.Height, cameraortho_height, cameraortho_near, cameraortho_far);
                }
                if (cameraSelectedType == 1)
                {
                    ImGui::DragFloat("FOV", &camerapers_fov, 0.01f);
                    ImGui::DragFloat("Near", &camerapers_near, 0.01f);
                    ImGui::DragFloat("Far", &camerapers_far, 0.01f);

                    constants->ProjMat = computeProjectionMatrix(camerapers_fov, viewportScene.Width / viewportScene.Height, camerapers_near, camerapers_far);
                }

            }
            if (ImGui::CollapsingHeader("Light info", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Columns(2, "light_info");
                ImGui::SetColumnWidth(0, 200.0f);
                ImGui::SetColumnWidth(1, 200.0f);
                ImGui::Text("Light direction");
                ImGui::NextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", lightDir.x, lightDir.y, lightDir.z);
                ImGui::Columns(1);
                ImGui::DragFloat("Azimuthal angle", &lightAngle1, 0.01f);
                ImGui::DragFloat("Zenithal angle", &lightAngle2, 0.01f);
                ImGui::Checkbox("Shadows enabled", &shadowsActive);
            }

            if (ImGui::CollapsingHeader("Voxelization", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::SliderInt("Voxel dimension", &voxelDim, 4, 128))
                {
                    GenerateVoxelResources();
                }
                ImGui::Checkbox("Voxelization view enabled", &voxelizationViewActive);           
                //ImGui::Text("Number voxels filled %d", numbervoxels);
            }

            if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Animated scene", &animSceneActive);
            }

            if (ImGui::CollapsingHeader("GPU Profiling", ImGuiTreeNodeFlags_DefaultOpen))
            {
                gpuProfiling.imguiTimes();
            }
            ImGui::End();

            ImGui::Begin("Light camera", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            ImVec2 windowSize = ImVec2(0.3f * shadowmapDepthDesc.Width, 0.3f * shadowmapDepthDesc.Height);
            ImGui::SetWindowSize(windowSize);
            ImGui::Image((ImTextureID)shadowMapSRV, windowSize);
            ImGui::End();

            ImGui::Render();
        }

        //Update shader constants buffer
        worldTime += 0.016f;
        lightDir = { -cosf(lightAngle1) * cosf(lightAngle2),-sinf(lightAngle2),-sinf(lightAngle1)*cosf(lightAngle2) };
        constants->WorldTime = worldTime;
        constants->ViewMat = computeViewMatrix(cameraPos, cameraPos + cameraDir);
        constants->LightViewMat = computeViewMatrix(-20.0f * lightDir, float3{ 0.0f,0.0f,0.0f });
        constants->LightProjMat = computeOrthographicMatrix(15.0f * viewportScene.Width / viewportScene.Height, 15.0f, 1.0f, 25.0f);
        constants->VoxelDim = voxelDim;
        constants->AnimScene = animSceneActive;

        cameraRight = getRightVectorViewMatrix(constants->ViewMat);
        cameraUp = getUpVectorViewMatrix(constants->ViewMat);

        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (ImGui_ImplWin32_WndProcHandler(window, msg.message, msg.wParam, msg.lParam)) {
                continue; 
            }

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) return 0;
            if (msg.message == WM_KEYDOWN && msg.wParam == 'Q')
            {
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == 'R')
            {
                CompileAllShadersAndCreateLayout();
            }

            //Basic logic for camera movement
            if (GetAsyncKeyState(VK_RBUTTON) & 0x8001) {
                // Obtener el estado actual de las teclas
                bool wKey = (GetAsyncKeyState('W') & 0x8001) != 0; // Comprobamos el estado actual y el de la tecla previamente presionada
                bool sKey = (GetAsyncKeyState('S') & 0x8001) != 0;
                bool aKey = (GetAsyncKeyState('A') & 0x8001) != 0;
                bool dKey = (GetAsyncKeyState('D') & 0x8001) != 0;

                float moveSpeed = 0.5f;

                // Actualizar la posición de la cámara basándose en las teclas presionadas
                float3 moveDirection = { 0.0f, 0.0f, 0.0f };

                if (wKey) moveDirection = moveDirection + cameraDir;
                if (sKey) moveDirection = moveDirection - cameraDir;
                if (aKey) moveDirection = moveDirection - cameraRight;
                if (dKey) moveDirection = moveDirection + cameraRight;

                cameraPos = cameraPos + moveSpeed * moveDirection;

                if (msg.message == WM_MOUSEMOVE)
                {

                    RECT clientRect;
                    GetClientRect(window, &clientRect);

                    float windowWidth = clientRect.right - clientRect.left;
                    float windowHeight = clientRect.bottom - clientRect.top;

                    const float sensitivity = 0.001f;

                    if (!mouseClicked)
                    {
                        mouseClicked = true;

                        float2 newMousePos = { windowWidth / 2.0f, windowHeight / 2.0f };
                        mousePos = { windowWidth / 2.0f, windowHeight / 2.0f };
                        float2 deltaMouse = newMousePos - mousePos;
                        float3 delta = deltaMouse.x * cameraRight - deltaMouse.y * cameraUp;
                        cameraDir = normalize3(cameraDir + sensitivity * delta);
                        SetCursorPos(windowWidth / 2.0f, windowHeight / 2.0f);
                    }
                    else
                    {
                        float2 newMousePos = { (float)LOWORD(msg.lParam), (float)HIWORD(msg.lParam) };
                        mousePos = { windowWidth / 2.0f, windowHeight / 2.0f };
                        float2 deltaMouse = newMousePos - mousePos;
                        const float sensitivity = 0.001f;
                        float3 delta = deltaMouse.x * cameraRight - deltaMouse.y * cameraUp;
                        cameraDir = normalize3(cameraDir + sensitivity * delta);
                        SetCursorPos(windowWidth / 2.0f, windowHeight / 2.0f);
                    }
                }
                else
                {
                    mouseClicked = false;
                }
            }

            DispatchMessageA(&msg);
        }

        gpuProfiling.beginProfiling();


        gpuProfiling.addPoint("Clear render targets");
        deviceContext->ClearDepthStencilView(shadowMapView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        deviceContext->ClearDepthStencilView(depthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);

        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
        deviceContext->GSSetConstantBuffers(0, 1, &constantBuffer);
        deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);

        gpuProfiling.addPoint("Shadow map render");
        deviceContext->OMSetRenderTargets(0, nullptr, shadowMapView);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShaderResources(0, 1, &pViewnullptr);
        deviceContext->VSSetShader(shadowMapVertexShader, nullptr, 0);
        deviceContext->GSSetShader(nullptr, nullptr, 0);
        deviceContext->PSSetShader(shadowMapPixelShader, nullptr, 0);

        deviceContext->RSSetViewports(1, &viewportScene);
        deviceContext->RSSetState(sceneRasterizerState);

        if (shadowsActive)
        {
            deviceContext->DrawIndexed(ARRAYSIZE(IndexData), 0, 0);
        }

        gpuProfiling.addPoint("Normal render");
        deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->GSSetShader(nullptr, nullptr, 0);
        deviceContext->PSSetShaderResources(0, 1, &shadowMapSRV);
        deviceContext->PSSetShader(pixelShader, nullptr, 0);

        deviceContext->RSSetViewports(1, &viewportScene);
        deviceContext->RSSetState(sceneRasterizerState);

        deviceContext->DrawIndexed(ARRAYSIZE(IndexData), 0, 0);

        gpuProfiling.addPoint("Voxelization");

        const float ZEROCOLOR[4] = { 0.0f,0.0f,0.0f,-1.0f };
        deviceContext->ClearUnorderedAccessViewFloat(voxelUAV, ZEROCOLOR);
        const void* pUAVs[] = { voxelUAV };
        deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 1, 1, (ID3D11UnorderedAccessView**)pUAVs, nullptr);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(voxelLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShader(voxelizationVertexShader, nullptr, 0);
        deviceContext->GSSetShader(voxelizationGeometryShader, nullptr, 0);
        deviceContext->PSSetShader(voxelizationPixelShader, nullptr, 0);

        deviceContext->RSSetViewports(1, &viewportVoxelize);
        deviceContext->RSSetState(voxelizeRasterizerState);

        deviceContext->DrawIndexed(ARRAYSIZE(IndexData), 0, 0);

        deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 0, nullptr, nullptr);
        deviceContext->GenerateMips(voxelSRV);

        if (voxelizationViewActive)
        {
            gpuProfiling.addPoint("Voxelization viewers");

            deviceContext->ClearDepthStencilView(depthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);
            deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);
            const float ZEROCOLOR[4] = { 0.0f,0.0f,0.0f,-1.0f };
            deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);

            deviceContext->VSSetShaderResources(1, 1, &voxelSRV);
            deviceContext->VSSetShader(voxelizationViewerVertexShader, nullptr, 0);
            deviceContext->GSSetShader(voxelizationViewerGeometryShader, nullptr, 0);
            deviceContext->PSSetShader(voxelizationViewerPixelShader, nullptr, 0);

            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
            deviceContext->IASetInputLayout(nullptr);

            deviceContext->RSSetViewports(1, &viewportScene);
            deviceContext->RSSetState(sceneRasterizerState);

            deviceContext->Draw(voxelDim * voxelDim * voxelDim, 0);
        }

        deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);

        gpuProfiling.endProfiling();

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swapChain->Present(1, 0);

        gpuProfiling.computeTimes();

    }
}
