#include "AvraStudio.h"

#if defined(_WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    avra::studio::AvraStudioComponent studioApp;
    avra::gui::Window window(&studioApp);

    if (!window.createStandalone(L"Avra Studio", 1380, 780)) {
        MessageBoxW(nullptr, L"Failed to create Avra Studio window.", L"Avra Studio Error", MB_ICONERROR);
        return 1;
    }

    window.show();
    return window.runMessageLoop();
}

// Support standard main() for console builds / debugging
int main() {
    return WinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
}
#else
int main() {
    return 0;
}
#endif
