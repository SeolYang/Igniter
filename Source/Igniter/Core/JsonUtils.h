#pragma once
#include <Igniter.h>

namespace ig::details
{
    template <typename VarType>
    void Serialize(nlohmann::json& archive, VarType&& var, const std::string_view dataTypeName, const std::string_view varName)
    {
        if (!archive.contains(dataTypeName))
        {
            archive[dataTypeName] = nlohmann::json({});
        }
        IG_CHECK(archive[dataTypeName].is_object());
        archive[dataTypeName][varName] = std::forward<VarType>(var);
    }

    template <typename VarType, typename F>
    void DeSerialize(const nlohmann::json& archive, VarType& var, const std::string_view dataTypeName, const std::string_view varName, F callback)
    {
        IG_CHECK(archive.contains(dataTypeName) && archive[dataTypeName].is_object());
        if (archive.contains(dataTypeName) && archive[dataTypeName].contains(varName))
        {
            callback(archive, var);
        }
    }
} // namespace ig::details

#define IG_SERIALIZE_JSON(DataType, JsonArchive, Data, Var) \
    ig::details::Serialize(JsonArchive, Data.Var, #DataType, #Var)

#define IG_SERIALIZE_GUID_JSON(DataType, JsonArchive, Data, Var) \
    ig::details::Serialize(JsonArchive, Data.Var.str(), #DataType, #Var)

#define IG_SERIALIZE_ENUM_JSON(DataType, JsonArchive, Data, Var) \
    ig::details::Serialize(JsonArchive, magic_enum::enum_name<std::decay_t<decltype(Data.Var)>>(Data.Var), #DataType, #Var)

#define IG_SERIALIZE_STRING_JSON(DataType, JsonArchive, Data, Var) \
    ig::details::Serialize(JsonArchive, Data.Var.ToStringView(), #DataType, #Var)

#define IG_DESERIALIZE_JSON(DataType, JsonArchive, Data, Var)                                                                                  \
    ig::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                         \
                                                               [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) {      \
                                                                   var = archive[#DataType][#Var].template get<std::decay_t<decltype(var)>>(); \
                                                               })

#define IG_DESERIALIZE_GUID_JSON(DataType, JsonArchive, Data, Var)                                                                               \
    ig::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                           \
                                                               [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) {        \
                                                                   var = xg::Guid(archive[#DataType][#Var].template get<std::string>().c_str()); \
                                                               })

#define IG_DESERIALIZE_ENUM_JSON(DataType, JsonArchive, Data, Var, FallbackVal)                                                                                                                               \
    ig::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                                                                                        \
                                                               [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) {                                                                     \
                                                                   var = magic_enum::enum_cast<std::decay_t<decltype(Data.Var)>>(archive[#DataType][#Var].template get<std::string>()).value_or(FallbackVal); \
                                                               })

#define IG_DESERIALIZE_STRING_JSON(DataType, JsonArchive, Data, Var)                                                                      \
    ig::details::DeSerialize<std::decay_t<decltype(Data.Var)>>(JsonArchive, Data.Var, #DataType, #Var,                                    \
                                                               [](const nlohmann::json& archive, std::decay_t<decltype(Data.Var)>& var) { \
                                                                   var = archive[#DataType][#Var].template get<std::string>();            \
                                                               })
