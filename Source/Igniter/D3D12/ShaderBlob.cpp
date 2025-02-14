#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/D3D12/ShaderBlob.h"

IG_DECLARE_LOG_CATEGORY(ShaderBlobLog);

IG_DEFINE_LOG_CATEGORY(ShaderBlobLog);

namespace ig
{
    // #sy_todo 셰이더 컴파일 후 파일로 저장/캐싱 -> 셰이더 에셋
    ShaderBlob::ShaderBlob(const ShaderCompileDesc& desc)
        : type(desc.Type)
    {
        Vector<const wchar_t*> arguments;
        arguments.push_back(TEXT("-E"));
        arguments.push_back(TEXT("main"));

        arguments.push_back(TEXT("-T"));
        switch (desc.Type)
        {
        case EShaderType::Vertex:
            arguments.push_back(TEXT("vs_6_6"));
            break;
        case EShaderType::Pixel:
            arguments.push_back(TEXT("ps_6_6"));
            break;
        case EShaderType::Domain:
            arguments.push_back(TEXT("ds_6_6"));
            break;
        case EShaderType::Hull:
            arguments.push_back(TEXT("hs_6_6"));
            break;
        case EShaderType::Geometry:
            arguments.push_back(TEXT("gs_6_6"));
            break;
        case EShaderType::Compute:
            arguments.push_back(TEXT("cs_6_6"));
            break;
        case EShaderType::Mesh:
            arguments.push_back(TEXT("ms_6_6"));
            break;
        case EShaderType::Amplification:
            arguments.push_back(TEXT("as_6_6"));
            break;
        default:
            IG_VERIFY(false);
            break;
        }

        /* https://github.com/microsoft/DirectXShaderCompiler/wiki/16-Bit-Scalar-Types */
        arguments.push_back(TEXT("-enable-16bit-types"));
        arguments.push_back(TEXT("-Qstrip_reflect"));

        if (desc.bPackMarticesInRowMajor)
        {
            arguments.push_back(TEXT("-Zpr"));
        }

        switch (desc.OptimizationLevel)
        {
        case EShaderOptimizationLevel::None:
            arguments.push_back(TEXT("-Od"));
            break;
        case EShaderOptimizationLevel::O0:
            arguments.push_back(TEXT("-O0"));
            break;
        case EShaderOptimizationLevel::O1:
            arguments.push_back(TEXT("-O1"));
            break;
        case EShaderOptimizationLevel::O2:
            arguments.push_back(TEXT("-O2"));
            break;
        case EShaderOptimizationLevel::O3:
            arguments.push_back(TEXT("-O3"));
            break;
        }

        if (desc.bDisableValidation)
        {
            arguments.push_back(TEXT("-Vd"));
        }

        if (desc.bTreatWarningAsErrors)
        {
            arguments.push_back(TEXT("-WX"));
        }

#if defined(DEBUG) || defined(_DEBUG) || defined(REL_WITH_DEBINFO) || defined(ENABLE_PROFILE)
        const bool bEnableDebugInfo = true;
#else
        const bool bEnableDebugInfo = desc.bForceEnableDebugInformation;
#endif
        if (bEnableDebugInfo)
        {
            arguments.push_back(TEXT("-Zi"));
            arguments.push_back(TEXT("-Qembed_debug"));
        }
        else
        {
            arguments.push_back(TEXT("-Qstrip_debug"));
        }

        arguments.push_back(TEXT("-I /Assets/Shaders"));
        ComPtr<IDxcUtils> utils;
        IG_VERIFY_SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));

        ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
        utils->CreateDefaultIncludeHandler(defaultIncludeHandler.ReleaseAndGetAddressOf());

        ComPtr<IDxcLibrary> library;
        IG_VERIFY_SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.ReleaseAndGetAddressOf())));

        U32 codePage = CP_UTF8;
        const std::wstring wideSourcePath = desc.SourcePath.ToWideString();
        ComPtr<IDxcBlobEncoding> sourceBlob;
        IG_VERIFY_SUCCEEDED(library->CreateBlobFromFile(wideSourcePath.c_str(), &codePage, &sourceBlob));

        ComPtr<IDxcCompiler3> compiler;
        IG_VERIFY_SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf())));

        const DxcBuffer buffer{.Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = codePage};

        IG_VERIFY_SUCCEEDED(compiler->Compile(&buffer, arguments.data(), static_cast<U32>(arguments.size()), defaultIncludeHandler.Get(),
            IID_PPV_ARGS(compiledResult.GetAddressOf())));

        ComPtr<IDxcBlobUtf8> errors;
        compiledResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
        if (errors && errors->GetStringLength() > 0)
        {
            IG_LOG(ShaderBlobLog, Fatal, "Failed to compile shader {}; {}", desc.SourcePath.ToStringView(), errors->GetStringPointer());
        }

        IG_VERIFY_SUCCEEDED(compiledResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
    }
} // namespace ig
