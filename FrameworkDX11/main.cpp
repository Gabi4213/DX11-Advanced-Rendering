//--------------------------------------------------------------------------------------
// File: main.cpp
//
// This application demonstrates animation using matrix transformations
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729722.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define _XM_NO_INTRINSICS_

#include "main.h"

int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Setup Dear ImGui context
    //IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/OpenSans/OpenSans-Regular.ttf", 18.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            if (useDeferredRendering) 
            {
                RenderDeferredScene();
            }
            else 
            {
                RenderForwardScene();
            }
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"lWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 1920, 1080};

	g_viewWidth = SCREEN_WIDTH;
	g_viewHeight = SCREEN_HEIGHT;

    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"lWindowClass", L"DirectX 11",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

HRESULT CompileShaderFromFile( const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if( FAILED(hr) )
    {
        if( pErrorBlob )
        {
            OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
            pErrorBlob->Release();
        }
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}

HRESULT CreateGBuffer(ID3D11Device* device, UINT width, UINT height)
{
    //Creates the differnt gbuffer elements
    HRESULT hr = S_OK;

    //Albedo
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;          // No MSAA for deferred by default
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&texDesc, nullptr, &graphicsBuffer[0].texture);
        if (FAILED(hr)) return hr;
    }

    //Normal
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // for higher precision
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&texDesc, nullptr, &graphicsBuffer[1].texture);
        if (FAILED(hr)) return hr;
    }

    //Position
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&texDesc, nullptr, &graphicsBuffer[2].texture);
        if (FAILED(hr)) return hr;
    }

    //Normal Map
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&texDesc, nullptr, &graphicsBuffer[3].texture);
        if (FAILED(hr)) return hr;
    }

    //Specular Map
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&texDesc, nullptr, &graphicsBuffer[4].texture);
        if (FAILED(hr)) return hr;
    }

    //Depth
    {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&texDesc, nullptr, &graphicsBuffer[5].texture);
        if (FAILED(hr)) return hr;
    }

    return hr;
}

HRESULT CreateGBufferViews(ID3D11Device* device) 
{
    //Creates the gbuffer views
    HRESULT hr = S_OK;

    for (int i = 0; i < 5; ++i)
    {
        //Render target views
        hr = device->CreateRenderTargetView(graphicsBuffer[i].texture, nullptr, &graphicsBuffer[i].renderTargetView);
        if (FAILED(hr)) return hr;

        //Shader resource views
        hr = device->CreateShaderResourceView(graphicsBuffer[i].texture, nullptr, &graphicsBuffer[i].shaderResourceView);
        if (FAILED(hr)) return hr;
    }

    //Depth stuff - made it seperate since values are different
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    hr = device->CreateRenderTargetView(graphicsBuffer[5].texture, &rtvDesc, &graphicsBuffer[5].renderTargetView);
    if (FAILED(hr)) return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(graphicsBuffer[5].texture, &srvDesc, &graphicsBuffer[5].shaderResourceView);
    if (FAILED(hr)) return hr;

    return hr;
}

HRESULT InitDevice()
{
    HRESULT hr = S_OK;
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

        if (hr == E_INVALIDARG)
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        }

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create device.", L"Error", MB_OK);
        return hr;
    }

    CreateGBuffer(g_pd3dDevice, width, height);
    CreateGBufferViews(g_pd3dDevice);

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2)
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
        if (SUCCEEDED(hr))
        {
            (void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;//  DXGI_FORMAT_R16G16B16A16_FLOAT;////DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create swapchain.", L"Error", MB_OK);
        return hr;
    }

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a back buffer.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a render target.", L"Error", MB_OK);
        return hr;
    }

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a depth / stencil texture.", L"Error", MB_OK);
        return hr;
    }

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
    {
        MessageBox(nullptr,
            L"Failed to create a depth / stencil view.", L"Error", MB_OK);
        return hr;
    }

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    //RENDER TARGET
    // 
    // Create texture description
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the texture
    hr = g_pd3dDevice->CreateTexture2D(&textureDesc, nullptr, &g_pRTTRrenderTargetTexture);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create render target texture.", L"Error", MB_OK);
        return hr;
    }

    // Create render target view
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    hr = g_pd3dDevice->CreateRenderTargetView(g_pRTTRrenderTargetTexture, &renderTargetViewDesc, &g_pRTTRenderTargetView);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create render target view.", L"Error", MB_OK);
        return hr;
    }

    //LIGHTING

    // Create texture description
    D3D11_TEXTURE2D_DESC lightingTextureDesc = {};
    lightingTextureDesc.Width = width;
    lightingTextureDesc.Height = height;
    lightingTextureDesc.MipLevels = 1;
    lightingTextureDesc.ArraySize = 1;
    lightingTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    lightingTextureDesc.SampleDesc.Count = 1;
    lightingTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    lightingTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    lightingTextureDesc.CPUAccessFlags = 0;
    lightingTextureDesc.MiscFlags = 0;

    // Create the texture
    hr = g_pd3dDevice->CreateTexture2D(&lightingTextureDesc, nullptr, &g_pLightingRrenderTargetTexture);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create render target texture.", L"Error", MB_OK);
        return hr;
    }

    // Create Lighting render target view
    D3D11_RENDER_TARGET_VIEW_DESC lightingRenderTargetViewDesc = {};
    lightingRenderTargetViewDesc.Format = textureDesc.Format;
    lightingRenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    lightingRenderTargetViewDesc.Texture2D.MipSlice = 0;

    hr = g_pd3dDevice->CreateRenderTargetView(g_pLightingRrenderTargetTexture, &lightingRenderTargetViewDesc, &g_pLightingTargetView);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create render target view.", L"Error", MB_OK);
        return hr;
    }

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    hr = g_pd3dDevice->CreateShaderResourceView(g_pRTTRrenderTargetTexture, &shaderResourceViewDesc, &g_pRTTShaderResourceView);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create shader resource view.", L"Error", MB_OK);
        return hr;
    } 

    //DOF
    D3D11_TEXTURE2D_DESC doftextureDesc = {};
    doftextureDesc.Width = width;
    doftextureDesc.Height = height;
    doftextureDesc.MipLevels = 1;
    doftextureDesc.ArraySize = 1;
    doftextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    doftextureDesc.SampleDesc.Count = 1;
    doftextureDesc.Usage = D3D11_USAGE_DEFAULT;
    doftextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    doftextureDesc.CPUAccessFlags = 0;
    doftextureDesc.MiscFlags = 0;

    // Create the texture
    hr = g_pd3dDevice->CreateTexture2D(&doftextureDesc, nullptr, &dofTexture);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create render target texture.", L"Error", MB_OK);
        return hr;
    }

    // Create render target view
    D3D11_RENDER_TARGET_VIEW_DESC dofrenderTargetViewDesc = {};
    dofrenderTargetViewDesc.Format = textureDesc.Format;
    dofrenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    dofrenderTargetViewDesc.Texture2D.MipSlice = 0;

    hr = g_pd3dDevice->CreateRenderTargetView(dofTexture, &dofrenderTargetViewDesc, &dofRenderTargetView);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create render target view.", L"Error", MB_OK);
        return hr;
    }

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC dofshaderResourceViewDesc = {};
    dofshaderResourceViewDesc.Format = textureDesc.Format;
    dofshaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    dofshaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    dofshaderResourceViewDesc.Texture2D.MipLevels = 1;

    hr = g_pd3dDevice->CreateShaderResourceView(dofTexture, &dofshaderResourceViewDesc, &dofShaderResourceView);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create shader resource view.", L"Error", MB_OK);
        return hr;
    }

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

	hr = InitRunTimeParameters();
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to initialise mesh.", L"Error", MB_OK);
		return hr;
	}

	hr = InitWorld(width, height);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to initialise world.", L"Error", MB_OK);
		return hr;
	}

    return S_OK;
}

