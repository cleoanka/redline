#include "../include/engine_sim_application.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#include <libgen.h>
#include <vector>
#include <cstdint>
// Asset paths are resolved relative to the current working directory (GetModulePath
// returns the CWD). When launched by double-clicking the .app the CWD is "/", so make it
// deterministic by switching to the directory containing the executable. In the build
// tree this is build/, so the existing relative paths keep working; inside redline.app it
// is Contents/MacOS/, where delta.conf redirects asset lookups to ../Resources.
static void chdirToExecutableDir() {
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> path(size + 1, 0);
    if (_NSGetExecutablePath(path.data(), &size) != 0) return;
    if (chdir(dirname(path.data())) != 0) { /* best effort */ }
}
#endif

static void runApp(void *handle) {
    EngineSimApplication application;
    application.initialize(handle, ysContextObject::DeviceAPI::Metal);
    application.run();
    application.destroy();
}

#if _WIN32
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;

    runApp(static_cast<void*>(&hInstance));

    return 0;
}

#else
int main() {
#if defined(__APPLE__)
    chdirToExecutableDir();
#endif
    runApp(nullptr);
    return 0;
}

#endif