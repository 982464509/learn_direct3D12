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
:	mhAppInst(hInstance)
{
    // Only one D3DApp can be constructed.
    assert(mApp == nullptr);
    mApp = this;
}

D3DApp::~D3DApp()
{
	if(md3dDevice != nullptr)
		//刷新命令队列。在销毁GPU引用的资源以前，必须等待GPU处理完队列中的所有命令
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
    if(m4xMsaaState != value)
    {
        m4xMsaaState = value;
        CreateSwapChain();
        OnResize();
    }
}

int D3DApp::Run()
{
	MSG msg = {0}; 	
	mTimer.Reset();
	while(msg.message != WM_QUIT)
	{		
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))// 如果有窗口消息就进行处理
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}		
		else
        {			
			//执行动画与游戏的相关逻辑			
			mTimer.Tick();
			if( !mAppPaused )
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
	if(!InitMainWindow())
		return false;
	if(!InitDirect3D())
		return false;   
    OnResize(); // Do the initial resize code.
	return true;
}
 
//⑥. 创建描述符堆
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	//为交换链中SwapChainBufferCount个用于渲染数据的缓冲区资源创建对应的渲染目标视图（Render Target View，RTV）	
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;	// 这个描述符堆用来存储SwapChainBufferCount个RTV
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));
        
	//用于深度测试的深度/模板缓冲区资源创建一个深度/模板视图（Depth/Stencil View，DSV）	
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));        		
}


void D3DApp::OnResize()
{	
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();	
	//上面刷新了队列，那么该命令列表就可以重置。以此来复用命令列表及其内存
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 释放所有缓冲区数据
	for (int i = 0; i < SwapChainBufferCount; ++i) {
		mSwapChainBuffer[i].Reset();
	}		
    mDepthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, 
		mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
	
	//⑦. 创建渲染目标视图
	//通过调用这两种方法为交换链中的每一个缓冲区都创建了一个RTV。
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// 获得交换链内的第i个缓冲区
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		// 为此缓冲区创建一个RTV 渲染目标视图
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// 偏移到描述符堆中的下一个缓冲区
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    //⑧. 创建深度/模板纹理及相应的深度/模板视图：
    D3D12_RESOURCE_DESC depthStencilDesc; //该结构体用来描述纹理资源
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//2D纹理资源
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;   //纹素为单位，表示纹理宽度。对于缓冲区资源来说，此项是缓冲区占用的字节数。
    depthStencilDesc.Height = mClientHeight; //纹素为单位，表示纹理高度。
    depthStencilDesc.DepthOrArraySize = 1;	// 纹理数组的大小
    depthStencilDesc.MipLevels = 1; //mipmap层级的数量
    depthStencilDesc.Format = mDepthStencilFormat;	//纹素格式	
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1; //SampleDesc：多重采样的质量级别以及对每个像素的采样次数。
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;	//指定纹理的布局
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//描述了一个用于清除资源的优化值。可提高清除操作的执行速度。若不希望指定优化清除值，可把此参数设为nullptr。
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	//根据提供的属性创建一个资源与一个堆，并把该资源提交到这个堆中：	
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),  //（资源欲提交至的）堆所具有的属性。默认堆：向这堆里提交的资源，唯独GPU可以访问。
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,  //描述待建的资源
		D3D12_RESOURCE_STATE_COMMON,
        &optClear, 
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf()))); //希望获得的ID3D12Resource接口的COM ID。

    // 利用此资源的格式，为整个资源的第0 mip层创建描述符
    md3dDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(), 
		nullptr, 
		DepthStencilView());

    // 将资源从初始状态转换为深度缓冲区，将其转换为可以绑定在渲染流水线上的深度/模板缓冲区。
	mCommandList->ResourceBarrier(
		1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(
				mDepthStencilBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, 
				D3D12_RESOURCE_STATE_DEPTH_WRITE));
	

    // 执行调整尺寸的命令
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	//⑩. 设置剪裁矩形
	// 设置视口 更新视口变换以覆盖客户端区域
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	//设置剪裁矩形
    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}
 
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// WM_ACTIVATE在窗口被激活或停用时被发送。当窗口停用时，我们会暂停游戏，当它变成激活时，我们会取消暂停。
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
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
		mClientWidth  = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if( md3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{				
				// Restoring from minimized state?
				if( mMinimized )
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}
				// Restoring from maximized state?
				else if( mMaximized )
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if( mResizing )
				{
					//如果用户在拖动调整栏，我们不在这里调整缓冲区的大小，因为当用户连续拖动调整栏时，会有一串WM_SIZE消息被发送到窗口。 
					//我们在用户完成窗口大小的调整并释放大小条后进行重置，这时会发送一个WM_EXITSIZEMOVE消息。
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// 当用户抓取调整栏时发送WM_ENTERSIZEMOVE消息
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing  = true;
		mTimer.Stop();
		return 0;

	// 当用户释放调整栏时发送WM_EXITSIZEMOVE消息
	// 此处将根据新的窗口大小重置相关对象（如缓冲区、视图等）
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing  = false;
		mTimer.Start();
		OnResize();
		return 0;
 
	// 当窗口被销毁时发送WM_DESTROY消息
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// 当某一菜单处于激活状态，而且用户按下的既不是助记键（mnemonic key）也不是加速键
	// （acceleratorkey）时，就发送WM_MENUCHAR消息
	case WM_MENUCHAR:
        // 当按下组合键alt-enter时不发出beep蜂鸣声
        return MAKELRESULT(0, MNC_CLOSE);

	// 捕获此消息以防窗口变得过小
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
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            Set4xMsaaState(!m4xMsaaState);
        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = mhAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWnd";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// 根据请求的客户端区域尺寸计算窗口的矩形尺寸。
	RECT R = { 0, 0, mClientWidth, mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
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
	if( !mhMainWnd )
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
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif
	//①. 创建Direct3D 12设备（ID3D12Device）
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// 尝试创建硬件设备。
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             //指定在创建设备时所用的显示适配器。若将此参数设定为空指针，则使用主显示适配器。
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));		
		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ComPtr<IDXGIAdapter> pWarpAdapter;
	ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
	ThrowIfFailed(D3D12CreateDevice(
		pWarpAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice)));

	// ②. 创建围栏并获取描述符的大小
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));		
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//③. 检查4X MSAA质量对我们的后置缓冲区格式的支持。
	//所有支持Direct3D 11的设备都支持4X MSAA的所有渲染
	//目标格式，所以我们只需要检查质量支持。
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	//平台肯定能支持4X MSAA，所以返回值应该也总是大于0。
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
	
