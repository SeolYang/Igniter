#pragma once
#include "Igniter/D3D12/Common.h"
#include "Igniter/Core/String.h"

namespace ig
{
    enum class EShaderType
    {
        Vertex,
        Pixel,
        Domain,
        Hull,
        Geometry,
        Compute,
        Amplification,
        Mesh
    };

    enum class EShaderOptimizationLevel
    {
        None,
        O0,
        O1,
        O2,
        O3 // Default
    };

    // #sy_todo 매크로 정의 지원
    class ShaderCompileDesc final
    {
    public:
        std::string_view SourcePath;
        EShaderType Type;
        bool bPackMarticesInRowMajor = false;                                      // -Zpr
        EShaderOptimizationLevel OptimizationLevel = EShaderOptimizationLevel::O3; // -Od~-O3
        bool bDisableValidation = false;                                           // -Vd
        bool bTreatWarningAsErrors = false;                                        // -WX
        bool bForceEnableDebugInformation = false;                                 // -Zi
    };

    class ShaderBlob final
    {
    public:
        ShaderBlob(const ShaderCompileDesc& desc);
        ~ShaderBlob() = default;

        EShaderType GetType() const { return type; }
        auto& GetNative() { return *shader.Get(); }

    private:
        const EShaderType type;
        ComPtr<IDxcResult> compiledResult;
        ComPtr<IDxcBlob> shader;
    };
} // namespace ig
