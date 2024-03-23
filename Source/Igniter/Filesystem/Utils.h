#pragma once
#include <Igniter.h>
#include <Core/JsonUtils.h>

namespace ig
{
    inline json LoadJsonFromFile(const fs::path& path)
    {
        json newJson{};
        if (fs::exists(path))
        {
            std::ifstream fileStream{ path.c_str() };

            if (fileStream.is_open())
            {
                newJson = json::parse(fileStream);
            }
        }

        return newJson;
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
        IG_CHECK(sizeOfBlob > 0);
        blob.resize(sizeOfBlob);
        blobStream.read((char*)blob.data(), blob.size());
        return blob;
    }

    inline bool SaveBlobToFile(const fs::path& path, const std::span<const uint8_t> blob)
    {
        IG_CHECK(blob.size_bytes() > 0);

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
        IG_CHECK(blobs.size() > 0);
        std::ofstream fileStream{ path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc };
        if (!fileStream.is_open())
        {
            return false;
        }

        for (const std::span<const uint8_t> blob : blobs)
        {
            IG_CHECK(blob.size_bytes() > 0);
            fileStream.write(reinterpret_cast<const char*>(blob.data()), blob.size_bytes());
        }

        fileStream.close();
        return true;
    }
} // namespace ig