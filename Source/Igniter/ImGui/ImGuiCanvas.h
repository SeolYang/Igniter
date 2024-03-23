#pragma once
#include <Igniter.h>
#include <ImGui/ImGuiLayer.h>
#include <Core/ContainerUtils.h>

namespace ig
{
    class ImGuiCanvas final
    {
    public:
        ImGuiCanvas() = default;
        ~ImGuiCanvas();

        void Render();

        template <typename T, typename... Args>
            requires std::derived_from<T, ImGuiLayer>
        T& AddLayer(Args&&... args)
        {
            layers.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            bDirty = true;
            return reinterpret_cast<T&>(*layers.back());
        }

        template <typename T>
            requires std::derived_from<T, ImGuiLayer>
        void RemoveLayer()
        {
            for (auto itr = layers.begin(); itr != layers.end(); ++itr)
            {
                if (typeid(*(*itr)) == typeid(T))
                {
                    layers.erase(itr);
                    bDirty = true;
                    break;
                }
            }
        }

        void RemoveLayer(const uint64_t layerID)
        {
            for (auto itr = layers.begin(); itr != layers.end(); ++itr)
            {
                if ((*itr)->GetID() == layerID)
                {
                    layers.erase(itr);
                    bDirty = true;
                    break;
                }
            }
        }

        template <typename T>
            requires std::derived_from<T, ImGuiLayer>
        void RemoveLayerAll()
        {
            for (auto itr = layers.begin(); itr != layers.end(); ++itr)
            {
                if (typeid(*(*itr)) == typeid(T))
                {
                    layers.erase(itr);
                    bDirty = true;
                }
            }
        }

        bool IsIgnoreInputEnabled() const { return bIgnoreInput; }
        void SetIgnoreInput(const bool bEnableIgnoreInput) { this->bIgnoreInput = bEnableIgnoreInput; }

    private:
        std::vector<std::unique_ptr<ImGuiLayer>> layers;
        bool bIgnoreInput = false;
        bool bDirty = false;
    };
} // namespace ig