HRESULT InitRunTimeParameters()
{
	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	HRESULT hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

    //Quad vertex shader
    ID3DBlob* pQuadVSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "QUAD_VS", "vs_4_0", &pQuadVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreateVertexShader(pQuadVSBlob->GetBufferPointer(), pQuadVSBlob->GetBufferSize(), nullptr, &g_pQuadVS);
    if (FAILED(hr))
    {
        pQuadVSBlob->Release();
        return hr;
    }

    // Compile the Deferred vertex shader
    ID3DBlob* pDeferredVSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "VSMain", "vs_4_0", &pDeferredVSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreateVertexShader(pDeferredVSBlob->GetBufferPointer(), pDeferredVSBlob->GetBufferSize(), nullptr, &g_pDeferredVertexShader);
    if (FAILED(hr))
    {
        pDeferredVSBlob->Release();
        return hr;
    }

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "EYEVECTORTS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "LIGHTVECTORTS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

    //Quad Layout
    D3D11_INPUT_ELEMENT_DESC quadLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "EYEVECTORTS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "LIGHTVECTORTS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT quadNumElements = ARRAYSIZE(quadLayout);

    hr = g_pd3dDevice->CreateInputLayout(quadLayout, quadNumElements, pQuadVSBlob->GetBufferPointer(), pQuadVSBlob->GetBufferSize(), &g_pQuadLayout);
    pQuadVSBlob->Release();
    if (FAILED(hr))
        return hr;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

    //Quad Pixel Shader
    ID3DBlob* pQuadPSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "QUAD_PS", "ps_4_0", &pQuadPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreatePixelShader(pQuadPSBlob->GetBufferPointer(), pQuadPSBlob->GetBufferSize(), nullptr, &g_pQuadPS);
    pQuadPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the Deferred pixel shader
    ID3DBlob* pDofPSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PSDOF", "ps_4_0", &pDofPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreatePixelShader(pDofPSBlob->GetBufferPointer(), pDofPSBlob->GetBufferSize(), nullptr, &g_pDofPixelShader);
    pDofPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the Deferred pixel shader
    ID3DBlob* pDeferredPSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PSMain", "ps_4_0", &pDeferredPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreatePixelShader(pDeferredPSBlob->GetBufferPointer(), pDeferredPSBlob->GetBufferSize(), nullptr, &g_pDeferredPixelShader);
    pDeferredPSBlob->Release();
    if (FAILED(hr))
        return hr;

    // Compile the Lighting pixel shader
    ID3DBlob* pLightingPSBlob = nullptr;
    hr = CompileShaderFromFile(L"shader.fx", "PSLIGHTING", "ps_4_0", &pLightingPSBlob);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreatePixelShader(pLightingPSBlob->GetBufferPointer(), pLightingPSBlob->GetBufferSize(), nullptr, &g_pLightingPixelShader);
    pLightingPSBlob->Release();
    if (FAILED(hr))
        return hr;

    //Quad array
    svQuad[0].pos = XMFLOAT3(-1.0f, 1.0f, 0.0f); 
    svQuad[0].tex = XMFLOAT2(0.0f, 0.0f);
    svQuad[1].pos = XMFLOAT3(1.0f, 1.0f, 0.0f);  
    svQuad[1].tex = XMFLOAT2(1.0f, 0.0f);
    svQuad[2].pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
    svQuad[2].tex = XMFLOAT2(0.0f, 1.0f);
    svQuad[3].pos = XMFLOAT3(1.0f, -1.0f, 0.0f); 
    svQuad[3].tex = XMFLOAT2(1.0f, 1.0f);

    //need to add width to, to keep aspect ratio
    float quadHeight = 0.5f; 

    for (int i = 1; i <= 4; ++i) 
    {
        int idx = i * 4; 
        float yOffset = 1.0f - (i - 1) * quadHeight;

        svQuad[idx].pos = XMFLOAT3(0.5f, yOffset, 0.0f);
        svQuad[idx + 1].pos = XMFLOAT3(1.0f, yOffset, 0.0f);
        svQuad[idx + 2].pos = XMFLOAT3(0.5f, yOffset - quadHeight, 0.0f);
        svQuad[idx + 3].pos = XMFLOAT3(1.0f, yOffset - quadHeight, 0.0f);

        svQuad[idx].tex = XMFLOAT2(0.0f, 0.0f);
        svQuad[idx + 1].tex = XMFLOAT2(1.0f, 0.0f);
        svQuad[idx + 2].tex = XMFLOAT2(0.0f, 1.0f);
        svQuad[idx + 3].tex = XMFLOAT2(1.0f, 1.0f);
    }

	// Create the constant buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;

    //Quad constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SCREEN_VERTEX) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = svQuad;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pScreenQuadVB);
    if (FAILED(hr))
        return hr;

    //Post Processing buffer
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = ((sizeof(PostProcessingParams) + 15) / 16) * 16;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pPostProcessingBuffer);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create post-processing buffer! HRESULT: " << hr << std::endl;
        return hr; // Handle buffer creation failure
    }

	// Create the light constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightConstantBuffer);
	if (FAILED(hr))
		return hr;

    //sample linear
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);

    //load quad texture
    CreateDDSTextureFromFile(g_pd3dDevice, L"Resources\\stone.dds", nullptr, &m_quadTexture);

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;  // Disable back-face culling
    rasterizerDesc.FrontCounterClockwise = TRUE;

    ID3D11RasterizerState* pRasterizerState = nullptr;
    hr = g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
    if (FAILED(hr)) return hr;

    //Set the rasterizer state
    g_pImmediateContext->RSSetState(pRasterizerState);

	return hr;
}

void setupLightForRender()
{
    g_LightEntities.resize(MAX_LIGHTS);

    LightPropertiesConstantBuffer lightProperties;
    lightProperties.EyePosition = XMFLOAT4(0.0f, 0.0f, 5.0f, 1.0f); // Example eye position

    for (int i = 0; i < MAX_LIGHTS; i++)
    {   
        LightComponent* lightComp = g_LightEntities[i].addComponent<LightComponent>();
       
        std::stringstream ss;
        ss << "Light " << i;
        std::string lightName = ss.str();
        g_LightEntities[i].setEntityName(lightName);

        lightComp->setLightType(0);


        lightComp->setEnabled(static_cast<int>(true));

        lightComp->setSpotAngle(XMConvertToRadians(45.0f));
        lightComp->setConstantAttenuation(1.0f);
        lightComp->setLinearAttenuation(1.0f);
        lightComp->setQuadraticAttenuation(1.0f);

        lightComp->setPosition(XMFLOAT4(0.0f + i * 2.0f, 0.0f, 3.0f, 1));
        lightComp->setColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
        lightComp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

        XMVECTOR LightDirection = XMVectorSet(-lightComp->getPosition().x, -lightComp->getPosition().y, -lightComp->getPosition().z, 0.0f);
        LightDirection = XMVector3Normalize(LightDirection);

        DirectX::XMFLOAT4 compLightDirection = lightComp->getDirection();
        XMStoreFloat4(&compLightDirection, LightDirection);

        lightProperties.Lights[i].Direction = compLightDirection;

        lightProperties.Lights[i] = lightComp->m_Light;
    }

    g_pImmediateContext->UpdateSubresource(g_pLightConstantBuffer, 0, nullptr, &lightProperties, 0, 0);
}

HRESULT InitWorld(int width, int height)
{
    //Creates the camera and its components
    g_Entities.resize(MAX_ENTITIES);

    g_Entities[0].setEntityName("Camera");

    CameraComponent* camComp = g_Entities[0].addComponent<CameraComponent>(XMFLOAT3(0, 0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f));
    camComp->SetBackgroundColor(XMVECTORF32{ 0.51f, 0.439f, 0.439f });

    TransformComponent* cameraTransComp = g_Entities[0].addComponent<TransformComponent>();
    cameraTransComp->setPosition(XMFLOAT3(0.0f, 4.4f, -14.5f));
    cameraTransComp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    cameraTransComp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

    AnimationComponent* cameraAnimComp = g_Entities[0].addComponent<AnimationComponent>(cameraTransComp);

    //Creates the gameobejcts and their components
    for (int i = 1; i < g_Entities.size(); i++)
    {
        std::stringstream ss;
        ss << "GameObject" << i;
        std::string objectName = ss.str();
        g_Entities[i].setEntityName(objectName);

        MeshComponent* meshComp = g_Entities[i].addComponent<MeshComponent>();

        meshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[i]);

        TransformComponent* transComp = g_Entities[i].addComponent<TransformComponent>();
        transComp->setPosition(meshStartingPositions[i]);
        transComp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
        transComp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

        TextureRendererComponent* textureComp = g_Entities[i].addComponent<TextureRendererComponent>();
  
        g_Entities[i].getComponent<TextureRendererComponent>()->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
        g_Entities[i].getComponent<TextureRendererComponent>()->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
        g_Entities[i].getComponent<TextureRendererComponent>()->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);


        AnimationComponent* animComp = g_Entities[i].addComponent<AnimationComponent>(transComp);
    }

    constexpr float fovAngleY = XMConvertToRadians(60.0f);
	g_Projection = XMMatrixPerspectiveFovLH(fovAngleY, width / (FLOAT)height, 1.0f, 100.0f);

    gbufferIndex = 0;
    useDeferredRendering = false;

    quadOneIndex = 0;
    quadTwoIndex = 2;
    quadThreeIndex = 3;

    setupLightForRender();

	return S_OK;
}