#ifdef _DEBUG
    LogAdapters();
#endif
	//④. 创建命令队列和命令列表	
	CreateCommandObjects();
	//⑤. 描述并创建交换链
    CreateSwapChain();
	//⑥. 创建描述符堆
    CreateRtvAndDsvDescriptorHeaps();
	return true;
}

//④. 创建命令队列和命令列表
void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // 关联命令分配器 command allocator
		nullptr,// 初始化流水线状态对象；因为在这里不会发起绘制命令，所以不会用到流水线状态对象
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// 首先要将命令列表置于关闭状态。这是因为在第一次引用命令列表时，我们要对它进行重置，而在调用
    // 重置方法之前又需先将其关闭
	mCommandList->Close();
}

//⑤. 描述并创建交换链
void D3DApp::CreateSwapChain()
{
	// 释放之前所创的交换链，随后再进行重建。这样一来，我们就可以用不同的设置来重新创建交换链，借此在运行时修改多重采样的配置。
    mSwapChain.Reset();
	
	//描述创建后的交换链的特性
    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;		// 缓冲区分辨率
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;	// 缓冲区的显示格式
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;// 逐行扫描 vs. 隔行扫描
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;	 // 图像如何相对于屏幕进行拉伸

    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//用它作为渲染目标，所以采用这个
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;	//渲染窗口的句柄
    sd.Windowed = true;	//若为true，程序将在窗口模式下运行；如果指定为false，则采用全屏模式。
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//当程序切换为全屏模式时，它将选择最适于当前应用程序窗口尺寸的显示模式。
	//如果没有指定该标志，当程序切换为全屏模式时，将采用当前桌面的显示模式。
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//注意，交换链需要通过命令队列对其进行刷新
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(		//CreateSwapChain方法用来创建交换链
		mCommandQueue.Get(),			// 指向ID3D12CommandQueue接口的指针
		&sd,											// 指向描述交换链的结构体的指针
		mSwapChain.GetAddressOf())); // 返回所创建的交换链接口
}

void D3DApp::FlushCommandQueue()
{
	// 增加围栏值，接下来将命令标记到此围栏点
    mCurrentFence++;
	// 向命令队列中添加一条用来设置新围栏点的命令	。由于这条命令要交由GPU修改围栏值，所以在GPU处理完以前的所有命令之前，它并不会设置新的围栏点Signal
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// 在CPU端等待GPU，直到后者执行完这个围栏点之前的所有命令
    if(mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		// 若GPU命中当前的围栏（即执行到Signal()指令，修改了围栏值），则激发预定事件
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
		// 等待GPU命中围栏，激发事件
		WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

//访问描述符堆内部的描述符。可获取当前 后台缓冲区的RTV
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	// CD3DX12构造函数根据给定的偏移量找到当前后台缓冲区的RTV
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),//获得描述符堆中第一个描述符的句柄
		mCurrBackBuffer,		// 偏移至后台缓冲区描述符句柄的索引
		mRtvDescriptorSize); // 描述符所占字节的大小

	//为了用偏移量找到当前后台缓冲区的RTV描述符，我们就必须知道RTV描述符的大小。
}

//访问描述符堆内部的描述符。可获取当前 后台缓冲区的DSV
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}


void D3DApp::CalculateFrameStats()
{
	// 这段代码计算了每秒的平均帧数，也计算了每帧的平均渲染时间	
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;
	frameCnt++;
	// 以1秒为统计周期来计算平均帧数以及每帧的平均渲染时间
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;
        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);
        wstring windowText = mMainWndCaption + L"    fps: " + fpsStr + L"   mspf: " + mspfStr;                       
        SetWindowText(mhMainWnd, windowText.c_str());
		
		// 为计算下一组平均值而重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}