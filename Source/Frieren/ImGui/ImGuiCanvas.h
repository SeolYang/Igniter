#pragma once
#include <ImGui/Common.h>
#include <ImGui/ImGuiLayer.h>
#include <Core/Container.h>

namespace fe
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
                }
            }
        }

        void RemoveLayer(const String layerName)
        {
            for (auto itr = layers.begin(); itr != layers.end(); ++itr)
            {
                if ((*itr)->GetName() == layerName)
                {
                    layers.erase(itr);
                    break;
                }
            }
        }

    private:
        std::vector<std::unique_ptr<ImGuiLayer>> layers;
    };
} // namespace fe