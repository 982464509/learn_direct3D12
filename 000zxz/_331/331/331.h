#pragma once

#include "resource.h"



//class MyD3DApp 

//protected:

//    MyD3DApp(HINSTANCE hInstance);
//    MyD3DApp(const MyD3DApp& rhs) = delete;
//    MyD3DApp& operator=(const MyD3DApp& rhs) = delete;
//
//    //析构函数用于释放D3DApp中所用的COM接口对象并刷新命令队列。
//    //在析构函数中刷新命令队列的原因是：在销毁GPU引用的资源以前，必须等待GPU处理完队列中的所有命令。否则，可能造成应用程序在退出时崩溃。
//    virtual ~MyD3DApp();
//
//public:
   
    //static MyD3DApp* GetApp();
    ////返回应用程序实例句柄。
    //HINSTANCE AppInst()const;
    ////返回主窗口句柄。
    //HWND      MainWnd()const;

    
   Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    //渲染目标视图 （描述符堆）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    //深度/模板视图（描述符堆）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

    //rtv描述符大小，使用这个进行偏移计算进行迭代
    UINT rtvDescriptorSize = 0;

    bool      m4xMsaaState = false;    // 4X MSAA enabled
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

     //围栏
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    //UINT64 mCurrentFence = 0;

   /* void CreateFence();
    void GetDescriptorSize();*/

