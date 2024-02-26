#include <D3D12/ShaderBlob.h>
#include <Core/Container.h>
#include <Core/Log.h>
#include <Engine.h>

namespace fe::dx
{
	FE_DEFINE_LOG_CATEGORY(ShaderBlobFatal, ELogVerbosiy::Fatal)

	// #todo 쉐이더 컴파일 후 파일로 저장/캐싱
	ShaderBlob::ShaderBlob(const ShaderCompileDesc& desc)
		: type(desc.Type)
	{
		std::vector<const wchar_t*> arguments;
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
				verify(false);
				break;
		}

		arguments.push_back(TEXT("-Qstrip_debug"));
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

#if defined(DEBUG) || defined(_DEBUG)
		const bool bEnableDebugInfo = true;
#else
		const bool bEnableDebugInfo = desc.bForceEnableDebugInformation;
#endif
		if (bEnableDebugInfo)
		{
			arguments.push_back(TEXT("-Zi"));
		}

		ComPtr<IDxcUtils> utils;
		verify_succeeded(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));

		ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
		utils->CreateDefaultIncludeHandler(defaultIncludeHandler.ReleaseAndGetAddressOf());

		ComPtr<IDxcLibrary> library;
		verify_succeeded(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.ReleaseAndGetAddressOf())));

		uint32_t				 codePage = CP_UTF8;
		const std::wstring		 wideSourcePath = desc.SourcePath.AsWideString();
		ComPtr<IDxcBlobEncoding> sourceBlob;
		verify_succeeded(library->CreateBlobFromFile(wideSourcePath.c_str(), &codePage, &sourceBlob));

		ComPtr<IDxcCompiler3> compiler;
		verify_succeeded(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf())));

		const DxcBuffer buffer{ .Ptr = sourceBlob->GetBufferPointer(),
								.Size = sourceBlob->GetBufferSize(),
								.Encoding = codePage };

		verify_succeeded(compiler->Compile(&buffer, arguments.data(), static_cast<uint32_t>(arguments.size()), defaultIncludeHandler.Get(),
										   IID_PPV_ARGS(compiledResult.GetAddressOf())));

		ComPtr<IDxcBlobUtf8> errors;
		compiledResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			FE_LOG(ShaderBlobFatal, "Failed to compile shader {}; {}", desc.SourcePath.AsStringView(), errors->GetStringPointer());
		}

		verify_succeeded(compiledResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
	}
} // namespace fe::dx