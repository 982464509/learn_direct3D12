//***************************************************************************************
//
// 
// 
//***************************************************************************************

#include "../../000zxz/000_Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp :public D3DApp 
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	std::wstring text = L"-------------------------------------- init";
	OutputDebugString(text.c_str());
    InitDirect3DApp theApp(hInstance);
    theApp.Initialize();
    theApp.Run();
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
	mDirectCmdListAlloc->Reset();

	// 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，我们便可以重置该命令列表。以
	// 此来复用命令列表及其内存
	mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);

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
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,   //即将清除的是深度缓冲区还是模板缓冲区。用按位或运算符连接两者表示同时清除这两种缓冲区。
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
	mCommandList->Close();

	// 将待执行的命令列表加入命令队列
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换后台缓冲区和前台缓冲区。与此同时，也必须对索引进行更新，使之一直指向交换后的当前后台缓冲区。
	// 这样一来，我们才可以正确地将下一帧场景渲染到新的后台缓冲区。
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 等待此帧的命令执行完毕。当前的实现没有什么效率，也过于简单
	// 我们在后面将重新组织渲染部分的代码，以免在每一帧都要等待
	FlushCommandQueue();
}