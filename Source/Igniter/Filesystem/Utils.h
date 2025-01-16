#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    inline Json LoadJsonFromFile(const Path& path)
    {
        if (!fs::exists(path))
        {
            return Json{};
        }

        std::ifstream fileStream{path.c_str()};
        if (!fileStream.is_open())
        {
            return Json{};
        }

        Json newJson{Json::parse(fileStream, nullptr, false)};
        return newJson.is_discarded() ? Json{} : newJson;
    }

    inline bool SaveJsonToFile(const Path& path, const Json& jsonData)
    {
        std::ofstream fileStream{path.c_str()};
        if (!fileStream.is_open())
        {
            return false;
        }

        fileStream << jsonData.dump(4);
        fileStream.close();
        return true;
    }

    inline Vector<uint8_t> LoadBlobFromFile(const Path& path)
    {
        Vector<uint8_t> blob;
        if (!fs::exists(path))
        {
            return blob;
        }

        std::ifstream blobStream{path.c_str(), std::ios::in | std::ios::binary};
        if (!blobStream.is_open())
        {
            return blob;
        }

        const Size sizeOfBlob = fs::file_size(path);
        IG_CHECK(sizeOfBlob > 0);
        blob.resize(sizeOfBlob);
        blobStream.read((char*)blob.data(), blob.size());
        return blob;
    }

    inline bool SaveBlobToFile(const Path& path, const std::span<const uint8_t> blob)
    {
        IG_CHECK(blob.size_bytes() > 0);

        std::ofstream fileStream{path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc};
        if (!fileStream.is_open())
        {
            return false;
        }

        fileStream.write(reinterpret_cast<const char*>(blob.data()), blob.size_bytes());
        fileStream.close();
        return true;
    }

    template <Size N>
    inline bool SaveBlobsToFile(const Path& path, const std::array<std::span<const uint8_t>, N> blobs)
    {
        IG_CHECK(blobs.size() > 0);
        std::ofstream fileStream{path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc};
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
