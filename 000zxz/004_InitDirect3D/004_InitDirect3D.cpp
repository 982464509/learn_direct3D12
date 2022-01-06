//***************************************************************************************
//
// 
// 
//***************************************************************************************

#include "../../000zxz/000_Common/d3dApp.h"
#include<Windows.h>
#include <string>
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

}