void CleanupDevice()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    for (int i = 1; i < g_Entities.size(); i++)
    {
        g_Entities[i].getComponent<MeshComponent>()->cleanup();
    }

    ID3D11RenderTargetView* nullViews[] = { nullptr };
    g_pImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);

    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
    if (g_pImmediateContext1) g_pImmediateContext1->Flush();
    g_pImmediateContext->Flush();

    if (g_pLightConstantBuffer)
        g_pLightConstantBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();

    //Quad
    if (g_pScreenQuadVB) g_pScreenQuadVB->Release();
    if (g_pQuadLayout) g_pQuadLayout->Release();
    if (g_pQuadVS) g_pQuadVS->Release();
    if (g_pQuadPS) g_pQuadPS->Release();

    //Deferred rendering
    if (g_pDeferredVertexShader) g_pDeferredVertexShader->Release();
    if (g_pDeferredPixelShader) g_pDeferredPixelShader->Release();

    //Lighting deferred rendering
    if (g_pLightingPixelShader) g_pLightingPixelShader->Release();
    if (g_pLightingTargetView) g_pLightingTargetView->Release();

    //Post Processing
    if (g_pPostProcessingBuffer) g_pPostProcessingBuffer->Release();

    ID3D11Debug* debugDevice = nullptr;
    g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDevice));

    if (g_pd3dDevice1) g_pd3dDevice1->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    // handy for finding dx memory leaks
    debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

    if (debugDevice)
        debugDevice->Release();
}

