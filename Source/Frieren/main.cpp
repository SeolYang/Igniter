#include "Frieren/Frieren.h"
#include "Frieren/Application/TestApp.h"

int main()
{
    fe::TestApp app{ig::AppDesc{.WindowTitle = "Test", .WindowWidth = 2560, .WindowHeight = 1440}};
    return app.Execute();
}
