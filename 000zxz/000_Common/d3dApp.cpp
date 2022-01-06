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
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
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
	{
		//ˢ��������С�������GPU���õ���Դ��ǰ������ȴ�GPU����������е���������
		FlushCommandQueue();
	}		
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
	OnResize();
	return true;
}


bool D3DApp::InitMainWindow()
{
	WNDCLASS wcss;	// ��һ���������ͨ����дWNDCLASS�ṹ�壬��������������������������һ������
	wcss.style = CS_HREDRAW | CS_VREDRAW;
	wcss.lpfnWndProc = MainWndProc;
	wcss.cbClsExtra = 0;
	wcss.cbWndExtra = 0;
	wcss.hInstance = mhAppInst;
	wcss.hIcon = LoadIcon(0, IDI_APPLICATION);
	wcss.hCursor = LoadCursor(0, IDC_ARROW);
	wcss.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcss.lpszMenuName = 0;
	wcss.lpszClassName = L"MainWnd";

	// ��������Ŀͻ�������ߴ���㴰�ڵľ��γߴ硣
	RECT rect = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	return true;
}

bool D3DApp::InitDirect3D()
{
	ComPtr<ID3D12Debug> debugController;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();

	CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory));  //����Direct3D 12�豸��ID3D12Device��

	ComPtr<IDXGIAdapter> pWarpAdapter;
	mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));
	D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
		
		
		





}