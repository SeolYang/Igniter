#include "Frieren/Frieren.h"
#include "Frieren/Application/TestApp.h"

int main()
{
    using namespace ig::literals;
    fe::TestApp app{ ig::AppDesc{.WindowTitle = "Test"_fs, .WindowWidth = 2560, .WindowHeight = 1440} };
    return app.Execute();
}