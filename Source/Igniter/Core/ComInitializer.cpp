#include "Igniter/Igniter.h"
#include "Igniter/Core/ComInitializer.h"

namespace ig
{
    class CoInitializer final
    {
    public:
        CoInitializer()
            : result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))
        {}

        ~CoInitializer() { CoUninitialize(); }

        HRESULT GetResult() const { return result; }

    private:
        HRESULT result;
    };

    HRESULT CoInitializeUnique()
    {
        /* #sy_note 윈도우의 FileDialog 등 API를 사용하기 위해선 'COINIT_APARTMENTTHREADED' 가 강제됨. */
        /* #sy_improvements https://github.com/aiekick/ImGuiFileDialog 또는 개별적으로 구현 고려 */
        /* https://github.com/mlabbe/nativefiledialog/blob/master/src/nfd_win.cpp 다른 라이브러리도 ::COINIT_APARTMENTTHREADED |
         * ::COINIT_DISABLE_OLE1DDE 로 초기화 해주고 있다.. */
        static thread_local CoInitializer initializer;
        return initializer.GetResult();
    }
} // namespace ig
