#pragma once
#include <Core/Assert.h>
#pragma warning(push)
#pragma warning(disable : 26800)
#pragma warning(disable : 26819)
#include <nlohmann/json.hpp>
#pragma warning(pop)
#include <magic_enum.hpp>

namespace fe::details
{
	template <typename VarType>
	void Serialize(nlohmann::json& archive, VarType&& var, const std::string_view dataTypeName, const std::string_view varName)
	{
		if (!archive.contains(dataTypeName))
		{
			archive[dataTypeName] = nlohmann::json({});
		}
		check(archive[dataTypeName].is_object());
		archive[dataTypeName][varName] = std::forward<VarType>(var);
	}

	template <typename VarType, typename F>
	void DeSerialize(const nlohmann::json& archive, VarType& var, const std::string_view dataTypeName, const std::string_view varName, F callback)
	{
		check(!archive.is_null());
		check(archive.contains(dataTypeName) && archive[dataTypeName].is_object());
		verify(archive[dataTypeName].contains(varName));
		callback(archive, var);
	}
} // namespace fe::details

#define FE_SERIALIZE_JSON(DataType, JsonArchive, Data, Var) \
	fe::details::Serialize(JsonArchive, Data.Var, #DataType, #Var)

#define FE_SERIALIZE_GUID_JSON(DataType, JsonArchive, Data, Var) \
	fe::details::Serialize(JsonArchive, Data.Var.str(), #DataType, #Var)

#define FE_SERIALIZE_ENUM_JSON(DataType, JsonArchive, Data, Var) \
	fe::details::Serialize(JsonArchive, magic_enum::enum_name<std::decay_t<decltype(Data.Var)>>(Data.Var), #DataType, #Var)

#define FE_DESERIALIZE_JSON(DataType, JsonArchive, Data, Var)                                                                                  \
	fe::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                         \
															   [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) {      \
																   var = archive[#DataType][#Var].template get<std::decay_t<decltype(var)>>(); \
															   })

#define FE_DESERIALIZE_GUID_JSON(DataType, JsonArchive, Data, Var)                                                                               \
	fe::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                           \
															   [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) {        \
																   var = xg::Guid(archive[#DataType][#Var].template get<std::string>().c_str()); \
															   })

#define FE_DESERIALIZE_ENUM_JSON(DataType, JsonArchive, Data, Var, FallbackVal)                                                                                                                               \
	fe::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                                                                                        \
															   [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) {                                                                     \
																   var = magic_enum::enum_cast<std::decay_t<decltype(Data.Var)>>(archive[#DataType][#Var].template get<std::string>()).value_or(FallbackVal); \
															   })