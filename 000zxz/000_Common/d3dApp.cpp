//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
	return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance)
	: mhAppInst(hInstance)
{
	// Only one D3DApp can be constructed.
	assert(mApp == nullptr);
	mApp = this;
}

D3DApp::~D3DApp()
{
	if (md3dDevice != nullptr)
		//ˢ��������С�������GPU���õ���Դ��ǰ������ȴ�GPU����������е���������
		FlushCommandQueue();
}

HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

float D3DApp::AspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
	return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;
		CreateSwapChain();
		OnResize();
	}
}

int D3DApp::Run()
{
	MSG msg = { 0 };
	mTimer.Reset();
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))// ����д�����Ϣ�ͽ��д���
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//ִ�ж�������Ϸ������߼�			
			mTimer.Tick();
			if (!mAppPaused)
			{
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	if (!InitMainWindow())
		return false;
	if (!InitDirect3D())
		return false;
	OnResize(); // Do the initial resize code.
	return true;
}

//��. ������������
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	//Ϊ��������SwapChainBufferCount��������Ⱦ���ݵĻ�������Դ������Ӧ����ȾĿ����ͼ��Render Target View��RTV��	
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;	// ����������������洢SwapChainBufferCount��RTV
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf()));

	//������Ȳ��Ե����/ģ�建������Դ����һ�����/ģ����ͼ��Depth/Stencil View��DSV��	
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()));
}


void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();
	//����ˢ���˶��У���ô�������б��Ϳ������á��Դ������������б������ڴ�
	mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);

	// �ͷ����л���������
	for (int i = 0; i < SwapChainBufferCount; ++i) {
		mSwapChainBuffer[i].Reset();
	}
	mDepthStencilBuffer.Reset();

	// Resize the swap chain.
	mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth,
		mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	mCurrBackBuffer = 0;

	//��. ������ȾĿ����ͼ
	//ͨ�����������ַ���Ϊ�������е�ÿһ����������������һ��RTV��
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// ��ý������ڵĵ�i��������
		mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
		// Ϊ�˻���������һ��RTV ��ȾĿ����ͼ
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// ƫ�Ƶ����������е���һ��������
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	//��. �������/ģ����������Ӧ�����/ģ����ͼ��
	D3D12_RESOURCE_DESC depthStencilDesc; //�ýṹ����������������Դ
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//2D������Դ
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;   //����Ϊ��λ����ʾ�������ȡ����ڻ�������Դ��˵�������ǻ�����ռ�õ��ֽ�����
	depthStencilDesc.Height = mClientHeight; //����Ϊ��λ����ʾ�����߶ȡ�
	depthStencilDesc.DepthOrArraySize = 1;	// ��������Ĵ�С
	depthStencilDesc.MipLevels = 1; //mipmap�㼶������
	depthStencilDesc.Format = mDepthStencilFormat;	//���ظ�ʽ	
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1; //SampleDesc�����ز��������������Լ���ÿ�����صĲ���������
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	//ָ�������Ĳ���
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//������һ�����������Դ���Ż�ֵ����������������ִ���ٶȡ�����ϣ��ָ���Ż����ֵ���ɰѴ˲�����Ϊnullptr��
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	//�����ṩ�����Դ���һ����Դ��һ���ѣ����Ѹ���Դ�ύ��������У�	
	md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),  //����Դ���ύ���ģ��������е����ԡ�Ĭ�϶ѣ���������ύ����Դ��Ψ��GPU���Է��ʡ�
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,  //������������Դ
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())); //ϣ����õ�ID3D12Resource�ӿڵ�COM ID��

	// ���ô���Դ�ĸ�ʽ��Ϊ������Դ�ĵ�0 mip�㴴��������
	md3dDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(),
		nullptr,
		DepthStencilView());

	// ����Դ�ӳ�ʼ״̬ת��Ϊ��Ȼ�����������ת��Ϊ���԰�����Ⱦ��ˮ���ϵ����/ģ�建������
	mCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE));


	// ִ�е����ߴ������
	mCommandList->Close();
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	//��. ���ü��þ���
	// �����ӿ� �����ӿڱ任�Ը��ǿͻ�������
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	//���ü��þ���
	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE�ڴ��ڱ������ͣ��ʱ�����͡�������ͣ��ʱ�����ǻ���ͣ��Ϸ��������ɼ���ʱ�����ǻ�ȡ����ͣ��
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;
	case WM_SIZE: // WM_SIZE is sent when the user resizes the window.  
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}
				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					//����û����϶������������ǲ�����������������Ĵ�С����Ϊ���û������϶�������ʱ������һ��WM_SIZE��Ϣ�����͵����ڡ� 
					//�������û���ɴ��ڴ�С�ĵ������ͷŴ�С����������ã���ʱ�ᷢ��һ��WM_EXITSIZEMOVE��Ϣ��
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// ���û�ץȡ������ʱ����WM_ENTERSIZEMOVE��Ϣ
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// ���û��ͷŵ�����ʱ����WM_EXITSIZEMOVE��Ϣ
		// �˴��������µĴ��ڴ�С������ض����绺��������ͼ�ȣ�
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// �����ڱ�����ʱ����WM_DESTROY��Ϣ
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// ��ĳһ�˵����ڼ���״̬�������û����µļȲ������Ǽ���mnemonic key��Ҳ���Ǽ��ټ�
		// ��acceleratorkey��ʱ���ͷ���WM_MENUCHAR��Ϣ
	case WM_MENUCHAR:
		// ��������ϼ�alt-enterʱ������beep������
		return MAKELRESULT(0, MNC_CLOSE);

		// �������Ϣ�Է����ڱ�ù�С
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// ��������Ŀͻ�������ߴ���㴰�ڵľ��γߴ硣
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(
		L"MainWnd",
		mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		0,
		0,
		mhAppInst,
		0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	return true;
}