void CenterMouseInWindow(HWND hWnd)
{
    // Get the dimensions of the window
    RECT rect;
    GetClientRect(hWnd, &rect);

    // Calculate the center position
    POINT center;
    center.x = (rect.right - rect.left) / 2;
    center.y = (rect.bottom - rect.top) / 2;

    // Convert the client area point to screen coordinates
    ClientToScreen(hWnd, &center);

    // Move the cursor to the center of the screen
    SetCursorPos(center.x, center.y);
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    float movement = 0.2f;
    static bool mouseDown = false;

    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch( message )
    {

    case WM_KEYDOWN:

        XMFLOAT3 cameraPosition = g_Entities[0].getComponent<TransformComponent>()->getPosition();

        switch (wParam)
        {
        case 27:
            PostQuitMessage(0);
            break;
        case 'W':
            cameraPosition.z += 0.5f;
            g_Entities[0].getComponent<TransformComponent>()->setPosition(cameraPosition);
            break;
        case 'A':
            cameraPosition.x -= 0.5f;
            g_Entities[0].getComponent<TransformComponent>()->setPosition(cameraPosition);
            break;
        case 'S':
            cameraPosition.z -= 0.5f;
            g_Entities[0].getComponent<TransformComponent>()->setPosition(cameraPosition);
            break;
        case 'D':
            cameraPosition.x += 0.5f;
            g_Entities[0].getComponent<TransformComponent>()->setPosition(cameraPosition);
            break;
        case 'Q':
            cameraPosition.y += 0.5f;
            g_Entities[0].getComponent<TransformComponent>()->setPosition(cameraPosition);
            break;
        case 'E':
            cameraPosition.y -= 0.5f;
            g_Entities[0].getComponent<TransformComponent>()->setPosition(cameraPosition);
            break;
        }
        break;

    case WM_RBUTTONDOWN:
        mouseDown = true;
        break;
    case WM_RBUTTONUP:
        mouseDown = false;
        break;
    case WM_MOUSEMOVE:
    {
        if (!mouseDown)
        {
            break;
        }
        // Get the dimensions of the window
        RECT rect;
        GetClientRect(hWnd, &rect);

        // Calculate the center position of the window
        POINT windowCenter;
        windowCenter.x = (rect.right - rect.left) / 2;
        windowCenter.y = (rect.bottom - rect.top) / 2;

        // Convert the client area point to screen coordinates
        ClientToScreen(hWnd, &windowCenter);

        // Get the current cursor position
        POINTS mousePos = MAKEPOINTS(lParam);
        POINT cursorPos = { mousePos.x, mousePos.y };
        ClientToScreen(hWnd, &cursorPos);

        // Calculate the delta from the window center
        POINT delta;
        delta.x = cursorPos.x - windowCenter.x;
        delta.y = cursorPos.y - windowCenter.y;

        // Update the camera with the delta
        // (You may need to convert POINT to POINTS or use the deltas as is)
        g_Entities[0].getComponent<CameraComponent>()->UpdateLookAt({ static_cast<short>(delta.x), static_cast<short>(delta.y) });

        // Recenter the cursor
        SetCursorPos(windowCenter.x, windowCenter.y);
    }
    break;
   
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            CenterMouseInWindow(hWnd);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

void UpdateLight()
{
    //Update the light properties and syncs with shader
    LightPropertiesConstantBuffer lightProperties;
    lightProperties.EyePosition = XMFLOAT4(0, 0, 5, 1);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        LightComponent* lightComp = g_LightEntities[i].getComponent<LightComponent>();

        lightComp->setEnabled(static_cast<int>(true));

        lightComp->setSpotAngle(XMConvertToRadians(45.0f));
        lightComp->setConstantAttenuation(1.0f);
        lightComp->setLinearAttenuation(1.0f);
        lightComp->setQuadraticAttenuation(1.0f);

        XMVECTOR LightDirection = XMVectorSet(-lightComp->getPosition().x, -lightComp->getPosition().y, -lightComp->getPosition().z, 0.0f);
        LightDirection = XMVector3Normalize(LightDirection);

        DirectX::XMFLOAT4 compLightDirection = lightComp->getDirection();

        XMStoreFloat4(&compLightDirection, LightDirection);

        lightProperties.Lights[i].Direction = compLightDirection;

        lightProperties.Lights[i] = lightComp->m_Light;
    }

    g_pImmediateContext->UpdateSubresource(g_pLightConstantBuffer, 0, nullptr, &lightProperties, 0, 0);
}

float calculateDeltaTime()
{
    // Update our time
    static float deltaTime = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    deltaTime = (timeCur - timeStart) / 1000.0f;
    timeStart = timeCur;

    float FPS60 = 1.0f / 60.0f;
    static float cummulativeTime = 0;

    // cap the framerate at 60 fps 
    cummulativeTime += deltaTime;
    if (cummulativeTime >= FPS60) {
        cummulativeTime = cummulativeTime - FPS60;
    }
    else {
        return 0;
    }

    return deltaTime;
}

void ForwardRenderingExampleSceneOne()
{
    //set the render target stuff 
    renderCubeAsRenderTarget = false;
    renderQuad = true;

    //Example for forward rendering
    //plane
    MeshComponent* planeMeshComp = g_Entities[1].getComponent<MeshComponent>();
    TransformComponent* planeTranscomp = g_Entities[1].getComponent<TransformComponent>();
    TextureRendererComponent* planeTextureComp = g_Entities[1].getComponent<TextureRendererComponent>();

    AnimationComponent* planeAnimComp = g_Entities[1].getComponent<AnimationComponent>();
    planeAnimComp->stopAnimations();

    planeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[3]);

    planeTranscomp->setPosition(XMFLOAT3(0.0f, -0.52f, 5.6f));
    planeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    planeTranscomp->setScale(XMFLOAT3(3.0f,1.0f,3.0f));

    planeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    planeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
    planeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);


    //cube
    MeshComponent* cubeMeshComp = g_Entities[2].getComponent<MeshComponent>();
    TransformComponent* cubeTranscomp = g_Entities[2].getComponent<TransformComponent>();
    TextureRendererComponent* cubeTextureComp = g_Entities[2].getComponent<TextureRendererComponent>();

    AnimationComponent* cubeAnimComp = g_Entities[2].getComponent<AnimationComponent>();
    cubeAnimComp->stopAnimations();

    cubeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);

    cubeTranscomp->setPosition(XMFLOAT3(3.8f, 0.45f, 2.6f));
    cubeTranscomp->setRotation(XMFLOAT3(0.0f, -0.3f, 0.0f));
    cubeTranscomp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

    cubeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    cubeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[1]);
    cubeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);


    //Prism
    MeshComponent* prismMeshComp = g_Entities[3].getComponent<MeshComponent>();
    TransformComponent* prismTranscomp = g_Entities[3].getComponent<TransformComponent>();
    TextureRendererComponent* prismTextureComp = g_Entities[3].getComponent<TextureRendererComponent>();

    AnimationComponent* prismAnimComp = g_Entities[3].getComponent<AnimationComponent>();
    prismAnimComp->stopAnimations();

    prismMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[4]);

    prismTranscomp->setPosition(XMFLOAT3(-3.5f, 0.45f, -1.44f));
    prismTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    prismTranscomp->setScale(XMFLOAT3(3.0f, 3.0f, 3.0f));

    prismTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    prismTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[1]);
    prismTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);


    //Lights
    LightComponent* light1comp = g_LightEntities[1].getComponent<LightComponent>();
    LightComponent* light2comp = g_LightEntities[2].getComponent<LightComponent>();
    LightComponent* light3comp = g_LightEntities[3].getComponent<LightComponent>();

    light1comp->setPosition(XMFLOAT4(-3.28f, 1.7f, -2.43f, 1.0f));
    light2comp->setPosition(XMFLOAT4(4.0f, 0.48f, 1.96f, 1.0f));
    light3comp->setPosition(XMFLOAT4(-3.6f, 5.24f, -2.0f, 1.0f));

    light1comp->setColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setColor(XMFLOAT4(0.12f, 1.0f, 1.0f, 1.0f));
    light3comp->setColor(XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f));

    light1comp->setLightType(0);
    light1comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setLightType(0);
    light2comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light3comp->setLightType(0);
    light3comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void RenderTargetExampleScene()
{
    //set the render target stuff 
    renderCubeAsRenderTarget = true;
    renderQuad = false;

    //plane
    MeshComponent* planeMeshComp = g_Entities[1].getComponent<MeshComponent>();
    TransformComponent* planeTranscomp = g_Entities[1].getComponent<TransformComponent>();
    TextureRendererComponent* planeTextureComp = g_Entities[1].getComponent<TextureRendererComponent>();

    AnimationComponent* planeAnimComp = g_Entities[1].getComponent<AnimationComponent>();
    planeAnimComp->stopAnimations();

    planeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[3]);

    planeTranscomp->setPosition(XMFLOAT3(0.0f, -0.52f, 5.6f));
    planeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    planeTranscomp->setScale(XMFLOAT3(3.0f, 1.0f, 3.0f));

    planeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    planeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
    planeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);

    //spinning cube 
    MeshComponent* cubeMeshComp = g_Entities[2].getComponent<MeshComponent>();
    TransformComponent* cubeTranscomp = g_Entities[2].getComponent<TransformComponent>();
    TextureRendererComponent* cubeTextureComp = g_Entities[2].getComponent<TextureRendererComponent>();

    AnimationComponent* cubeAnimComp = g_Entities[2].getComponent<AnimationComponent>();
    cubeAnimComp->stopAnimations();

    cubeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);

    cubeTranscomp->setPosition(XMFLOAT3(-3.6f, 4.5f, 1.1f));
    cubeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    cubeTranscomp->setScale(XMFLOAT3(3.0f, 3.0f, 3.0f));

    cubeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[4]);
    cubeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
    cubeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[4]);


    XMFLOAT3 cubeRotation = XMFLOAT3(0.0f, 30.0f, 0.0f);
    cubeAnimComp->animateRotation(cubeRotation, 30.0f);

    //bunny
    MeshComponent* bunnyMeshComp = g_Entities[3].getComponent<MeshComponent>();
    TransformComponent* bunnyTranscomp = g_Entities[3].getComponent<TransformComponent>();
    TextureRendererComponent* bunnyTextureComp = g_Entities[3].getComponent<TextureRendererComponent>();

    AnimationComponent* buynnyAnimComp = g_Entities[3].getComponent<AnimationComponent>();
    buynnyAnimComp->stopAnimations();

    bunnyMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[1]);

    bunnyTranscomp->setPosition(XMFLOAT3(5.3f, -0.3f, 1.0f));
    bunnyTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    bunnyTranscomp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

    bunnyTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    bunnyTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[2]);
    bunnyTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);

    //Extra Cube
    //Expand the objects size
    if (g_Entities.size() < 5) 
    {
        g_Entities.resize(MAX_ENTITIES + 1);
        std::stringstream ss;
        ss << "GameObject" << g_Entities.size();
        std::string objectName = ss.str();
        g_Entities[4].setEntityName(objectName);

        MeshComponent* cube2MeshComp = g_Entities[4].addComponent<MeshComponent>();
        TransformComponent* cube2Transcomp = g_Entities[4].addComponent<TransformComponent>();
        TextureRendererComponent* cube2TextureComp = g_Entities[4].addComponent<TextureRendererComponent>();

        AnimationComponent* cube2AnimComp = g_Entities[4].addComponent<AnimationComponent>(cube2Transcomp);
    }

    MeshComponent* cube2MeshComp = g_Entities[4].getComponent<MeshComponent>();
    TransformComponent* cube2Transcomp = g_Entities[4].getComponent<TransformComponent>();
    TextureRendererComponent* cube2TextureComp = g_Entities[4].getComponent<TextureRendererComponent>();

    AnimationComponent* cube2AnimComp = g_Entities[4].getComponent<AnimationComponent>();
    cube2AnimComp->stopAnimations();

    cube2MeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);

    cube2Transcomp->setPosition(XMFLOAT3(1.0f, 4.9f, -0.24f));
    cube2Transcomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    cube2Transcomp->setScale(XMFLOAT3(2.0f, 2.0f, 2.0f));

    cube2TextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[4]);
    cube2TextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
    cube2TextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);

    XMFLOAT3 cube2Rotation = XMFLOAT3(0.0f, -30.0f, 0.0f);
    cube2AnimComp->animateRotation(cube2Rotation, 30.0f);

    //Lights
    LightComponent* light1comp = g_LightEntities[1].getComponent<LightComponent>();
    LightComponent* light2comp = g_LightEntities[2].getComponent<LightComponent>();
    LightComponent* light3comp = g_LightEntities[3].getComponent<LightComponent>();

    light1comp->setPosition(XMFLOAT4(-3.28f, 1.7f, -2.43f, 1.0f));
    light2comp->setPosition(XMFLOAT4(4.0f, 0.48f, 1.96f, 1.0f));
    light3comp->setPosition(XMFLOAT4(-3.6f, 5.24f, -2.0f, 1.0f));

    light1comp->setColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    light2comp->setColor(XMFLOAT4(0.12f, 1.0f, 1.0f, 1.0f));
    light3comp->setColor(XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f));

    light1comp->setLightType(1);
    light1comp->setDirection(XMFLOAT4(-8.8f, 0.0f, 0.0f, 1.0f));
    light2comp->setLightType(0);
    light2comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light3comp->setLightType(0);
    light3comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void DeferredRenderingExampleSceneOne()
{
    //Example for deferred rendering 
    //plane
    MeshComponent* planeMeshComp = g_Entities[1].getComponent<MeshComponent>();
    TransformComponent* planeTranscomp = g_Entities[1].getComponent<TransformComponent>();
    TextureRendererComponent* planeTextureComp = g_Entities[1].getComponent<TextureRendererComponent>();

    if (!g_Entities[1].getComponent<AnimationComponent>())
    {
        AnimationComponent* planeAnimComp = g_Entities[1].addComponent<AnimationComponent>(planeTranscomp);
    }

    AnimationComponent* planeAnimComp = g_Entities[1].getComponent<AnimationComponent>();
    planeAnimComp->stopAnimations();

    planeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[3]);

    planeTranscomp->setPosition(XMFLOAT3(0.0f, -0.52f, 5.6f));
    planeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    planeTranscomp->setScale(XMFLOAT3(3.0f, 1.0f, 3.0f));

    planeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    planeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
    planeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);

    //spinning cube 
    MeshComponent* cubeMeshComp = g_Entities[2].getComponent<MeshComponent>();
    TransformComponent* cubeTranscomp = g_Entities[2].getComponent<TransformComponent>();
    TextureRendererComponent* cubeTextureComp = g_Entities[2].getComponent<TextureRendererComponent>();

    AnimationComponent* cubeAnimComp = g_Entities[2].getComponent<AnimationComponent>();
    cubeAnimComp->stopAnimations();

    cubeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);

    cubeTranscomp->setPosition(XMFLOAT3(-3.6f, 4.5f, 1.1f));
    cubeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    cubeTranscomp->setScale(XMFLOAT3(3.0f, 3.0f, 3.0f));

    cubeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[4]);
    cubeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[2]);
    cubeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[3]);

    XMFLOAT3 cubeRotation = XMFLOAT3(0.0f, 30.0f, 0.0f);
    cubeAnimComp->animateRotation(cubeRotation, 30.0f);

    //bunny
    MeshComponent* bunnyMeshComp = g_Entities[3].getComponent<MeshComponent>();
    TransformComponent* bunnyTranscomp = g_Entities[3].getComponent<TransformComponent>();
    TextureRendererComponent* bunnyTextureComp = g_Entities[3].getComponent<TextureRendererComponent>();


    AnimationComponent* bunnyAnimComp = g_Entities[3].getComponent<AnimationComponent>();
    bunnyAnimComp->stopAnimations();

    bunnyMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[1]);

    bunnyTranscomp->setPosition(XMFLOAT3(5.3f, -0.3f, 1.0f));
    bunnyTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    bunnyTranscomp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

    bunnyTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    bunnyTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[2]);
    bunnyTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);
  
    //Lights
    LightComponent* light1comp = g_LightEntities[0].getComponent<LightComponent>();
    LightComponent* light2comp = g_LightEntities[1].getComponent<LightComponent>();
    LightComponent* light3comp = g_LightEntities[2].getComponent<LightComponent>();
    LightComponent* light4comp = g_LightEntities[3].getComponent<LightComponent>();

    light1comp->setPosition(XMFLOAT4(-1.3f, 4.2f, -1.24f, 1.0f));
    light2comp->setPosition(XMFLOAT4(-4.6f, 2.4f, -1.6f, 1.0f));
    light3comp->setPosition(XMFLOAT4(-3.6f, 6.27f, -0.8f, 1.0f));
    light4comp->setPosition(XMFLOAT4(4.2f, 4.9f, -0.24f, 1.0f));

    light1comp->setColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setColor(XMFLOAT4(0.12f, 1.0f, 1.0f, 1.0f));
    light3comp->setColor(XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f));
    light4comp->setColor(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));


    light1comp->setLightType(0);
    light1comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setLightType(0);
    light2comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light3comp->setLightType(0);
    light3comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light4comp->setLightType(0);
    light4comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void SpecularExampleScene() 
{
    //set the render target stuff 
    renderCubeAsRenderTarget = false;
    renderQuad = true;

    for (int i = 1; i < g_Entities.size(); i++)
    {
        TransformComponent* transComp = g_Entities[i].getComponent<TransformComponent>();

        transComp->setPosition(XMFLOAT3(0.0f, -20.0f, 0.0f));
        transComp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
        transComp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
    }


    //spinning cube 
    MeshComponent* cubeMeshComp = g_Entities[2].getComponent<MeshComponent>();
    TransformComponent* cubeTranscomp = g_Entities[2].getComponent<TransformComponent>();
    TextureRendererComponent* cubeTextureComp = g_Entities[2].getComponent<TextureRendererComponent>();
    AnimationComponent* cubeAnimComp = g_Entities[2].getComponent<AnimationComponent>();

    cubeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);

    cubeTranscomp->setPosition(XMFLOAT3(0.0f, 4.5f, 1.1f));
    cubeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    cubeTranscomp->setScale(XMFLOAT3(3.0f, 3.0f, 3.0f));

    cubeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    cubeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[0]);
    cubeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[0]);

    XMFLOAT3 cubeRotation = XMFLOAT3(0.0f, 30.0f, 0.0f);
    cubeAnimComp->animateRotation(cubeRotation, 30.0f);


    //Lights
    LightComponent* light1comp = g_LightEntities[0].getComponent<LightComponent>();
    LightComponent* light2comp = g_LightEntities[1].getComponent<LightComponent>();
    LightComponent* light3comp = g_LightEntities[2].getComponent<LightComponent>();
    LightComponent* light4comp = g_LightEntities[3].getComponent<LightComponent>();

    light1comp->setPosition(XMFLOAT4(-3.3f, 7.4f, -1.0f, 1.0f));
    light2comp->setPosition(XMFLOAT4(-1.3f, 5.4f, -1.0f, 1.0f));
    light3comp->setPosition(XMFLOAT4(0.3f, 3.4f, -1.0f, 1.0f));
    light4comp->setPosition(XMFLOAT4(2.3f, 1.4f, -1.0f, 1.0f));

    light1comp->setColor(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setColor(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
    light3comp->setColor(XMFLOAT4(0.0f, 0.85f, 1.0f, 1.0f));
    light4comp->setColor(XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f));

    light1comp->setLightType(0);
    light1comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setLightType(0);
    light2comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light3comp->setLightType(0);
    light3comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light4comp->setLightType(0);
    light4comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void NormalMapExampleScene()
{
    //set the render target stuff 
    renderCubeAsRenderTarget = false;
    renderQuad = true;

    for (int i = 1; i < g_Entities.size(); i++)
    {
        TransformComponent* transComp = g_Entities[i].getComponent<TransformComponent>();

        transComp->setPosition(XMFLOAT3(0.0f, -20.0f, 0.0f));
        transComp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
        transComp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
    }

    //spinning cube 
    MeshComponent* cubeMeshComp = g_Entities[2].getComponent<MeshComponent>();
    TransformComponent* cubeTranscomp = g_Entities[2].getComponent<TransformComponent>();
    TextureRendererComponent* cubeTextureComp = g_Entities[2].getComponent<TextureRendererComponent>();
    AnimationComponent* cubeAnimComp = g_Entities[2].getComponent<AnimationComponent>();

    cubeMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);

    cubeTranscomp->setPosition(XMFLOAT3(0.0f, 4.5f, 1.1f));
    cubeTranscomp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
    cubeTranscomp->setScale(XMFLOAT3(3.0f, 3.0f, 3.0f));

    cubeTextureComp->setAlbedoTexture(g_pd3dDevice, defaultTextures[0]);
    cubeTextureComp->setNormalTexture(g_pd3dDevice, normalMapTextures[1]);
    cubeTextureComp->setSpecularTexture(g_pd3dDevice, specularMapTextures[4]);

    XMFLOAT3 cubeRotation = XMFLOAT3(0.0f, 30.0f, 0.0f);
    cubeAnimComp->animateRotation(cubeRotation, 30.0f);

    //Lights
    LightComponent* light1comp = g_LightEntities[0].getComponent<LightComponent>();
    LightComponent* light2comp = g_LightEntities[1].getComponent<LightComponent>();
    LightComponent* light3comp = g_LightEntities[2].getComponent<LightComponent>();
    LightComponent* light4comp = g_LightEntities[3].getComponent<LightComponent>();

    light1comp->setPosition(XMFLOAT4(-3.3f, 7.4f, -1.8f, 1.0f));
    light2comp->setPosition(XMFLOAT4(-1.3f, 5.4f, -1.8f, 1.0f));
    light3comp->setPosition(XMFLOAT4(0.3f, 3.4f, -1.8f, 1.0f));
    light4comp->setPosition(XMFLOAT4(2.3f, 1.4f, -1.8f, 1.0f));

    light1comp->setColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    light2comp->setColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    light3comp->setColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    light4comp->setColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    light1comp->setLightType(0);
    light1comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light2comp->setLightType(0);
    light2comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light3comp->setLightType(0);
    light3comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    light4comp->setLightType(0);
    light4comp->setDirection(XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}

void RenderImGuizmo()   
{
    //ImGui Gizmo
    //Generic window stuff
    ImGui::SetNextWindowSize(ImVec2(1920, 1061));
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Advanced Real-Time Graphics Application - Gabriela Maczynska", nullptr, window_flags);

    ImGuizmo::BeginFrame();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    float windowWidth = (float)ImGui::GetWindowWidth();
    float windowHeight = (float)ImGui::GetWindowHeight();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

    //If entity is wrong for some reason
    if (selectedEntity < 0)
    {
        ImGui::End();
        ImGui::PopStyleColor(3);
        return;
    }

    //Light entity
    if (lightSelected)
    {
        int index = selectedEntity;

        XMMATRIX viewMatrix = g_Entities[0].getComponent<CameraComponent>()->GetViewMatrix(*g_Entities[0].getComponent<TransformComponent>());
        XMMATRIX projectionMatrix = g_Projection;

        XMFLOAT4 position = g_LightEntities[index].getComponent<LightComponent>()->getPosition();
        XMFLOAT3 rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);

        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
        XMMATRIX worldMatrix = rotationMatrix * translationMatrix;

        //Make gizmo actually work
        float matrix[16];
        XMStoreFloat4x4((XMFLOAT4X4*)matrix, worldMatrix);
        ImGuizmo::Manipulate(reinterpret_cast<float*>(&viewMatrix), reinterpret_cast<float*>(&projectionMatrix), ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, matrix);

        if (ImGuizmo::IsUsing())
        {
            //Postion
            XMMATRIX updatedMatrix = XMLoadFloat4x4((XMFLOAT4X4*)matrix);

            XMVECTOR newPosVector = updatedMatrix.r[3];
            XMFLOAT4 newPosition;
            XMStoreFloat4(&newPosition, newPosVector);

            g_LightEntities[index].getComponent<LightComponent>()->setPosition(newPosition);
        }

        ImGui::End();
        ImGui::PopStyleColor(3);
        return;
    }

    //Guizmo related data
    XMMATRIX viewMatrix = g_Entities[0].getComponent<CameraComponent>()->GetViewMatrix(*g_Entities[0].getComponent<TransformComponent>());
    XMMATRIX projectionMatrix = g_Projection;

    XMFLOAT3 position = g_Entities[selectedEntity].getComponent<TransformComponent>()->getPosition();
    XMFLOAT3 rotation = g_Entities[selectedEntity].getComponent<TransformComponent>()->getRotation();

    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX worldMatrix = rotationMatrix * translationMatrix;


    //Make gizmo actually work
    float matrix[16];
    XMStoreFloat4x4((XMFLOAT4X4*)matrix, worldMatrix);
    ImGuizmo::Manipulate(reinterpret_cast<float*>(&viewMatrix), reinterpret_cast<float*>(&projectionMatrix),ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, matrix);

    //Other entities that are'nt light
    if (ImGuizmo::IsUsing())
    {
        XMMATRIX updatedMatrix = XMLoadFloat4x4((XMFLOAT4X4*)matrix);

        XMVECTOR newPosVector = updatedMatrix.r[3];
        XMFLOAT3 newPosition;
        XMStoreFloat3(&newPosition, newPosVector);

        g_Entities[selectedEntity].getComponent<TransformComponent>()->setPosition(newPosition);
    }

    ImGui::End();
    ImGui::PopStyleColor(3);
}

