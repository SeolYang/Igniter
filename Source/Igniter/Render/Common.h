#pragma once
/* #sy_wip rename to Utils */
#include <Core/Log.h>
#include <Core/Container.h>
#include <Core/FrameManager.h>
#include <Math/Common.h>

namespace ig
{
    /* Engine/HLSL => Row Vector(Pre-Multiplication) */
    inline Matrix ConvertToShaderSuitableForm(const Matrix& rowMajorRowVectorMat)
    {
        return rowMajorRowVectorMat.Transpose();
    }
} // namespace ig