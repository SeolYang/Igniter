#include "Frieren/Frieren.h"
#include "Frieren/Application/TestApp.h"

int main()
{
    using namespace ig::literals;
    fe::TestApp app{ig::AppDesc{.WindowTitle = "Test"_fs, .WindowWidth = 1920, .WindowHeight = 1080}};
    return app.Execute();
}