void RenderImGui() 
{
    //ImGui Stuff
    //New frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    //Hierarchy
    ImGui::SetNextWindowSize(ImVec2(400, 300));
    ImGui::SetNextWindowPos(ImVec2(0, 24));

    //imgui color settings
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

    ImGui::Begin("Hierarchy", nullptr, windowFlags);
    ImGuizmo::BeginFrame();

    ImGuiTreeNodeFlags hierarchyFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

    if (ImGui::Button("Add New Game Object"))
    {
        //Expand the objects size
        g_Entities.resize(g_Entities.size() + 1);
        int currentSize = g_Entities.size() - 1;

        //Entity name
        std::stringstream ss;
        ss << "GameObject" << g_Entities.size() - 1;
        std::string objectName = ss.str();
        g_Entities[currentSize].setEntityName(objectName);

        //Add components
        MeshComponent* entityMeshComp = g_Entities[currentSize].addComponent<MeshComponent>();
        TransformComponent* entityTransComp = g_Entities[currentSize].addComponent<TransformComponent>();
        TextureRendererComponent* entityTextureComp = g_Entities[currentSize].addComponent<TextureRendererComponent>();
        AnimationComponent* entityAnimComp = g_Entities[currentSize].addComponent<AnimationComponent>(entityTransComp);

        //Set base values
        entityMeshComp->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[0]);
        entityTransComp->setPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
        entityTransComp->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
        entityTransComp->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
    }

    //Selecting the objectz
    if (ImGui::TreeNodeEx("Entities", hierarchyFlags))
    {
        for (size_t i = 0; i < g_Entities.size(); i++)
        {
            if (ImGui::Selectable(g_Entities[i].getEntityName().c_str(), selectedEntity == (int)i))
            {
                selectedEntity = static_cast<int>(i);
                lightSelected = false;
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Light Entities", hierarchyFlags))
    {
        for (size_t i = 0; i < g_LightEntities.size(); i++)
        {
            if (ImGui::Selectable(g_LightEntities[i].getEntityName().c_str(), selectedEntity == (int)(g_LightEntities.size())))
            {
                selectedEntity = i;       
                lightSelected = true;
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();

    //Inspector
    ImGui::SetNextWindowSize(ImVec2(400, 420));
    ImGui::SetNextWindowPos(ImVec2(0, 325));
    ImGui::Begin("Inspector", nullptr, windowFlags);
    ImGuiTreeNodeFlags headerTags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

    Entity* entity = nullptr;
    if (selectedEntity >= 0)
    {
        if (!lightSelected)
        {
            entity = &g_Entities[selectedEntity];
        }
        else 
        {
           entity = &g_LightEntities[selectedEntity];         
        }
    }

    if (entity)
    {
        if (lightSelected)
        {
            ImGui::Text("%s", g_LightEntities[selectedEntity].getEntityName().c_str());
        }
        else 
        {
            ImGui::Text("%s", g_Entities[selectedEntity].getEntityName().c_str());
        }

        //Transform
        if (TransformComponent* transform = entity->getComponent<TransformComponent>())
        {
            if (ImGui::CollapsingHeader("Transform Component", headerTags))
            {
                XMFLOAT3 pos = transform->getPosition();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f, -100.0f, 100.0f))
                {
                    transform->setPosition(pos);
                }

                XMFLOAT3 rot = transform->getRotation();
                if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f, -360.0f, 360.0f))
                {
                    transform->setRotation(rot);
                }

                XMFLOAT3 scale = transform->getScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, -360.0f, 360.0f))
                {
                    transform->setScale(scale);
                }

                if (ImGui::Button("Reset Position"))
                {
                    transform->setPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
                }

                ImGui::SameLine();

                if (ImGui::Button("Reset Rotation"))
                {
                    transform->setRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
                }

                ImGui::SameLine();

                if (ImGui::Button("Reset Scale"))
                {
                    transform->setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
                }
            }
        }

        //Camera
        if (CameraComponent* camera = entity->getComponent<CameraComponent>())
        {
            if (ImGui::CollapsingHeader("Camera Component", headerTags))
            {
                XMFLOAT3 lookDirection = camera->getLookDirection();
                if (ImGui::DragFloat3("Look Direction", &lookDirection.x, 0.1f, 0.0f, 100.0f))
                {
                    camera->setLookDirection(XMFLOAT3(lookDirection.x, lookDirection.y, 1.0f));
                }

                XMFLOAT3 up = camera->getUp();
                if (ImGui::DragFloat("Up", &up.x, 0.1f, 0.0f, 100.0f))
                {
                    camera->setUp(XMFLOAT3(up.x, 1.0f, 1.0f));
                }

                XMFLOAT3 bgColor;
                XMStoreFloat3(&bgColor, camera->GetBackgroundColor());
                if (ImGui::ColorEdit3("Background Color", &bgColor.x))
                {
                    camera->SetBackgroundColor(XMVECTORF32{ bgColor.x, bgColor.y, bgColor.z });
                }
            }
        }

        //Mesh
        if (MeshComponent* mesh = entity->getComponent<MeshComponent>())
        {
            if (ImGui::CollapsingHeader("Mesh Renderer Component", headerTags))
            {
                static int selectedMesh = 0;

                if (ImGui::Combo(("Model" + std::to_string(selectedEntity)).c_str(), &selectedMesh, meshNames, IM_ARRAYSIZE(meshNames)))
                {
                    mesh->initMesh(g_pd3dDevice, g_pImmediateContext, objectMeshes[selectedMesh]);
                }
            }
        }

        //Light
        if (LightComponent* light = entity->getComponent<LightComponent>())
        {
            if (ImGui::CollapsingHeader("Light Component", headerTags))
            {
                static int lightTypeIndex = 0;

                if (ImGui::Combo("Light Type", &lightTypeIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
                {
                    light->setLightType(lightTypeIndex);                  
                }

                XMFLOAT4 lightPos = light->getPosition();
                if (ImGui::DragFloat3("Light Position", &lightPos.x, 0.1f, -100.0f, 100.0f))
                {
                    light->setPosition(lightPos);
                }

                //directional
                if (lightTypeIndex == 1)
                {
                    XMFLOAT4 lightDirection = light->getDirection();
                    if (ImGui::DragFloat("Light Rotation", &lightDirection.x, 0.1f, -100.0f, 0.0f))
                    {
                        light->setDirection(lightDirection);
                    }
                }
                //oither lights
                else 
                {
                    XMFLOAT4 lightDirection = light->getDirection();
                    if (ImGui::DragFloat3("Light Rotation", &lightDirection.x, 0.1f, -10.0f, 0.0f))
                    {
                        light->setDirection(lightDirection);
                    }
                }

                XMFLOAT4 lightColor = light->getColor();
                if (ImGui::ColorEdit3("Light Color", &lightColor.x))
                {
                    light->setColor(lightColor);
                }
            }
        }

        //Animation
        if (AnimationComponent* animation = entity->getComponent<AnimationComponent>())
        {
            if (ImGui::CollapsingHeader("Animation Component", headerTags))
            {
                XMFLOAT3 pos = animation->getTargetPosition();
                XMFLOAT3 rot = animation->getTargetRotation();

                //duration
                float duration = animation->getDuration();
                ImGui::DragFloat("Duration", & duration, 0.1f, 1.0f, 10.0f);
                animation->setDuration(duration);

                //position adn rotation targets
                ImGui::DragFloat3("Target Position", &pos.x, 0.1f, -100.0f, 100.0f);
                ImGui::DragFloat3("Target Rotation", &rot.x, 0.1f, -100.0f, 100.0f);
                animation->setTargetPosition(pos);
                animation->setTargetRotation(rot);

                if(ImGui::Button("Animate"))
                {
                    animation->animatePosition(pos, duration);
                    animation->animateRotation(rot, duration);
                }

                ImGui::SameLine();

                if (ImGui::Button("Set Current Position"))
                {
                    animation->setTargetPosition(entity->getComponent<TransformComponent>()->getPosition());
                }
                ImGui::SameLine();

                if (ImGui::Button("Set Current Rotation"))
                {
                    animation->setTargetRotation(entity->getComponent<TransformComponent>()->getRotation());
                }
            }
        }
        //Texture
        if (TextureRendererComponent* texRenderer = entity->getComponent<TextureRendererComponent>())
        {
            if (ImGui::CollapsingHeader("Texture Renderer Component", headerTags))
            {
                static int selectedTextureIndex = 0;
                static int selectedNormalMapIndex = 0;
                static int selectedSpecularMapIndex = 0;

                if (ImGui::Combo(("Albedo Texture" + std::to_string(selectedEntity)).c_str(),&selectedTextureIndex, defaultTextureNames, IM_ARRAYSIZE(defaultTextureNames)))
                {
                    texRenderer->setAlbedoTexture(g_pd3dDevice, defaultTextures[selectedTextureIndex]);
                }

                if (ImGui::Combo(("Normal Map" + std::to_string(selectedEntity)).c_str(),&selectedNormalMapIndex, normalMapTextureNames, IM_ARRAYSIZE(normalMapTextureNames)))
                {
                    texRenderer->setNormalTexture(g_pd3dDevice, normalMapTextures[selectedNormalMapIndex]);
                }

                if (ImGui::Combo(("Specular Map" + std::to_string(selectedEntity)).c_str(),&selectedSpecularMapIndex, specularMapTextureNames, IM_ARRAYSIZE(specularMapTextureNames)))
                {
                    texRenderer->setSpecularTexture(g_pd3dDevice, specularMapTextures[selectedSpecularMapIndex]);
                }           
            }
        }
    }

    ImGui::End();

    //Settings
    ImGui::SetNextWindowSize(ImVec2(400, 300));
    ImGui::SetNextWindowPos(ImVec2(0, 745));
    ImGui::Begin("Settings", nullptr, 0);
    ImGuizmo::BeginFrame();
    
    if (ImGui::CollapsingHeader("Post Processing"))
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "-----------------Post Processing----------------");

        //Greyscale
        ImGui::Checkbox("Greyscale", &g_enableGreyscale);
        ImGui::SliderFloat("Strength##1", &g_greyscaleStrength, 0.0f, 1.0f);
    
        if (!useDeferredRendering) 
        {
            //Chromatic Abberration and Gaussian Blur
            ImGui::Text("Chromatic Aberration");
            ImGui::SliderFloat("Strength##2", &g_aberrationStrength, 0.0f, 0.1f);

            ImGui::Text("Gaussian Blur");
            ImGui::SliderFloat("Strength##3", &g_blurStrength, 0.0f, 100.0f);
        }

        if (useDeferredRendering)
        {
            //DOF   
            ImGui::Text("Depth of Field blur");

            ImGui::SliderFloat("Near Distance##3", &g_nearDistance, 0.1f, 50.0f);
            ImGui::SliderFloat("Far Distance##3", &g_farDistance, 10.0f, 50.0f);
        }
    }
    
    //Rendering Type
    if (ImGui::CollapsingHeader("Rendering Type"))
    {
        ImGui::Checkbox("Use Deferred Rendering", &useDeferredRendering);
    }

    //Scene Examples
    if (ImGui::CollapsingHeader("Scene Examples"))
    {
        if (!useDeferredRendering)
        {
            if (ImGui::Button("Forward Rendering Scene 1"))
            {
                ForwardRenderingExampleSceneOne();
            }
            if (ImGui::Button("Render Target Scene"))
            {
                RenderTargetExampleScene();
            }
        }
        else
        {
            if(ImGui::Button("Deffered Rendering Scene 1"))
            {
                DeferredRenderingExampleSceneOne();
            }
            if (ImGui::Button("Specular Scene"))
            {
                SpecularExampleScene();
            }
            if (ImGui::Button("NormalMap Scene"))
            {
                NormalMapExampleScene();
            }
        }
    }
    
    //Forward Rendering options
    if (!useDeferredRendering)
    {
        if (ImGui::CollapsingHeader("Forward Rendering Options"))
        {
            ImGui::Text("Renders a quad with its texture as the render targer");
            ImGui::Checkbox("Render Quad", &renderQuad);
            ImGui::NewLine();
    
            ImGui::Text("If you want to render the cube as the render target");
            ImGui::Text("Make sure to disable render quad above!");
            ImGui::Checkbox("Render Cube as Render Target", &renderCubeAsRenderTarget);
        }
    }
    
    //Deffered rendering options
    if (useDeferredRendering)
    {
        if (ImGui::CollapsingHeader("Deferred Rendering"))
        {
            //Quad 1
            ImGui::Text("Quad 1 Output: ");
            ImGui::SliderInt(" ##1", &quadOneIndex, 0, 5);
    
            switch (quadOneIndex)
            {
            case 0:
                ImGui::Text("Albedo G-Buffer");
                break;
            case 1:
                ImGui::Text("Normal Map G-Buffer");
                break;
            case 2:
                ImGui::Text("Normal G-Buffer");
                break;
            case 3:
                ImGui::Text("Position G-Buffer");
                break;
            case 4:
                ImGui::Text("Specular G-Buffer");
                break;
            case 5:
                ImGui::Text("Depth G-Buffer");
                break;
            }
            ImGui::NewLine();
    
            //Quad 2
            ImGui::Text("Quad 2 Output: ");
            ImGui::SliderInt(" ##2", &quadTwoIndex, 0, 5);
    
            switch (quadTwoIndex)
            {
            case 0:
                ImGui::Text("Albedo G-Buffer");
                break;
            case 1:
                ImGui::Text("Normal Map G-Buffer");
                break;
            case 2:
                ImGui::Text("Normal G-Buffer");
                break;
            case 3:
                ImGui::Text("Position G-Buffer");
                break;
            case 4:
                ImGui::Text("Specular G-Buffer");
                break;
            case 5:
                ImGui::Text("Depth G-Buffer");
                break;
            }
            ImGui::NewLine();
    
            //Quad 3
            ImGui::Text("Quad 3 Output: ");
            ImGui::SliderInt(" ##3", &quadThreeIndex, 0, 5);
    
            switch (quadThreeIndex)
            {
            case 0:
                ImGui::Text("Albedo G-Buffer");
                break;
            case 1:
                ImGui::Text("Normal Map G-Buffer");
                break;
            case 2:
                ImGui::Text("Normal G-Buffer");
                break;
            case 3:
                ImGui::Text("Position G-Buffer");
                break;
            case 4:
                ImGui::Text("Specular G-Buffer");
                break;
            case 5:
                ImGui::Text("Depth G-Buffer");
                break;
            }
        }
    }    
  
    ImGui::End();
    ImGui::PopStyleColor(4);
}

