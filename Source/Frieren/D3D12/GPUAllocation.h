#pragma once
#include <D3D12/Commons.h>

namespace fe::Private
{
    struct GPUAllocation
    {
        ID3D12Resource* Resource = nullptr;
		D3D12MA::Allocation* Allocation = nullptr;
    };
}