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

struct GlobalShaderConstants
{
    float4x4 ProjMat;
    float4x4 ViewMat;
    float WorldTime;
};

//  CPU resources

float3 cameraPos;
float3 cameraDir;
float3 cameraRight;
float3 cameraUp;
float2 mousePos;
bool mouseClicked;
float gpuTime;

//  GPU resources
ID3D11Device1* device;

ID3D11Texture2D* frameBuffer;
ID3D11RenderTargetView* frameBufferView;
ID3D11Texture2D* depthBuffer;
ID3D11DepthStencilView* depthBufferView;

ID3DBlob* vsBlob;
ID3D11VertexShader* vertexShader;
ID3DBlob* psBlob;
ID3D11PixelShader* pixelShader;

ID3D11Buffer* vertexBuffer;
ID3D11Buffer* indexBuffer;
ID3D11Buffer* constantBuffer;

ID3D11ShaderResourceView* pViewnullptr = nullptr;
ID3D11UnorderedAccessView* pUnorderedViewnullptr = nullptr;

ID3D11Query* pQueryDisjoint;
ID3D11Query* pQueryBeginFrame;
ID3D11Query* pQueryEndFrame;


void CompileAllShaders()
{
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
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
    
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSA wndClass = { 0, DefWindowProcA, 0, 0, 0, 0, 0, 0, 0, TITLE };
    RegisterClassA(&wndClass);
    HWND window = CreateWindowExA(0, TITLE, TITLE, WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);

    baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&device);
    ID3D11DeviceContext1* deviceContext;
    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&deviceContext);

    IDXGIDevice1* dxgiDevice;
    device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
    IDXGIAdapter* dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    IDXGIFactory2* dxgiFactory;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    {
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
    IDXGISwapChain1* swapChain;
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
    device->CreateDepthStencilView(depthBuffer, nullptr, &depthBufferView);

    CompileAllShaders();

    ID3D11InputLayout* inputLayout;
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = // float3 position, float3 normal, float2 texcoord, float3 color
    {
        { "POS", 0,           DXGI_FORMAT_R32G32B32_FLOAT,    0,                            0,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NOR", 0,           DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEX", 0,           DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT,           0, D3D11_APPEND_ALIGNED_ELEMENT,   D3D11_INPUT_PER_VERTEX_DATA,   0 },
    };
    device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    D3D11_RASTERIZER_DESC1 rasterizerDesc = {};
    {
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_BACK;
    }
    ID3D11RasterizerState1* rasterizerState;
    device->CreateRasterizerState1(&rasterizerDesc, &rasterizerState);

    D3D11_SAMPLER_DESC samplerDesc = {};
    {
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    }
    ID3D11SamplerState* samplerState;
    device->CreateSamplerState(&samplerDesc, &samplerState);

    D3D11_BLEND_DESC blendDesc = {};
    {
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }
    ID3D11BlendState* blendState;
    device->CreateBlendState(&blendDesc, &blendState);
   
    CreateConstantsBuffers();
    CreateAllVertexIndexBuffers();

    float backgroundColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT stride = 8 * sizeof(float); // vertex size (11 floats: float3 position, float3 normal, float2 texcoord, float3 color)
    UINT offset = 0;
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)(depthBufferDesc.Width), (float)(depthBufferDesc.Height), 0.0f, 1.0f };  
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    //Global time
    float worldTime = 0.0f;

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    GlobalShaderConstants* constants = (GlobalShaderConstants*) mappedSubresource.pData;
    deviceContext->Unmap(constantBuffer, 0);

    //Imgui initialization stuff
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui::GetIO().FontGlobalScale = 1.5f;

    ImGui_ImplDX11_Init(device, deviceContext);
    ImGui_ImplWin32_Init(window);

    
    cameraPos = { 0.0f,2.0f,-30.0f };
    cameraDir = { 0.0f, 0.0f, 1.0f };
    cameraRight = { 0.0f,0.0f,0.0f };
    cameraUp = { 0.0f,0.0f,0.0f };
    mouseClicked = false;
    gpuTime = 0.0f;

    ID3D11Texture2D* copyDepthTexture;
    ID3D11ShaderResourceView* copyDepthSRV;

    D3D11_TEXTURE2D_DESC copyDepthDesc = {};
    copyDepthDesc.Width = depthBufferDesc.Width;
    copyDepthDesc.Height = depthBufferDesc.Height;
    copyDepthDesc.MipLevels = 1;
    copyDepthDesc.ArraySize = 1;
    copyDepthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // Use the format of your depth buffer
    copyDepthDesc.SampleDesc.Count = 1;
    copyDepthDesc.Usage = D3D11_USAGE_DEFAULT;
    copyDepthDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    device->CreateTexture2D(&copyDepthDesc, nullptr, &copyDepthTexture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // Use the format of your depth buffer
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(copyDepthTexture, &srvDesc, &copyDepthSRV);


    //Time querys
    D3D11_QUERY_DESC query_disjoint_description = {};
    query_disjoint_description.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    device->CreateQuery(&query_disjoint_description, &pQueryDisjoint);
    D3D11_QUERY_DESC query_timestamp_description = {};
    query_timestamp_description.Query = D3D11_QUERY_TIMESTAMP;
    device->CreateQuery(&query_timestamp_description, &pQueryBeginFrame);
    device->CreateQuery(&query_timestamp_description, &pQueryEndFrame);


    while (true)
    {

        deviceContext->Begin(pQueryDisjoint);
        deviceContext->End(pQueryBeginFrame);

        //Here some imgui logic
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(300, 150),ImGuiCond_FirstUseEver);
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.0f, 0.0f, 1.0f);
        ImGui::Begin("Inspector");
            ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
            ImGui::Text("Camera Right: (%.2f, %.2f, %.2f)", cameraRight.x, cameraRight.y, cameraRight.z);
            ImGui::Text("Camera Up: (%.2f, %.2f, %.2f)", cameraUp.x, cameraUp.y, cameraUp.z);
            ImGui::Text("Camera Forward: (%.2f, %.2f, %.2f)", cameraDir.x, cameraDir.y, cameraDir.z);
            ImGui::Text("GPU Time: %.2f ms", gpuTime);

        ImGui::End();

        ImGui::Begin("Depth Buffer", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        ImVec2 windowSize = ImVec2(0.3f*depthBufferDesc.Width, 0.3f*depthBufferDesc.Height);
        ImGui::SetWindowSize(windowSize);
        ImGui::Image((ImTextureID)copyDepthSRV, windowSize);
        ImGui::End();
        
        //End of imgui logic
        ImGui::Render();

        worldTime += 0.016f;

        //Update shader constants buffer
        constants->WorldTime = worldTime;
        constants->ViewMat = computeViewMatrix(cameraPos, cameraPos + cameraDir);
        constants->ProjMat = computeProjectionMatrix(0.5, 1.0f * viewport.Width / viewport.Height, 1.0f, 10.0f);
        //constants->ProjMat = computeOrthographicMatrix(20.0f * viewport.Width / viewport.Height, 20.0f , 1.0f, 100.0f);
        cameraRight = getRightVectorViewMatrix(constants->ViewMat);
        cameraUp = getUpVectorViewMatrix(constants->ViewMat);

        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (ImGui_ImplWin32_WndProcHandler(window, msg.message, msg.wParam, msg.lParam)) {
                continue; 
            }

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) return 0;
            if (msg.message == WM_KEYDOWN && msg.wParam == 'R')
            {
                CompileAllShaders();
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

        deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);
        deviceContext->ClearDepthStencilView(depthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShaderResources(0, 1, &pViewnullptr);
        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        deviceContext->RSSetViewports(1, &viewport);
        deviceContext->RSSetState(rasterizerState);

        deviceContext->PSSetShader(pixelShader, nullptr, 0);
        deviceContext->PSSetSamplers(0, 1, &samplerState);

        deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);
        deviceContext->DrawIndexed(ARRAYSIZE(IndexData), 0, 0);
        
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swapChain->Present(1, 0);

        deviceContext->End(pQueryEndFrame);
        deviceContext->End(pQueryDisjoint);


        //Compute GPU timing
        UINT64 StartTime = 0;
        while (deviceContext->GetData(pQueryBeginFrame, &StartTime, sizeof(StartTime), 0) != S_OK);
        UINT64 EndTime = 0;
        while (deviceContext->GetData(pQueryEndFrame, &EndTime, sizeof(EndTime), 0) != S_OK);
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData;
        while (deviceContext->GetData(pQueryDisjoint, &DisjointData, sizeof(DisjointData), 0) != S_OK);
        float Time = 0.0f;
        if (!DisjointData.Disjoint)
        {
            UINT64 Delta = EndTime - StartTime;
            float Frequency = static_cast<float>(DisjointData.Frequency);
            gpuTime = (Delta / Frequency) * 1000.0f;
        }

        deviceContext->CopyResource(copyDepthTexture, depthBuffer);


    }
}
