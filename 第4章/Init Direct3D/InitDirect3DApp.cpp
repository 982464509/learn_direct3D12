//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
    InitDirect3DApp(HINSTANCE hInstance);
    ~InitDirect3DApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,PSTR cmdLine, int showCmd)				   
{
	// Enable run-time memory check for debug builds. 为调试构建启用运行时内存检查。
#if defined(DEBUG) | defined(_DEBUG)
	  //检索或修改_crtDbgFlag标志的状态，以控制调试堆管理器的分配行为（仅调试版本）。_CrtSetDbgFlag函数允许应用程序通过修改_crtDbgFlag标志的位域来控制调试堆管理器如何跟踪内存分配。通过设置这些位（打开），应用程序可以指示调试堆管理器执行特殊的调试操作，包括在应用程序退出时检查内存泄漏，如果发现任何泄漏就报告，通过指定释放的内存块应该保留在堆的链接列表中来模拟低内存条件，以及通过在每个分配请求中检查每个内存块来验证堆的完整性。当_DEBUG没有定义时，对_CrtSetDbgFlag的调用在预处理过程中被删除。
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    std::wstring text = L"-------------------------------------- init";
    OutputDebugString(text.c_str());
    try
    {
        InitDirect3DApp theApp(hInstance);
		if (!theApp.Initialize()) 
		{
			return 0;
		}                   
        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance)
{    
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{  
	return D3DApp::Initialize();
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt)
{
}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
    // 重复使用记录命令的相关内存
    // 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，我们便可以重置该命令列表。以
    // 此来复用命令列表及其内存
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
        
    // 对资源的状态进行转换，将资源从呈现状态转换为渲染目标状态
    mCommandList->ResourceBarrier(
        1, 
        &CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 设置视口和裁剪矩形。它们需要随着命令列表的重置而重置
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    //在每帧为了刷新场景而开始绘制之前，我们总是要清除后台缓冲区渲染目标和深度/模板缓冲区。
    // 清除后台缓冲区和深度缓冲区
    mCommandList->ClearRenderTargetView(
        CurrentBackBufferView(),
        Colors::Khaki, 0, nullptr); //nullptr表示清除整个渲染目标
    mCommandList->ClearDepthStencilView(
        DepthStencilView(), 
        D3D12_CLEAR_FLAG_DEPTH |D3D12_CLEAR_FLAG_STENCIL,   //即将清除的是深度缓冲区还是模板缓冲区。用按位或运算符连接两者表示同时清除这两种缓冲区。
        1.0f,   //此值来清除深度缓冲区
        0,      //以此值来清除模板缓冲区。
        0, 
        nullptr);    

    // 指定设置将要渲染的缓冲区。（渲染目标和深度/模板缓冲区）
    mCommandList->OMSetRenderTargets(
        1, 
        &CurrentBackBufferView(),
        true, 
        &DepthStencilView());

    // 再次对资源状态进行转换，将资源从渲染目标状态转换回呈现状态
    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT));

    // 完成命令的记录
    ThrowIfFailed(mCommandList->Close());

    // 将待执行的命令列表加入命令队列
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 交换后台缓冲区和前台缓冲区。与此同时，也必须对索引进行更新，使之一直指向交换后的当前后台缓冲区。
    // 这样一来，我们才可以正确地将下一帧场景渲染到新的后台缓冲区。
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // 等待此帧的命令执行完毕。当前的实现没有什么效率，也过于简单
    // 我们在后面将重新组织渲染部分的代码，以免在每一帧都要等待
    FlushCommandQueue();
}
