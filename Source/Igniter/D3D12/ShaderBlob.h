#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>

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
    class ShaderCompileDesc
    {
    public:
        String SourcePath;
        EShaderType Type;
        bool bPackMarticesInRowMajor = false;                                      // -Zpr
        EShaderOptimizationLevel OptimizationLevel = EShaderOptimizationLevel::O3; // -Od~-O3
        bool bDisableValidation = false;                                           // -Vd
        bool bTreatWarningAsErrors = false;                                        // -WX
        bool bForceEnableDebugInformation = false;                                 // -Zi
    };

    // #sy_todo dxil.dll 빌드 후 이벤트로 옮겨주기
    class ShaderBlob
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