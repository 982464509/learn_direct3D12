//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
// 
// d3dApp类提供了创建应用程序主窗口、运行程序消息循环、处理窗口消息以及初始化Direct3D等多种功能的函数。
// 此外，该类还为应用程序例程定义了一组框架函数。我们可以根据需求通过实例化一个继承自D3DApp的类，
// 重写（override）框架的虚函数，以此从D3DApp类中派生出自定义的用户代码。
//***************************************************************************************

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:

    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;

    //析构函数用于释放D3DApp中所用的COM接口对象并刷新命令队列。
    //在析构函数中刷新命令队列的原因是：在销毁GPU引用的资源以前，必须等待GPU处理完队列中的所有命令。否则，可能造成应用程序在退出时崩溃。
    virtual ~D3DApp();

public:

    static D3DApp* GetApp();    
    //返回应用程序实例句柄。
	HINSTANCE AppInst()const;
    //返回主窗口句柄。
	HWND      MainWnd()const;
    //后台缓冲区的宽度与高度之比
	float     AspectRatio()const;
    //是否启用4X MSAA
    bool Get4xMsaaState()const;
    //开启或禁用4X MSAA功能。
    void Set4xMsaaState(bool value);

    //封装了应用程序的消息循环。它使用的是Win32的PeekMessage函数。    
	int Run();
    
    //一、初始化。例如分配资源、初始化对象和建立3D场景等。
    virtual bool Initialize();

    //二、该方法实现应用程序主窗口的窗口过程函数。如果对该方法进行了重写，那么其中并未处理的消息都应当转交至此。
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:

    //三、用于创建RTV和DSV描述符堆。
    //默认的实现是创建一个含有2个RTV描述符的RTV堆，一个DSV描述符的DSV堆（为深度/模板缓冲区而创建）。
    //（除了多渲染目标（multiple render targets）这种高级技术，默认已经足够，不用override）
    virtual void CreateRtvAndDsvDescriptorHeaps();

    //四、当窗口的大小发生了改变，一些Direct3D属性也需要随之调整。
    //重新重置：后台缓冲区、深度/模板缓冲区、渲染目标、深度/模板的视图、projection matrix投影矩阵等。
	virtual void OnResize(); 

    //五、每帧都会调用该抽象方法。通过它更新3D应用程序（如呈现动画、移动摄像机、做碰撞检测以及检查用户的输入等）。
	virtual void Update(const GameTimer& gt)=0;

    //六、每帧都会调用该抽象方法。在该方法中发出渲染命令，将当前帧真正地绘制到后台缓冲区中。
    //当完成帧的绘制后，再调用IDXGISwapChain::Present方法将后台缓冲区的内容显示在屏幕上。
    virtual void Draw(const GameTimer& gt)=0;

	// 便于overrides鼠标输入消息的处理流程
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

    //初始化应用程序主窗口。
	bool InitMainWindow();

    //通过实现4.3节中讨论的步骤来完成Direct3D的初始化。
	bool InitDirect3D();

    //创建命令队列、命令列表分配器和命令列表。4.3.4
	void CreateCommandObjects();

    //创建交换链。
    void CreateSwapChain();

    //强制CPU等待GPU，直到GPU处理完队列中所有的命令。4.2.2
	void FlushCommandQueue();

    //返回交换链中当前后台缓冲区的ID3D12Resource。
	ID3D12Resource* CurrentBackBuffer()const;

    //返回当前后台缓冲区的RTV（渲染目标视图，render target view）。
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;

    //返回主深度/模板缓冲区的DSV（深度/模板视图，depth/stencil view）。
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

    //计算每秒的平均帧数以及每帧平均的毫秒时长。实现方法将在4.5.4节中讨论。
	void CalculateFrameStats();

    //枚举系统中所有的适配器（参见4.1.10节）。
    void LogAdapters();

    //枚举指定适配器的全部显示输出（参见4.1.10节）。
    void LogAdapterOutputs(IDXGIAdapter* adapter);

    //枚举某个显示输出对特定格式支持的所有显示模式（参见4.1.10节）。
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr;  // application instance handle
    HWND      mhMainWnd = nullptr;  // main window handle
	bool      mAppPaused = false;         // is the application paused?
	bool      mMinimized = false;          // is the application minimized?
	bool      mMaximized = false;          // is the application maximized?
	bool      mResizing = false;              // 大小调整栏是否受到拖拽？
    bool      mFullscreenState = false;   // fullscreen enabled

	// Set true to use 4X MSAA .  The default is false.
    bool      m4xMsaaState = false;    // 4X MSAA enabled
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	// 用于记录"delta-time"（帧之间的时间间隔）和游戏总时间(参见4.4节)
	GameTimer mTimer;
	
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    //围栏
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;
	
    //命令队列
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    //命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    //命令列表
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;   
    //渲染目标视图 （描述符堆）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    //深度/模板视图（描述符堆）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    //交换链中的缓冲区
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    //用来记录当前后台缓冲区的索引
    int mCurrBackBuffer = 0;


    //视口
    D3D12_VIEWPORT mScreenViewport; 
    //裁剪矩形
    D3D12_RECT mScissorRect;

    //rtv描述符大小，使用这个进行偏移计算进行迭代
	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

    // 用户应该在派生类的派生构造函数中自定义这些初始值
	std::wstring mMainWndCaption = L"d3d App ";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};