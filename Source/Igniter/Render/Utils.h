#pragma once
#include <Igniter.h>

namespace ig
{
    /* Engine/HLSL => Row Vector(Pre-Multiplication) */
    inline Matrix ConvertToShaderSuitableForm(const Matrix& rowMajorRowVectorMat)
    {
        return rowMajorRowVectorMat.Transpose();
    }
}    // namespace ig
