#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <directxmath.h>
#include <DirectXCollision.h>
#include "DDSTextureLoader.h"
#include "resource.h"

#include <iostream>
#include <vector>
#include <sstream>

#include "structures.h"
#include "constants.h"

#include "TransformComponent.h"
#include "MeshComponent.h"
#include "CameraComponent.h"
#include "TextureRendererComponent.h"
#include "AnimationComponent.h"

#include "Entity.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "imgui/ImGuizmo.h"

using namespace std;

//--------------------------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------------------------

//Generic
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitRunTimeParameters();
HRESULT InitWorld(int width, int height);
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
float calculateDeltaTime();
void CenterMouseInWindow(HWND hWnd);
void CleanupDevice();

//Render functions
void RenderForwardScene();
void RenderDeferredScene();
void RenderLightingPass(ID3D11DeviceContext* context);
void BeginGeometryPass(ID3D11DeviceContext* context);
void RenderGeometryPass(ID3D11DeviceContext* context, float deltaTime);
void SaveRenderState(RenderState& state);
void ResetRenderState(const RenderState& state);
void setupLightForRender();

//Update functions
void UpdatePostProcessing();
void UpdateLight();

//UI
void RenderImGui();
void RenderImGuizmo();
void ForwardRenderingExampleSceneOne();
void DeferredRenderingExampleSceneOne();

//Mesh
void RenderQuad(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader);
void RenderAdditionalQuads(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader);
void RenderDOFQuad();

//G-Buffer functions
HRESULT CreateGBuffer(ID3D11Device* device, UINT width, UINT height);
HRESULT CreateGBufferViews(ID3D11Device* device);

//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------

//Generic Project Variables
int	g_viewWidth;
int	g_viewHeight;
HINSTANCE g_hInst = nullptr;
HWND  g_hWnd = nullptr;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
XMMATRIX g_Projection;

//Devices
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11Device1* g_pd3dDevice1 = nullptr;

//Contexts
ID3D11DeviceContext* g_pImmediateContext = nullptr;
ID3D11DeviceContext1* g_pImmediateContext1 = nullptr;

//Swap Chains
IDXGISwapChain* g_pSwapChain = nullptr;
IDXGISwapChain1* g_pSwapChain1 = nullptr;

//Render Target - Normal and Custom
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11RenderTargetView* g_pRTTRenderTargetView;
ID3D11ShaderResourceView* g_pRTTShaderResourceView;
ID3D11ShaderResourceView* m_quadTexture;
ID3D11Texture2D* g_pRTTRrenderTargetTexture;

//Depth Stencils
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11Texture2D* g_pDepthStencil = nullptr;

//Vertex and Pixel Shader - Normal and Cube
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11VertexShader* g_pQuadVS = nullptr;
ID3D11PixelShader* g_pQuadPS = nullptr;

//Input Layouts
ID3D11InputLayout* g_pVertexLayout = nullptr;
ID3D11InputLayout* g_pQuadLayout = nullptr;

//Constnt Buffers
ID3D11Buffer* g_pConstantBuffer = nullptr;
ID3D11Buffer* g_pLightConstantBuffer = nullptr;
ID3D11Buffer* g_pScreenQuadVB = nullptr;

//Entities
std::vector<Entity> g_Entities;
std::vector<Entity> g_LightEntities;

//Quad
SCREEN_VERTEX svQuad[20];



//--------------------------------------------------------------------------------------
// Texture Structures
//--------------------------------------------------------------------------------------

//Albedo
std::vector<std::wstring> defaultTextures =
{
    L"Resources\\stone.dds",
    L"Resources\\white.dds",
    L"Resources\\tex2.dds",
    L"Resources\\brick.dds",
    L"Resources\\black.dds",
    L"Resources\\gold.dds"
};

static const char* specularMapTextureNames[] =
{
    "Checkered Pattern Texture",
    "Smiley Face Texture",
    "Scratches Texture",
    "white Texture",
    "Black Texture"
};

