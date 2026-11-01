#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <shellapi.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

namespace {

constexpr char kSfxMagic[8] = { 'A', 'V', 'R', 'A', 'S', 'F', 'X', '1' };

struct SfxFooter {
    uint64_t zipSizeBytes{0};
    char magic[8]{};
};

enum ControlIds : int {
    kIdEditDir       = 101,
    kIdBtnBrowse     = 102,
    kIdChkLicense    = 103,
    kIdChkDesktop    = 104,
    kIdChkStartMenu  = 105,
    kIdChkLaunch     = 106,
    kIdBtnInstall    = 107,
    kIdBtnCancel     = 108
};

HWND g_hwndMain       = nullptr;
HWND g_hwndEditDir    = nullptr;
HWND g_hwndBtnBrowse  = nullptr;
HWND g_hwndBtnInstall = nullptr;
HWND g_hwndBtnCancel  = nullptr;

bool g_chkLicense   = true;
bool g_chkDesktop   = true;
bool g_chkStartMenu = true;
bool g_chkLaunch    = true;
bool g_isInstalling = false;
bool g_isCompleted  = false;
std::wstring g_statusText = L"Ready to install Avra Studio (Win64 Standalone + C++20 VST3/CLAP SDK).";

HFONT g_fontTitle  = nullptr;
HFONT g_fontSub    = nullptr;
HFONT g_fontBody   = nullptr;
HFONT g_fontBold   = nullptr;
HBRUSH g_brEditBg  = nullptr;

std::wstring getSelfExePath() {
    wchar_t buf[MAX_PATH * 2]{};
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH * 2);
    return (len > 0) ? std::wstring(buf, len) : L"";
}

bool extractEmbeddedPayloadZip(const fs::path& destZipPath, std::wstring& outError) {
    const std::wstring selfPath = getSelfExePath();
    std::ifstream in(fs::path(selfPath), std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        outError = L"Could not open installer executable for reading.";
        return false;
    }

    const std::streamoff totalSize = in.tellg();
    if (totalSize < static_cast<std::streamoff>(sizeof(SfxFooter) + 1024)) {
        outError = L"Installer payload footer is missing.";
        return false;
    }

    in.seekg(totalSize - static_cast<std::streamoff>(sizeof(SfxFooter)), std::ios::beg);
    SfxFooter footer{};
    in.read(reinterpret_cast<char*>(&footer), sizeof(SfxFooter));
    if (std::memcmp(footer.magic, kSfxMagic, 8) != 0 || footer.zipSizeBytes == 0 ||
        static_cast<std::streamoff>(footer.zipSizeBytes + sizeof(SfxFooter)) > totalSize) {
        outError = L"Installer payload signature mismatch or corrupted archive.";
        return false;
    }

    const std::streamoff zipStart =
        totalSize - static_cast<std::streamoff>(sizeof(SfxFooter)) - static_cast<std::streamoff>(footer.zipSizeBytes);
    in.seekg(zipStart, std::ios::beg);

    std::ofstream out(destZipPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        outError = L"Could not create temporary extraction archive.";
        return false;
    }

    std::vector<char> chunk(1024 * 1024);
    uint64_t remaining = footer.zipSizeBytes;
    while (remaining > 0 && in.good()) {
        const size_t toRead = static_cast<size_t>(std::min<uint64_t>(remaining, chunk.size()));
        in.read(chunk.data(), static_cast<std::streamsize>(toRead));
        const std::streamsize got = in.gcount();
        if (got <= 0) break;
        out.write(chunk.data(), got);
        remaining -= static_cast<uint64_t>(got);
    }
    out.close();
    if (remaining != 0) {
        outError = L"Incomplete payload extraction read.";
        return false;
    }
    return true;
}

bool runHiddenCommand(std::wstring cmdLine) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back(L'\0');

    if (!CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    while (WaitForSingleObject(pi.hProcess, 50) == WAIT_TIMEOUT) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == 0;
}