bool D3DApp::InitDirect3D()
{
	// Enable the D3D12 debug layer.
#if defined(DEBUG) || defined(_DEBUG)	
	{
		ComPtr<ID3D12Debug> debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();
	}
#endif
	//��. ����Direct3D 12�豸��ID3D12Device��
	CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory));

	// ���Դ���Ӳ���豸��
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             //ָ���ڴ����豸ʱ���õ���ʾ�������������˲����趨Ϊ��ָ�룬��ʹ������ʾ��������
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));
		D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice));
	}

	// ��. ����Χ������ȡ�������Ĵ�С
	md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//��. ���4X MSAA���������ǵĺ��û�������ʽ��֧�֡�
	//����֧��Direct3D 11���豸��֧��4X MSAA��������Ⱦ
	//Ŀ���ʽ����������ֻ��Ҫ�������֧�֡�
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	//ƽ̨�϶���֧��4X MSAA�����Է���ֵӦ��Ҳ���Ǵ���0��
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	//��. ����������к������б�	
	CreateCommandObjects();
	//��. ����������������
	CreateSwapChain();
	//��. ������������
	CreateRtvAndDsvDescriptorHeaps();
	return true;
}

//��. ����������к������б�
void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));

	md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf()));
	md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // ������������� command allocator
		nullptr,// ��ʼ����ˮ��״̬������Ϊ�����ﲻ�ᷢ�����������Բ����õ���ˮ��״̬����
		IID_PPV_ARGS(mCommandList.GetAddressOf()));

	// ����Ҫ�������б����ڹر�״̬��������Ϊ�ڵ�һ�����������б�ʱ������Ҫ�����������ã����ڵ���
	// ���÷���֮ǰ�����Ƚ���ر�
	mCommandList->Close();
}

//��. ����������������
void D3DApp::CreateSwapChain()
{
	// �ͷ�֮ǰ�����Ľ�����������ٽ����ؽ�������һ�������ǾͿ����ò�ͬ�����������´��������������������ʱ�޸Ķ��ز��������á�
	mSwapChain.Reset();

	//����������Ľ�����������
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;		// �������ֱ���
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;	// ����������ʾ��ʽ
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;// ����ɨ�� vs. ����ɨ��
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	 // ͼ������������Ļ��������

	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//������Ϊ��ȾĿ�꣬���Բ������
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;	//��Ⱦ���ڵľ��
	sd.Windowed = true;	//��Ϊtrue�������ڴ���ģʽ�����У����ָ��Ϊfalse�������ȫ��ģʽ��
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//�������л�Ϊȫ��ģʽʱ������ѡ�������ڵ�ǰӦ�ó��򴰿ڳߴ����ʾģʽ��
	//���û��ָ���ñ�־���������л�Ϊȫ��ģʽʱ�������õ�ǰ�������ʾģʽ��
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//ע�⣬��������Ҫͨ��������ж������ˢ��
	mdxgiFactory->CreateSwapChain(		//CreateSwapChain������������������
		mCommandQueue.Get(),			// ָ��ID3D12CommandQueue�ӿڵ�ָ��
		&sd,											// ָ�������������Ľṹ���ָ��
		mSwapChain.GetAddressOf()); // �����������Ľ������ӿ�
}

void D3DApp::FlushCommandQueue()
{	
	mCurrentFence++;	
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);		
		mFence->SetEventOnCompletion(mCurrentFence, eventHandle);		
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{	
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),		
		mCurrBackBuffer, mRtvDescriptorSize);		
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}


void D3DApp::CalculateFrameStats()
{	
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;
	frameCnt++;	
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;
		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);
		wstring windowText = mMainWndCaption + L"    fps: " + fpsStr + L"   mspf: " + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}