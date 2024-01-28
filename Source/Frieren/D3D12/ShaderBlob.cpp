#include <D3D12/ShaderBlob.h>
#include <array>

namespace fe::dx
{
	// #todo 쉐이더 컴파일 후 파일로 저장
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
				FE_ASSERT(false);
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
			arguments.push_back(TEXT("WX"));
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
		FE_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf()))));

		ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
		utils->CreateDefaultIncludeHandler(defaultIncludeHandler.ReleaseAndGetAddressOf());

		ComPtr<IDxcLibrary> library;
		FE_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(library.ReleaseAndGetAddressOf()))));

		uint32_t				 codePage = CP_UTF8;
		const std::wstring		 wideSourcePath = desc.SourcePath.AsWideString();
		ComPtr<IDxcBlobEncoding> sourceBlob;
		FE_SUCCEEDED_ASSERT(library->CreateBlobFromFile(wideSourcePath.c_str(), &codePage, &sourceBlob));

		ComPtr<IDxcCompiler3> compiler;
		FE_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf()))));

		const DxcBuffer buffer{
			.Ptr = sourceBlob->GetBufferPointer(),
			.Size = sourceBlob->GetBufferSize(),
			.Encoding = codePage
		};

		ComPtr<IDxcResult> result;
		FE_SUCCEEDED_ASSERT(compiler->Compile(
			&buffer,
			arguments.data(), arguments.size(),
			defaultIncludeHandler.Get(),
			IID_PPV_ARGS(result.GetAddressOf())));

		// #todo 별도의 error handling 구현 https://youtu.be/tyyKeTsdtmo?si=gERRzeRVmqxAcPT7&t=1158
		// ComPtr<IDxcBlobUtf8> errors;
		// result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
		// if (errors && errors->GetStringLength() > 0)
		//{
		//	Logging(Error, (char*)errors->GetBufferPointer());
		//}

		FE_SUCCEEDED_ASSERT(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
	}
} // namespace fe::dx