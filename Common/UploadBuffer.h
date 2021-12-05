#pragma once
#include "d3dUtil.h"

//1. 上传缓冲区资源的构造与析构函数
//2. 处理资源的映射和取消映射操作
//3. CopyData方法来更新缓冲区内的特定元素

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : 
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);

        // 常量缓冲区的大小为256B的整数倍。这是因为硬件只能按m*256B的偏移量和n*256B的数据长度这两种规格来查看常量数据
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation; // 256的整数倍
        // UINT  SizeInBytes;}     // 256的整数倍       
        if (isConstantBuffer) {
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));
        }
            
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
        // 只要还会修改当前的资源，我们就无须取消映射
        // 但是，在资源被GPU使用期间，我们千万不可向该资源进行写操作（所以必须借助于同步技术）
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if(mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};