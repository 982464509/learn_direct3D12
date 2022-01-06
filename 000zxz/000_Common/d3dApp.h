//***************************************************************************************
// 
// 
// d3dApp���ṩ�˴���Ӧ�ó��������ڡ����г�����Ϣѭ������������Ϣ�Լ���ʼ��Direct3D�ȶ��ֹ��ܵĺ�����
// ���⣬���໹ΪӦ�ó������̶�����һ���ܺ��������ǿ��Ը�������ͨ��ʵ����һ���̳���D3DApp���࣬
// ��д��override����ܵ��麯�����Դ˴�D3DApp�����������Զ�����û����롣
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

    //�������������ͷ�D3DApp�����õ�COM�ӿڶ���ˢ��������С�
    //������������ˢ��������е�ԭ���ǣ�������GPU���õ���Դ��ǰ������ȴ�GPU����������е�����������򣬿������Ӧ�ó������˳�ʱ������
    virtual ~D3DApp();

public:
	
    static D3DApp* GetApp();
    //����Ӧ�ó���ʵ�������
    HINSTANCE AppInst()const;
    //���������ھ����
    HWND      MainWnd()const;
    //��̨�������Ŀ����߶�֮��
    float     AspectRatio()const;
    //�Ƿ�����4X MSAA
    bool Get4xMsaaState()const;
    //���������4X MSAA���ܡ�
    void Set4xMsaaState(bool value);

    int Run();

    //һ����ʼ�������������Դ����ʼ������ͽ���3D�����ȡ�
    virtual bool Initialize();    
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    //�������ڴ���RTV��DSV�������ѡ�
    //Ĭ�ϵ�ʵ���Ǵ���һ������2��RTV��������RTV�ѣ�һ��DSV��������DSV�ѣ�Ϊ���/ģ�建��������������
    //�����˶���ȾĿ�꣨multiple render targets�����ָ߼�������Ĭ���Ѿ��㹻������override��
    virtual void CreateRtvAndDsvDescriptorHeaps();

    virtual void OnResize();
    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
    virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
    virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
    //��ʼ��Ӧ�ó��������ڡ�
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    //������������
    void CreateSwapChain();
    //ǿ��CPU�ȴ�GPU��ֱ��GPU��������������е�����
    void FlushCommandQueue();
    //�õ��������е�ǰ ��̨��������ID3D12Resource��
    ID3D12Resource* CurrentBackBuffer()const;
    //�õ���ǰ ��̨��������RTV����ȾĿ����ͼ��render target view����
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
    //�õ������/ģ�建������DSV�����/ģ����ͼ����
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;   
    void CalculateFrameStats();

protected:

    static D3DApp* mApp;

    static const int SwapChainBufferCount = 2;

    HINSTANCE mhAppInst = nullptr;  // application instance handle
    HWND      mhMainWnd = nullptr;  // main window handle
    bool      mAppPaused = false;         // is the application paused?
    bool      mMinimized = false;          // is the application minimized?
    bool      mMaximized = false;          // is the application maximized?
    bool      mResizing = false;              // ��С�������Ƿ��ܵ���ק��
    bool      mFullscreenState = false;   // fullscreen enabled

    // Set true to use 4X MSAA .  The default is false.
    bool      m4xMsaaState = false;    // 4X MSAA enabled
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

    // ���ڼ�¼"delta-time"��֮֡���ʱ����������Ϸ��ʱ��(�μ�4.4��)
    GameTimer mTimer;

    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    //Χ��
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    //�������
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    //���������
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    //�����б�
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
    
    //��ȾĿ����ͼ ���������ѣ�
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    //���/ģ����ͼ���������ѣ�
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    //�������еĻ�����
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    //������¼��ǰ��̨������������
    int mCurrBackBuffer = 0;

    //�ӿ�
    D3D12_VIEWPORT mScreenViewport;
    //�ü�����
    D3D12_RECT mScissorRect;

    //rtv��������С��ʹ���������ƫ�Ƽ�����е���
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;


    // �û�Ӧ������������������캯�����Զ�����Щ��ʼֵ
    std::wstring mMainWndCaption = L"zxz d3d";
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    int mClientWidth = 800;
    int mClientHeight = 600;
};