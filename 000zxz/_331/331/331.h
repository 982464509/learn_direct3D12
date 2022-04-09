#pragma once

#include "resource.h"



//class MyD3DApp 

//protected:

//    MyD3DApp(HINSTANCE hInstance);
//    MyD3DApp(const MyD3DApp& rhs) = delete;
//    MyD3DApp& operator=(const MyD3DApp& rhs) = delete;
//
//    //�������������ͷ�D3DApp�����õ�COM�ӿڶ���ˢ��������С�
//    //������������ˢ��������е�ԭ���ǣ�������GPU���õ���Դ��ǰ������ȴ�GPU����������е�����������򣬿������Ӧ�ó������˳�ʱ������
//    virtual ~MyD3DApp();
//
//public:
   
    //static MyD3DApp* GetApp();
    ////����Ӧ�ó���ʵ�������
    //HINSTANCE AppInst()const;
    ////���������ھ����
    //HWND      MainWnd()const;

    
   Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    //��ȾĿ����ͼ ���������ѣ�
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
    //���/ģ����ͼ���������ѣ�
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

    //rtv��������С��ʹ���������ƫ�Ƽ�����е���
    UINT rtvDescriptorSize = 0;

    bool      m4xMsaaState = false;    // 4X MSAA enabled
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

     //Χ��
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    //UINT64 mCurrentFence = 0;

   /* void CreateFence();
    void GetDescriptorSize();*/

