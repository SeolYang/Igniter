#include <D3D12/Commons.h>
#include <Core/String.h>

extern "C"
{
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
}
extern "C"
{
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

namespace fe::Private
{
	void SetD3DObjectName(ID3D12Object* object, std::string_view name)
	{
		if (object != nullptr)
		{
			object->SetName(Wider(name).c_str());
		}
	}
} // namespace fe::Private
