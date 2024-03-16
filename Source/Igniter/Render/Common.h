#pragma once
#include <Core/Log.h>
#include <Core/Assert.h>
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