void SaveRenderState(RenderState& state)
{
    //Saves the render state
    g_pImmediateContext->IAGetInputLayout(&state.lastLayout);
    g_pImmediateContext->IAGetVertexBuffers(0, 1, &state.lastBuffer, &state.lastStride, &state.lastOffset);
    g_pImmediateContext->IAGetPrimitiveTopology(&state.lastTopology);
}

void ResetRenderState(const RenderState& state)
{
    //Resets the render state
    g_pImmediateContext->IASetInputLayout(state.lastLayout);
    g_pImmediateContext->IASetVertexBuffers(0, 1, &state.lastBuffer, &state.lastStride, &state.lastOffset);
    g_pImmediateContext->IASetPrimitiveTopology(state.lastTopology);

    if (state.lastLayout) state.lastLayout->Release();
    if (state.lastBuffer) state.lastBuffer->Release();
}

void RenderQuad(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader)
{
    //Renders a fullscreen quad
    RenderState state;
    SaveRenderState(state);

    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);

    UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;

    g_pImmediateContext->UpdateSubresource(g_pScreenQuadVB, 0, nullptr, &svQuad[0], 0, 0);
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pScreenQuadVB, &stride, &offset);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    g_pImmediateContext->VSSetShader(vertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(pixelShader, nullptr, 0);

    g_pImmediateContext->Draw(4, 0);

    ResetRenderState(state);
}

