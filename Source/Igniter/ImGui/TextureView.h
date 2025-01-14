#pragma once
#include "Igniter/Igniter.h"

namespace ig::ImGuiX
{
    // 텍스처 뷰 자체가 텍스처 에셋의 소유권을 가지고 있어야함. (Ref >= '1')
    // 일단 AssetInspector의 경우에는 Texture를 중간에 변경해서, 이전에 Submit 했던 텍스처가 GPU 상에 남아 있다면
    // 이전 텍스처를 Unload해서 Ref == 0 이 되었을때, GPU 상에서 사용 중임에도 해제되어 버리는 경우를 막기위한 로직이
    // 기본적으로 포함되어 있음. 다만, 현재 RenderContext의 GPU 리소스 관리 로직에 따르면 자체적으로 이미
    // 해제를 지연 시키고 있기 때문에. 현재로서는 크게 문제 없을 것으로 판단 됨.
    class TextureView final
    {
    };
} // namespace ig::ImGuiX
