#pragma once
#include <Filesystem/Common.h>
#include <Core/Json.h>

namespace fe
{
    inline std::optional<json> LoadJsonFromFile(const fs::path& path)
    {
        if (!fs::exists(path))
        {
            return std::nullopt;
        }

        std::ifstream fileStream{ path.c_str() };
        if (!fileStream.is_open())
        {
            return std::nullopt;
        }

        json jsonData{ json::parse(fileStream) };
        return jsonData;
    }

    inline bool SaveJsonToFile(const fs::path& path, const json& jsonData)
    {
        std::ofstream fileStream{ path.c_str() };
        if (!fileStream.is_open())
        {
            return false;
        }

        fileStream << jsonData.dump(4);
        fileStream.close();
        return true;
    }

    template <typename T>
    std::optional<T> LoadSerializedDataFromJsonFile(const fs::path& path)
    {
        std::optional<json> serializedData = LoadJsonFromFile(path);
        if (!serializedData)
        {
            return std::nullopt;
        }

        T data{};
        *serializedData >> data;
        return data;
    }

    template <typename T>
    bool SaveSerializedDataToJsonFile(const fs::path& path, const T& data)
    {
        json serializedData{};
        serializedData << data;

        return SaveJsonToFile(path, serializedData);
    }

    inline std::vector<uint8_t> LoadBlobFromFile(const fs::path& path)
    {
        std::vector<uint8_t> blob;
        if (!fs::exists(path))
        {
            return blob;
        }

        std::ifstream blobStream{ path.c_str(), std::ios::in | std::ios::binary };
        if (!blobStream.is_open())
        {
            return blob;
        }

        const size_t sizeOfBlob = fs::file_size(path);
        check(sizeOfBlob > 0);
        blob.resize(sizeOfBlob);
        blobStream.read((char*)blob.data(), blob.size());
        return blob;
    }

    inline bool SaveBlobToFile(const fs::path& path, const std::span<const uint8_t> blob)
    {
        check(blob.size_bytes() > 0);

        std::ofstream fileStream{ path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc };
        if (!fileStream.is_open())
        {
            return false;
        }

        fileStream.write(reinterpret_cast<const char*>(blob.data()), blob.size_bytes());
        fileStream.close();
        return true;
    }

    template <size_t N>
    inline bool SaveBlobsToFile(const fs::path& path, const std::array<std::span<const uint8_t>, N> blobs)
    {
        check(blobs.size() > 0);
        std::ofstream fileStream{ path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc };
        if (!fileStream.is_open())
        {
            return false;
        }

        for (const std::span<const uint8_t> blob : blobs)
        {
            check(blob.size_bytes() > 0);
            fileStream.write(reinterpret_cast<const char*>(blob.data()), blob.size_bytes());
        }

        fileStream.close();
        return true;
    }
} // namespace fe