bool createShellShortcut(const fs::path& targetExe, const fs::path& workingDir,
                         const fs::path& iconPath, const fs::path& lnkPath,
                         const wchar_t* description) {
    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IShellLinkW, reinterpret_cast<void**>(&psl));
    if (FAILED(hr) || !psl) return false;

    psl->SetPath(targetExe.wstring().c_str());
    psl->SetWorkingDirectory(workingDir.wstring().c_str());
    psl->SetDescription(description);
    if (fs::exists(iconPath)) {
        psl->SetIconLocation(iconPath.wstring().c_str(), 0);
    } else {
        psl->SetIconLocation(targetExe.wstring().c_str(), 0);
    }

    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
    bool ok = false;
    if (SUCCEEDED(hr) && ppf) {
        ok = SUCCEEDED(ppf->Save(lnkPath.wstring().c_str(), TRUE));
        ppf->Release();
    }
    psl->Release();
    return ok;
}

bool writeUninstallerExe(const fs::path& destExePath) {
    const std::wstring selfPath = getSelfExePath();
    std::ifstream in(fs::path(selfPath), std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;

    const std::streamoff totalSize = in.tellg();
    if (totalSize <= 0) return false;

    uint64_t bytesToCopy = static_cast<uint64_t>(totalSize);
    if (totalSize >= static_cast<std::streamoff>(sizeof(SfxFooter) + 1024)) {
        in.seekg(totalSize - static_cast<std::streamoff>(sizeof(SfxFooter)), std::ios::beg);
        SfxFooter footer{};
        in.read(reinterpret_cast<char*>(&footer), sizeof(SfxFooter));
        if (std::memcmp(footer.magic, kSfxMagic, 8) == 0 && footer.zipSizeBytes > 0 &&
            static_cast<std::streamoff>(footer.zipSizeBytes + sizeof(SfxFooter)) < totalSize) {
            bytesToCopy = static_cast<uint64_t>(totalSize) - sizeof(SfxFooter) - footer.zipSizeBytes;
        }
    }

    in.seekg(0, std::ios::beg);
    std::ofstream out(destExePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    std::vector<char> chunk(256 * 1024);
    uint64_t remaining = bytesToCopy;
    while (remaining > 0 && in.good()) {
        const size_t toRead = static_cast<size_t>(std::min<uint64_t>(remaining, chunk.size()));
        in.read(chunk.data(), static_cast<std::streamsize>(toRead));
        const std::streamsize got = in.gcount();
        if (got <= 0) break;
        out.write(chunk.data(), got);
        remaining -= static_cast<uint64_t>(got);
    }
    out.close();
    return remaining == 0;
}

void removeAllShortcuts() {
    std::error_code ec;
    const int csidls[] = { CSIDL_DESKTOPDIRECTORY, CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_PROGRAMS, CSIDL_COMMON_PROGRAMS };
    for (int csidl : csidls) {
        wchar_t folderBuf[MAX_PATH]{};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, csidl, nullptr, 0, folderBuf))) {
            fs::remove(fs::path(folderBuf) / L"Avra Studio.lnk", ec);
        }
    }
    wchar_t userProfile[MAX_PATH]{};
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
        fs::remove(fs::path(userProfile) / L"Desktop" / L"Avra Studio.lnk", ec);
        fs::remove(fs::path(userProfile) / L"OneDrive" / L"Desktop" / L"Avra Studio.lnk", ec);
    }
}