static const char* defaultTextureNames[] =
{
    "Stone Texture",
    "White Texture",
    "Tex2 Texture",
    "Brick Texture",
    "Black Texture",
    "Gold Texture"
};

//Normal
std::vector<std::wstring> normalMapTextures =
{
    L"Resources\\NormalMapNothing.dds",
    L"Resources\\conenormal.dds",
    L"Resources\\bumpyNormal.dds",
    L"Resources\\stoneNormal.dds",
    L"Resources\\brickNormal.dds",
    L"Resources\\bunnyNormal.dds"
};

static const char* normalMapTextureNames[] =
{
    "No Normal Map",
    "Cone Normal Map",
    "Bumpy Normal Map",
    "Stone Normal Map",
    "Brick Normal Map",
    "Bunny Normal Map"
};

//Specular
std::vector<std::wstring> specularMapTextures =
{
    L"Resources\\Specular2.dds",
    L"Resources\\Specular.dds",
    L"Resources\\Specular3.dds",
    L"Resources\\white.dds",
    L"Resources\\black.dds"
};

//--------------------------------------------------------------------------------------
// Entity Structures
//--------------------------------------------------------------------------------------

//bunny model: https://clara.io/view/a56a852d-08e4-402d-b4df-25ee1f798eb0
static const char* objectMeshes[] =
{
    "Resources\\Cube-Working.obj",
    "Resources\\Bunny.obj",
    "Resources\\Monkey.obj",
    "Resources\\Plane2.obj",
    "Resources\\Prism.obj"
};

static const char* meshNames[] =
{
    "Cube",
    "Bunny",
    "Monkey",
    "Plane",
    "Prism"
};

static const char* entityTypeNames[] =
{
    "Game Object",
    "Light"
};

static const char* lightTypeNames[] =
{
    "Point",
    "Directional",
    "Spot"
};

XMFLOAT3 meshStartingPositions[4]
{
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    XMFLOAT3(4.0f, 1.0f, 0.0f),
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    XMFLOAT3(2.0f, 0.9f, 0.0f)
};

int selectedEntity;
bool lightSelected = false;

//--------------------------------------------------------------------------------------
// Post Processing
//--------------------------------------------------------------------------------------

ID3D11Buffer* g_pPostProcessingBuffer = nullptr;

bool renderQuad = true;
bool renderCubeAsRenderTarget = false;

bool g_enableGreyscale = 0.0f;

float g_greyscaleStrength = 1.0f;
float g_tintStrength = 1.0f;
float g_aberrationStrength = 0.0f;
float g_blurStrength = 0.0f;

float g_nearDistance = 0.1f;
float g_farDistance = 10.0f;


//--------------------------------------------------------------------------------------
// G Buffer
//--------------------------------------------------------------------------------------

struct GraphicsRenderTarget
{
    ID3D11Texture2D* texture = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11ShaderResourceView* shaderResourceView = nullptr;
};
GraphicsRenderTarget graphicsBuffer[6];
ID3D11SamplerState* m_pSamplerLinear;

bool useDeferredRendering;
int gbufferIndex;
int quadOneIndex, quadTwoIndex, quadThreeIndex;

//Geometry Pass
ID3D11VertexShader* g_pDeferredVertexShader = nullptr;
ID3D11PixelShader* g_pDeferredPixelShader = nullptr;

//Lighting Pass
ID3D11Texture2D* g_pLightingRrenderTargetTexture;
ID3D11RenderTargetView* g_pLightingTargetView = nullptr;
ID3D11PixelShader* g_pLightingPixelShader = nullptr;

//DOF
ID3D11PixelShader* g_pDofPixelShader = nullptr;

ID3D11Texture2D* dofTexture = nullptr;
ID3D11RenderTargetView* dofRenderTargetView = nullptr;
ID3D11ShaderResourceView* dofShaderResourceView = nullptr;