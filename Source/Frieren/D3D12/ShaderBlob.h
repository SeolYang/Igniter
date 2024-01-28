#pragma once
#include <D3D12/Common.h>
#include <Core/String.h>

namespace fe::dx
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

	class ShaderCompileDesc
	{
	public:
		String					 SourcePath;
		EShaderType				 Type;
		bool					 bPackMarticesInRowMajor = false;				   // -Zpr
		EShaderOptimizationLevel OptimizationLevel = EShaderOptimizationLevel::O3; // -Od~-O3
		bool					 bDisableValidation = false;					   // -Vd
		bool					 bTreatWarningAsErrors = false;					   // -WX
		bool					 bForceEnableDebugInformation = false;			   // -Zi
																				   // #todo Define Macros
	};

	// from compiled binary
	class ShaderBlob
	{
	public:
		ShaderBlob(const ShaderCompileDesc& desc);
		~ShaderBlob() = default;

		EShaderType GetType() const { return type; }
		IDxcBlob&	GetNative() const { return *shader.Get(); }

	private:
		const EShaderType type;
		ComPtr<IDxcBlob>  shader;
	};
} // namespace fe::dx