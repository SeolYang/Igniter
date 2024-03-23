#pragma once
#include <PCH.h>

namespace ig
{
    /* Engine/HLSL => Row Vector(Pre-Multiplication) */
    inline Matrix ConvertToShaderSuitableForm(const Matrix& rowMajorRowVectorMat)
    {
        return rowMajorRowVectorMat.Transpose();
    }
} // namespace ig