void registerUninstaller(const fs::path& installDir) {
    std::error_code ec;
    fs::remove(installDir / L"Uninstall_Avra_Studio.bat", ec);

    const fs::path uninstExe = installDir / L"Uninstall_Avra_Studio.exe";
    writeUninstallerExe(uninstExe);

    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AvraStudio",
                        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS && hKey) {
        const std::wstring dispName = L"Avra Studio (Open-Source Audio Plugin Workstation)";
        const std::wstring publisher = L"Avra Audio";
        const std::wstring version = L"1.0.0";
        const std::wstring iconStr = (installDir / L"Avra.exe").wstring();
        const std::wstring uninstStr = L"\"" + uninstExe.wstring() + L"\" --uninstall";
        const std::wstring quietUninstStr = L"\"" + uninstExe.wstring() + L"\" --uninstall --silent";
        const std::wstring locStr = installDir.wstring();
        DWORD noModify = 1;

        RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, reinterpret_cast<const BYTE*>(dispName.c_str()),
                       static_cast<DWORD>((dispName.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"Publisher", 0, REG_SZ, reinterpret_cast<const BYTE*>(publisher.c_str()),
                       static_cast<DWORD>((publisher.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ, reinterpret_cast<const BYTE*>(version.c_str()),
                       static_cast<DWORD>((version.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ, reinterpret_cast<const BYTE*>(iconStr.c_str()),
                       static_cast<DWORD>((iconStr.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, reinterpret_cast<const BYTE*>(locStr.c_str()),
                       static_cast<DWORD>((locStr.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, reinterpret_cast<const BYTE*>(uninstStr.c_str()),
                       static_cast<DWORD>((uninstStr.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"QuietUninstallString", 0, REG_SZ, reinterpret_cast<const BYTE*>(quietUninstStr.c_str()),
                       static_cast<DWORD>((quietUninstStr.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"NoModify", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&noModify), sizeof(DWORD));
        RegSetValueExW(hKey, L"NoRepair", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&noModify), sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

int runUninstallerFlow(fs::path installDir, bool silentMode) {
    std::error_code ec;
    const fs::path selfExe = fs::path(getSelfExePath());

    if (installDir.empty()) {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AvraStudio",
                          0, KEY_READ, &hKey) == ERROR_SUCCESS && hKey) {
            wchar_t locBuf[MAX_PATH * 2]{};
            DWORD bufBytes = sizeof(locBuf);
            DWORD type = 0;
            if (RegQueryValueExW(hKey, L"InstallLocation", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(locBuf), &bufBytes) == ERROR_SUCCESS &&
                (type == REG_SZ || type == REG_EXPAND_SZ) && locBuf[0] != L'\0') {
                installDir = fs::path(locBuf);
            }
            RegCloseKey(hKey);
        }
    }
    if (installDir.empty()) {
        installDir = selfExe.parent_path();
    }

    if (!silentMode) {
        const std::wstring prompt =
            L"Are you sure you want to completely uninstall Avra Studio and remove all installed files from:\n\n" +
            installDir.wstring() +
            L"\n\nYour Desktop and Start Menu shortcuts will also be removed.";
        const int resp = MessageBoxW(nullptr, prompt.c_str(), L"Avra Studio Uninstaller",
                                     MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND);
        if (resp != IDYES) {
            return 0;
        }
    }

    // 1. Remove Desktop (including OneDrive Desktop) and Start Menu shortcuts
    removeAllShortcuts();

    // 2. Remove Control Panel Add/Remove Programs registry entry
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AvraStudio");

    // 3. Remove installed files & directories (never touch git development repos or drive roots)
    const bool isSafeToWipe =
        !installDir.empty() &&
        installDir.has_relative_path() &&
        installDir != installDir.root_path() &&
        !fs::exists(installDir / L".git", ec);

    if (isSafeToWipe && fs::exists(installDir, ec)) {
        for (const auto& entry : fs::directory_iterator(installDir, ec)) {
            std::error_code eqEc;
            if (fs::equivalent(entry.path(), selfExe, eqEc)) {
                continue;
            }
            fs::remove_all(entry.path(), ec);
        }
    }

    if (!silentMode) {
        MessageBoxW(nullptr,
                    L"Avra Studio has been completely uninstalled from your computer.",
                    L"Avra Studio Uninstaller",
                    MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
    }

    // 4. Schedule self-deletion of Uninstall_Avra_Studio.exe and the empty installDir after process exit
    if (isSafeToWipe && fs::exists(installDir, ec)) {
        const fs::path tempDir = fs::temp_directory_path(ec);
        SetCurrentDirectoryW(tempDir.wstring().c_str());

        const std::wstring cleanupCmd =
            L"cmd.exe /c ping 127.0.0.1 -n 2 >nul & del /f /q \"" + selfExe.wstring() +
            L"\" >nul 2>&1 & rmdir /s /q \"" + installDir.wstring() + L"\" >nul 2>&1";
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};
        std::vector<wchar_t> mutableCmd(cleanupCmd.begin(), cleanupCmd.end());
        mutableCmd.push_back(L'\0');
        if (CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                           CREATE_NO_WINDOW, nullptr, tempDir.wstring().c_str(), &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }

    return 0;
}

bool performInstallation(const fs::path& targetDir, bool createShortcuts, std::wstring& outError) {
    std::error_code ec;
    fs::create_directories(targetDir, ec);
    if (!fs::exists(targetDir, ec)) {
        outError = L"Could not create target installation directory: " + targetDir.wstring();
        return false;
    }

    // If installing into an existing non-git directory, purge stale build/examples/studio/tests
    // folders so no old CMake targets or leftover files pollute the clean installation.
    if (!fs::exists(targetDir / L".git", ec)) {
        for (const wchar_t* staleDir : { L"build", L"examples", L"studio", L"tests", L"installer", L"scripts", L"dist", L"presets", L"plugins" }) {
            fs::remove_all(targetDir / staleDir, ec);
        }
    }

    const fs::path tempZip = fs::temp_directory_path(ec) / L"Avra_Studio_Setup_Payload.zip";
    if (!extractEmbeddedPayloadZip(tempZip, outError)) {
        return false;
    }

    // Extract using Windows built-in tar.exe (fast C libarchive), with PowerShell Expand-Archive fallback
    const std::wstring tarCmd =
        L"tar.exe -xf \"" + tempZip.wstring() + L"\" -C \"" + targetDir.wstring() + L"\"";
    bool extracted = runHiddenCommand(tarCmd);
    if (!extracted) {
        const std::wstring psCmd =
            L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -Path '" +
            tempZip.wstring() + L"' -DestinationPath '" + targetDir.wstring() + L"' -Force\"";
        extracted = runHiddenCommand(psCmd);
    }
    fs::remove(tempZip, ec);

    if (!extracted || !fs::exists(targetDir / L"Avra.exe", ec)) {
        outError = L"Failed to extract Avra Studio files into " + targetDir.wstring();
        return false;
    }

    fs::create_directories(targetDir / L"plugins", ec);
    fs::create_directories(targetDir / L"presets", ec);

    if (createShortcuts) {
        const fs::path exePath = targetDir / L"Avra.exe";
        const fs::path icoPath = targetDir / L"assets" / L"avra_studio.ico";

        if (g_chkDesktop) {
            wchar_t desktopBuf[MAX_PATH]{};
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktopBuf))) {
                createShellShortcut(exePath, targetDir, icoPath,
                                    fs::path(desktopBuf) / L"Avra Studio.lnk",
                                    L"Avra Studio - Visual Audio Plugin Workstation");
            }
        }
        if (g_chkStartMenu) {
            wchar_t programsBuf[MAX_PATH]{};
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAMS, nullptr, 0, programsBuf))) {
                createShellShortcut(exePath, targetDir, icoPath,
                                    fs::path(programsBuf) / L"Avra Studio.lnk",
                                    L"Avra Studio - Visual Audio Plugin Workstation");
            }
        }
    }
    registerUninstaller(targetDir);

    return true;
}

void drawCustomCheckbox(HDC hdc, int x, int y, bool checked, const wchar_t* label) {
    RECT box = { x, y, x + 18, y + 18 };
    HBRUSH fillBr = CreateSolidBrush(checked ? RGB(16, 62, 58) : RGB(20, 26, 34));
    HPEN borderPen = CreatePen(PS_SOLID, 1, checked ? RGB(0, 245, 212) : RGB(62, 82, 98));
    HGDIOBJ oldBr = SelectObject(hdc, fillBr);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    RoundRect(hdc, box.left, box.top, box.right, box.bottom, 4, 4);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBr);
    DeleteObject(borderPen);

    if (checked) {
        HPEN chkPen = CreatePen(PS_SOLID, 2, RGB(0, 245, 212));
        HGDIOBJ op = SelectObject(hdc, chkPen);
        MoveToEx(hdc, x + 4, y + 9, nullptr);
        LineTo(hdc, x + 8, y + 13);
        LineTo(hdc, x + 14, y + 5);
        SelectObject(hdc, op);
        DeleteObject(chkPen);
    }

    SelectObject(hdc, g_fontBody);
    SetTextColor(hdc, RGB(224, 234, 242));
    RECT txtR = { x + 26, y - 1, 590, y + 20 };
    DrawTextW(hdc, label, -1, &txtR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK SetupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_fontTitle = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_fontSub   = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_fontBody  = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_fontBold  = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_brEditBg  = CreateSolidBrush(RGB(18, 24, 32));

        g_hwndEditDir = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"C:\\AvraStudio",
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                        28, 224, 456, 28, hwnd, reinterpret_cast<HMENU>(kIdEditDir),
                                        GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_hwndEditDir, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBody), TRUE);

        g_hwndBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          494, 223, 96, 30, hwnd, reinterpret_cast<HMENU>(kIdBtnBrowse),
                                          GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_hwndBtnBrowse, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBody), TRUE);

        g_hwndBtnInstall = CreateWindowExW(0, L"BUTTON", L"INSTALL AVRA STUDIO",
                                           WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                           280, 412, 196, 34, hwnd, reinterpret_cast<HMENU>(kIdBtnInstall),
                                           GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_hwndBtnInstall, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBold), TRUE);

        g_hwndBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          486, 412, 104, 34, hwnd, reinterpret_cast<HMENU>(kIdBtnCancel),
                                          GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_hwndBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBody), TRUE);
        return 0;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdcEdit, RGB(240, 246, 250));
        SetBkColor(hdcEdit, RGB(18, 24, 32));
        return reinterpret_cast<LRESULT>(g_brEditBg);
    }

    case WM_LBUTTONDOWN: {
        if (g_isInstalling) return 0;
        const int mx = static_cast<short>(LOWORD(lParam));
        const int my = static_cast<short>(HIWORD(lParam));
        if (mx >= 28 && mx <= 580) {
            if (my >= 268 && my <= 290) {
                g_chkLicense = !g_chkLicense;
                EnableWindow(g_hwndBtnInstall, g_chkLicense ? TRUE : FALSE);
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (my >= 296 && my <= 318) {
                g_chkDesktop = !g_chkDesktop;
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (my >= 324 && my <= 346) {
                g_chkStartMenu = !g_chkStartMenu;
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (my >= 352 && my <= 374) {
                g_chkLaunch = !g_chkLaunch;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == kIdBtnBrowse && !g_isInstalling) {
            BROWSEINFOW bi{};
            bi.hwndOwner = hwnd;
            bi.lpszTitle = L"Select Destination Folder for Avra Studio:";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t pathBuf[MAX_PATH]{};
                if (SHGetPathFromIDListW(pidl, pathBuf)) {
                    fs::path chosen(pathBuf);
                    if (chosen.filename() != L"Avra") {
                        chosen /= L"Avra";
                    }
                    SetWindowTextW(g_hwndEditDir, chosen.wstring().c_str());
                }
                CoTaskMemFree(pidl);
            }
            return 0;
        }
        if (id == kIdBtnCancel) {
            PostQuitMessage(0);
            return 0;
        }
        if (id == kIdBtnInstall && !g_isInstalling) {
            if (g_isCompleted) {
                PostQuitMessage(0);
                return 0;
            }
            if (!g_chkLicense) {
                MessageBoxW(hwnd, L"Please accept the Avra Studio Non-Commercial License Agreement to continue.",
                            L"Avra Studio Setup", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            wchar_t dirBuf[MAX_PATH * 2]{};
            GetWindowTextW(g_hwndEditDir, dirBuf, MAX_PATH * 2);
            fs::path targetDir(dirBuf);

            g_isInstalling = true;
            EnableWindow(g_hwndBtnInstall, FALSE);
            EnableWindow(g_hwndBtnBrowse, FALSE);
            EnableWindow(g_hwndEditDir, FALSE);
            g_statusText = L"Extracting Avra Studio workstation, 150-module DSP library & assets...";
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);

            std::wstring err;
            if (performInstallation(targetDir, true, err)) {
                g_isInstalling = false;
                g_isCompleted = true;
                g_statusText = L"Installation complete! Installed to: " + targetDir.wstring();
                SetWindowTextW(g_hwndBtnInstall, L"FINISH");
                EnableWindow(g_hwndBtnInstall, TRUE);
                ShowWindow(g_hwndBtnCancel, SW_HIDE);
                InvalidateRect(hwnd, nullptr, FALSE);

                if (g_chkLaunch) {
                    const fs::path exePath = targetDir / L"Avra.exe";
                    ShellExecuteW(nullptr, L"open", exePath.wstring().c_str(), nullptr,
                                  targetDir.wstring().c_str(), SW_SHOWNORMAL);
                    PostQuitMessage(0);
                }
            } else {
                g_isInstalling = false;
                g_statusText = L"Error: " + err;
                EnableWindow(g_hwndBtnInstall, TRUE);
                EnableWindow(g_hwndBtnBrowse, TRUE);
                EnableWindow(g_hwndEditDir, TRUE);
                InvalidateRect(hwnd, nullptr, FALSE);
                MessageBoxW(hwnd, err.c_str(), L"Avra Studio Setup Error", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc{};
        GetClientRect(hwnd, &rc);

        // Background
        HBRUSH bgBr = CreateSolidBrush(RGB(15, 20, 26));
        FillRect(hdc, &rc, bgBr);
        DeleteObject(bgBr);

        // Top Banner
        RECT hdrR = { 0, 0, rc.right, 72 };
        HBRUSH hdrBr = CreateSolidBrush(RGB(22, 30, 40));
        FillRect(hdc, &hdrR, hdrBr);
        DeleteObject(hdrBr);

        HPEN cyanLine = CreatePen(PS_SOLID, 2, RGB(0, 245, 212));
        HGDIOBJ oldPen = SelectObject(hdc, cyanLine);
        MoveToEx(hdc, 0, 72, nullptr);
        LineTo(hdc, rc.right, 72);
        SelectObject(hdc, oldPen);
        DeleteObject(cyanLine);

        // Draw App Icon in Banner
        HICON hIco = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101));
        if (hIco) {
            DrawIconEx(hdc, 22, 14, hIco, 44, 44, 0, nullptr, DI_NORMAL);
        }

        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, g_fontTitle);
        SetTextColor(hdc, RGB(242, 248, 252));
        RECT titleR = { 78, 12, rc.right - 20, 40 };
        DrawTextW(hdc, L"AVRA STUDIO \x2014 WINDOWS x64 SETUP", -1, &titleR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, g_fontSub);
        SetTextColor(hdc, RGB(0, 245, 212));
        RECT subR = { 78, 38, rc.right - 20, 62 };
        DrawTextW(hdc, L"Visual C++20 Audio Plugin Workstation & 150-Module DSP Library (Free Non-Commercial)",
                  -1, &subR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // License Card
        RECT licBox = { 28, 86, rc.right - 28, 194 };
        HBRUSH licBr = CreateSolidBrush(RGB(19, 26, 34));
        HPEN licPen = CreatePen(PS_SOLID, 1, RGB(44, 62, 78));
        HGDIOBJ ob = SelectObject(hdc, licBr);
        HGDIOBJ op = SelectObject(hdc, licPen);
        RoundRect(hdc, licBox.left, licBox.top, licBox.right, licBox.bottom, 6, 6);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(licBr);
        DeleteObject(licPen);

        SelectObject(hdc, g_fontBold);
        SetTextColor(hdc, RGB(255, 184, 76));
        RECT licHdr = { 40, 94, rc.right - 40, 114 };
        DrawTextW(hdc, L"AVRA STUDIO NON-COMMERCIAL OPEN-SOURCE LICENSE", -1, &licHdr, DT_LEFT | DT_SINGLELINE);

        SelectObject(hdc, g_fontBody);
        SetTextColor(hdc, RGB(204, 216, 226));
        RECT licTxt = { 40, 116, rc.right - 40, 188 };
        const wchar_t* kLicSummary =
            L"1. Avra Studio is 100% free for personal, educational, and studio music production.\n"
            L"2. You may freely use Avra Studio and its exported VST3/CLAP/Standalone plugins in any music.\n"
            L"3. Selling, paywalling, or commercially redistributing Avra Studio or plugins built with Avra Studio\n"
            L"   is strictly prohibited.";
        DrawTextW(hdc, kLicSummary, -1, &licTxt, DT_LEFT | DT_WORDBREAK);

        // Destination Folder Label
        SelectObject(hdc, g_fontBold);
        SetTextColor(hdc, RGB(230, 238, 245));
        RECT dirLbl = { 28, 202, 400, 222 };
        DrawTextW(hdc, L"Destination Folder:", -1, &dirLbl, DT_LEFT | DT_SINGLELINE);

        // Checkboxes
        drawCustomCheckbox(hdc, 28, 270, g_chkLicense,
                           L"I accept the Avra Studio Non-Commercial License Agreement (Not for Commercial Sale)");
        drawCustomCheckbox(hdc, 28, 298, g_chkDesktop,
                           L"Create Desktop Shortcut (Avra Studio)");
        drawCustomCheckbox(hdc, 28, 326, g_chkStartMenu,
                           L"Create Start Menu Shortcut");
        drawCustomCheckbox(hdc, 28, 354, g_chkLaunch,
                           L"Launch Avra Studio when setup finishes");

        // Status Bar Box
        RECT stBox = { 28, 382, rc.right - 28, 404 };
        SelectObject(hdc, g_fontSub);
        SetTextColor(hdc, g_isCompleted ? RGB(0, 245, 212) : RGB(255, 184, 76));
        DrawTextW(hdc, g_statusText.c_str(), -1, &stBox, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_fontTitle) DeleteObject(g_fontTitle);
        if (g_fontSub)   DeleteObject(g_fontSub);
        if (g_fontBody)  DeleteObject(g_fontBody);
        if (g_fontBold)  DeleteObject(g_fontBold);
        if (g_brEditBg)  DeleteObject(g_brEditBg);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool silentMode = false;
    bool uninstallMode = false;
    fs::path customDir;

    const std::wstring selfFilename = fs::path(getSelfExePath()).filename().wstring();
    if (selfFilename.find(L"Uninstall") != std::wstring::npos ||
        selfFilename.find(L"uninstall") != std::wstring::npos) {
        uninstallMode = true;
    }

    if (argv) {
        for (int i = 1; i < argc; ++i) {
            std::wstring arg = argv[i];
            if (arg == L"/S" || arg == L"--silent" || arg == L"-s") {
                silentMode = true;
            } else if (arg == L"--uninstall" || arg == L"/U" || arg == L"/uninstall") {
                uninstallMode = true;
            } else if ((arg == L"--dir" || arg == L"/D") && i + 1 < argc) {
                customDir = argv[++i];
            }
        }
        LocalFree(argv);
    }

    if (uninstallMode) {
        const int rc = runUninstallerFlow(customDir, silentMode);
        CoUninitialize();
        return rc;
    }

    if (customDir.empty()) {
        customDir = L"C:\\AvraStudio";
    }

    if (silentMode) {
        std::wstring err;
        const bool ok = performInstallation(customDir, false, err);
        CoUninitialize();
        return ok ? 0 : 1;
    }

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = SetupWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    wc.lpszClassName = L"AvraStudioSetupWindowClass";
    RegisterClassExW(&wc);

    const int winW = 626;
    const int winH = 496;
    const int scrW = GetSystemMetrics(SM_CXSCREEN);
    const int scrH = GetSystemMetrics(SM_CYSCREEN);
    const int winX = (scrW - winW) / 2;
    const int winY = (scrH - winH) / 2;

    g_hwndMain = CreateWindowExW(
        0, wc.lpszClassName, L"Avra Studio Setup (Win64)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        winX, winY, winW, winH, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return 0;
}