void RenderDOFQuad()
{
    //Renders the specific quad that renders depth of field effect
    RenderState state;
    SaveRenderState(state);

    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);

    UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;

    for (int i = 4; i < 5; ++i)
    {
        g_pImmediateContext->UpdateSubresource(g_pScreenQuadVB, 0, nullptr, &svQuad[i * 4], 0, 0);

        g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pScreenQuadVB, &stride, &offset);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pDofPixelShader, nullptr, 0);

        switch (i)
        {
        case 4:
            g_pImmediateContext->PSSetShaderResources(0, 1, &graphicsBuffer[0].shaderResourceView);
            break;
        }
        g_pImmediateContext->Draw(4, 0);
    }

    ResetRenderState(state);
}

void RenderAdditionalQuads(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader) 
{
    //Renders full screen quad
    RenderState state;
    SaveRenderState(state);

    g_pImmediateContext->IASetInputLayout(g_pQuadLayout);

    UINT stride = sizeof(SCREEN_VERTEX);
    UINT offset = 0;

    g_pImmediateContext->UpdateSubresource(g_pScreenQuadVB, 0, nullptr, &svQuad[0], 0, 0);
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pScreenQuadVB, &stride, &offset);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    g_pImmediateContext->VSSetShader(vertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(pixelShader, nullptr, 0);

    ID3D11ShaderResourceView* rtvs[6] =
    {
        graphicsBuffer[0].shaderResourceView,
        graphicsBuffer[1].shaderResourceView,
        graphicsBuffer[2].shaderResourceView,
        graphicsBuffer[3].shaderResourceView,
        graphicsBuffer[4].shaderResourceView,
        graphicsBuffer[5].shaderResourceView
    };

    g_pImmediateContext->PSSetShaderResources(0, 6, rtvs);

    g_pImmediateContext->Draw(4, 0);

    //Renders the remaining small quads
    for (int i = 1; i < 4; ++i) 
    {
        g_pImmediateContext->UpdateSubresource(g_pScreenQuadVB, 0, nullptr, &svQuad[i * 4], 0, 0);

        g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pScreenQuadVB, &stride, &offset);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        g_pImmediateContext->VSSetShader(g_pQuadVS, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pQuadPS, nullptr, 0);

        switch (i) 
        {
            case 1:
                g_pImmediateContext->PSSetShaderResources(0, 1, &graphicsBuffer[quadOneIndex].shaderResourceView);
                break;
            case 2:
                g_pImmediateContext->PSSetShaderResources(0, 1, &graphicsBuffer[quadTwoIndex].shaderResourceView);
                break;
            case 3:
                g_pImmediateContext->PSSetShaderResources(0, 1, &graphicsBuffer[quadThreeIndex].shaderResourceView);
                break;
        }
        g_pImmediateContext->Draw(4, 0);
    }

    ResetRenderState(state);
}

