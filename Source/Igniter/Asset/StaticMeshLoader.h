#pragma once
#include <Igniter.h>
#include <Asset/StaticMesh.h>

namespace ig
{
    class StaticMeshLoader final
    {
    public:
        StaticMeshLoader(HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);
        StaticMeshLoader(const StaticMeshLoader&) = delete;
        StaticMeshLoader(StaticMeshLoader&&) noexcept = delete;
        ~StaticMeshLoader() = default;

        StaticMeshLoader& operator=(const StaticMeshLoader&) = delete;
        StaticMeshLoader& operator=(StaticMeshLoader&&) noexcept = delete;

        /* #sy_deprecated */
        static std::optional<StaticMesh> _Load(const xg::Guid& guid, HandleManager& handleManager, RenderDevice& renderDevice, GpuUploader& gpuUploader, GpuViewManager& gpuViewManager);

    public:
        HandleManager& handleManager;
        RenderDevice& renderDevice;
        GpuUploader& gpuUploader;
        GpuViewManager& gpuViewManager;
    };
}