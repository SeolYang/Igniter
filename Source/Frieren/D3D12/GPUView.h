#pragma once
#include <D3D12/Common.h>
#include <Core/Handle.h>
#include <Core/Misc.h>

namespace fe::dx
{
	class GPUView
	{
	public:
		bool IsValid() const { return Type != EGPUViewType::Unknown && Index != InvalidIndexU32; }
		operator bool() const { return IsValid(); }

		bool HasValidCPUHandle() const { return CPUHandle.ptr != std::numeric_limits<decltype(CPUHandle.ptr)>::max(); }
		bool HasValidGPUHandle() const { return GPUHandle.ptr != std::numeric_limits<decltype(GPUHandle.ptr)>::max(); }

	public:
		const EGPUViewType				  Type = EGPUViewType::Unknown; /* Type of descriptor. */
		const uint32_t					  Index = InvalidIndexU32;		/* Index of descriptor in Descriptor Heap. */
		const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = {};				/* CPU Handle of descriptor. */
		const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = {};				/* GPU Handle of descriptor. */
	};

	class GPUViewHandleDestroyer
	{
		friend class GPUViewManager;

	public:
		void operator()(Handle handle, const uint64_t evaluatedTypeHash) const;

	private:
		static void InitialGlobalData(DeferredDeallocator* deallocator, GPUViewManager* manager);

	private:
		static DeferredDeallocator* deferredDeallocator;
		static GPUViewManager*		gpuViewManager;
	};



} // namespace fe::dx