void UpdatePostProcessing() 
{
    //Updates post processing params so that it syncs wiht the shader
    PostProcessingParams postProcessingParams;

    postProcessingParams.greyscaleStrength = g_greyscaleStrength;
    postProcessingParams.enableGreyscale = g_enableGreyscale ? 1.0f : 0.0f;

    postProcessingParams.aberrationStrength = g_aberrationStrength;

    postProcessingParams.blurStrength = g_blurStrength;

    postProcessingParams.nearDistance = g_nearDistance;
    postProcessingParams.farDistance = g_farDistance;

    D3D11_MAPPED_SUBRESOURCE map;
    HRESULT hr = g_pImmediateContext->Map(g_pPostProcessingBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create map.", L"Error", MB_OK);
    }

    memcpy(map.pData, &postProcessingParams, sizeof(PostProcessingParams));

    g_pImmediateContext->Unmap(g_pPostProcessingBuffer, 0);
    g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pPostProcessingBuffer);
}

void RenderForwardScene()
{
    //Renders the scene using forward rendering
    float t = calculateDeltaTime();
    if (t == 0.0f)
        return;

    RenderImGui();
    RenderImGuizmo();

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRTTRenderTargetView, g_pDepthStencilView);

    g_pImmediateContext->ClearRenderTargetView(g_pRTTRenderTargetView, g_Entities[0].getComponent<CameraComponent>()->GetBackgroundColor());
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    for (int i = 1; i < g_Entities.size(); i++)
    {
        g_Entities[i].getComponent<MeshComponent>()->update(t, g_pImmediateContext, *g_Entities[i].getComponent<TransformComponent>());

        XMMATRIX mGO = XMLoadFloat4x4(g_Entities[i].getComponent<MeshComponent>()->getTransform());
        ConstantBuffer cb1;
        cb1.mWorld = XMMatrixTranspose(mGO);
        cb1.mView = XMMatrixTranspose(g_Entities[0].getComponent<CameraComponent>()->GetViewMatrix(*g_Entities[0].getComponent<TransformComponent>()));
        cb1.mProjection = XMMatrixTranspose(g_Projection);
        cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);
        g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);

        UpdateLight();
        g_Entities[0].getComponent<CameraComponent>()->Update(*g_Entities[0].getComponent<TransformComponent>());

        g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

        g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
        g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

        g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
        g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);

        ID3D11Buffer* materialCB = g_Entities[i].getComponent<MeshComponent>()->getMaterialConstantBuffer();
        g_pImmediateContext->PSSetConstantBuffers(1, 1, &materialCB);

        g_Entities[i].getComponent<MeshComponent>()->draw(g_pImmediateContext, *g_Entities[i].getComponent<TextureRendererComponent>());

        g_Entities[i].getComponent<AnimationComponent>()->update(t);
    }

    g_Entities[0].getComponent<AnimationComponent>()->update(t);

    ID3D11ShaderResourceView* sceneTexture = g_pRTTShaderResourceView;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, g_Entities[0].getComponent<CameraComponent>()->GetBackgroundColor());
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    g_pImmediateContext->PSSetShaderResources(0, 1, &g_pRTTShaderResourceView);
   
    g_pImmediateContext->PSSetShaderResources(3, 1, &sceneTexture);


    if (renderCubeAsRenderTarget && !renderQuad)
    {
        g_Entities[2].getComponent<MeshComponent>()->draw(g_pImmediateContext, *g_Entities[2].getComponent<TextureRendererComponent>(), sceneTexture);
    }

    if (renderQuad) 
    {
        RenderQuad(g_pQuadVS, g_pQuadPS);
    }

    UpdatePostProcessing();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present( 0, 0 );
}

void BeginGeometryPass(ID3D11DeviceContext* context) 
{
    //Begins the geometry pass, so set gbuffers etc
    ID3D11RenderTargetView* rtvs[6] = 
    {
        graphicsBuffer[0].renderTargetView, 
        graphicsBuffer[1].renderTargetView, 
        graphicsBuffer[2].renderTargetView,
        graphicsBuffer[3].renderTargetView,
        graphicsBuffer[4].renderTargetView,
        graphicsBuffer[5].renderTargetView
    };

    context->OMSetRenderTargets(6, rtvs, g_pDepthStencilView);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (int i = 0; i < 6; ++i)
    {
        context->ClearRenderTargetView(graphicsBuffer[i].renderTargetView, g_Entities[0].getComponent<CameraComponent>()->GetBackgroundColor());
    }

    context->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void RenderGeometryPass(ID3D11DeviceContext* context, float deltaTime)
{
    //Renders the geometry for deferred rendering
    for (int i = 1; i < g_Entities.size(); i++)
    {
        g_Entities[i].getComponent<MeshComponent>()->update(deltaTime, context, *g_Entities[i].getComponent<TransformComponent>());

        XMMATRIX mGO = XMLoadFloat4x4(g_Entities[i].getComponent<MeshComponent>()->getTransform());
        ConstantBuffer cb1;
        cb1.mWorld = XMMatrixTranspose(mGO);
        cb1.mView = XMMatrixTranspose(g_Entities[0].getComponent<CameraComponent>()->GetViewMatrix(*g_Entities[0].getComponent<TransformComponent>()));
        cb1.mProjection = XMMatrixTranspose(g_Projection);
        cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);

        context->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);

        g_Entities[0].getComponent<CameraComponent>()->Update(*g_Entities[0].getComponent<TransformComponent>());

        context->IASetInputLayout(g_pVertexLayout);

        context->VSSetShader(g_pDeferredVertexShader, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

        context->PSSetShader(g_pDeferredPixelShader, nullptr, 0);
        context->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);
        ID3D11Buffer* materialCB = g_Entities[i].getComponent<MeshComponent>()->getMaterialConstantBuffer();
        context->PSSetConstantBuffers(1, 1, &materialCB);   

        g_Entities[i].getComponent<MeshComponent>()->draw(context, *g_Entities[i].getComponent<TextureRendererComponent>());

        g_Entities[i].getComponent<AnimationComponent>()->update(deltaTime);
    }   

    g_Entities[0].getComponent<AnimationComponent>()->update(deltaTime);
}

void RenderLightingPass(ID3D11DeviceContext* context)
{
    //This function is responcible particualry for the ligthing pass of deferred rendering
    //it calls lighting and other necessary functions
    ID3D11ShaderResourceView* rtvs[6] =
    {
        graphicsBuffer[0].shaderResourceView,
        graphicsBuffer[1].shaderResourceView,
        graphicsBuffer[2].shaderResourceView,
        graphicsBuffer[3].shaderResourceView,
        graphicsBuffer[4].shaderResourceView,
        graphicsBuffer[5].shaderResourceView
    };

    context->PSSetShaderResources(0,6,rtvs);

    UpdateLight();
    g_Entities[0].getComponent<CameraComponent>()->Update(*g_Entities[0].getComponent<TransformComponent>());

    context->VSSetShader(g_pQuadVS, nullptr, 0);
    context->PSSetShader(g_pLightingPixelShader, nullptr, 0);  

    RenderAdditionalQuads(g_pQuadVS, g_pLightingPixelShader);
    RenderDOFQuad();
 
    ID3D11ShaderResourceView* nullshaders[6] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    context->PSSetShaderResources(0, 6, nullshaders);
}

void RenderDeferredScene()
{
    //This functoin is the final function that renders the deffered rendering approach
    //it takes the geometry adn lighting pass and calls them
    //along with any other necessary functions
    float t = calculateDeltaTime();
    if (t == 0.0f)
        return;

    BeginGeometryPass(g_pImmediateContext);
    RenderGeometryPass(g_pImmediateContext, t);

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    float clearColor[4] = { 0.0f, 0.0f, 0.3f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, g_Entities[0].getComponent<CameraComponent>()->GetBackgroundColor());
  
    RenderLightingPass(g_pImmediateContext);

    UpdatePostProcessing();

    RenderImGui();
    RenderImGuizmo();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(0, 0);
} 