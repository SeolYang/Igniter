#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    /* Engine/HLSL => Row Vector(Pre-Multiplication) */
    inline Matrix ConvertToShaderSuitableForm(const Matrix& rowMajorRowVectorMat)
    {
        return rowMajorRowVectorMat.Transpose();
    }
}    // namespace ig
