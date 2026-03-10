#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include "xorstr.hpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")

bool GetGtaDirectory(std::string& outDir)
{
    HKEY hKey = nullptr;

    LONG res = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        xorstr_("Software\\SAMP"),
        0,
        KEY_READ,
        &hKey
    );

    if (res != ERROR_SUCCESS)
        return false;

    char exePath[512]{};
    DWORD size = sizeof(exePath);

    res = RegQueryValueExA(
        hKey,
        xorstr_("gta_sa_exe"),
        nullptr,
        nullptr,
        reinterpret_cast<LPBYTE>(exePath),
        &size
    );

    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS || exePath[0] == '\0')
        return false;

    std::string fullPath(exePath);
    const std::string filePart = "\\gta_sa.exe";

    size_t pos = fullPath.rfind(filePart);
    if (pos == std::string::npos)
        return false;

    outDir = fullPath.substr(0, pos);
    return true;
}

std::string EnsureSlash(const std::string& path)
{
    if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
        return path;
    return path + "\\";
}

std::wstring ToWide(const std::string& s)
{
    if (s.empty())
        return std::wstring();

    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0)
        return std::wstring();

    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], len);
    return w;
}

void TrimToken(std::string& s)
{
    // Trim whitespace from both ends (for token body)
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();

    size_t start = 0;
    while (start < s.size() && (s[start] == '\r' || s[start] == '\n' || s[start] == ' ' || s[start] == '\t'))
        ++start;

    if (start > 0)
        s = s.substr(start);
}

bool HttpGetBody(const std::wstring& host, const std::wstring& path, std::string& outBody)
{
    outBody.clear();

    HINTERNET hSession = WinHttpOpen(
        L"WraithUpdater/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Custom header so PHP can check we're the client
    WinHttpAddRequestHeaders(
        hRequest,
        L"X-Wraith-Client: 1\r\n",
        -1L,
        WINHTTP_ADDREQ_FLAG_ADD
    );

    BOOL bResult = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Read the response body
    DWORD dwSize = 0;
    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0)
            break;

        std::vector<char> buffer(dwSize + 1);
        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded) || dwDownloaded == 0)
            break;

        buffer[dwDownloaded] = '\0';
        outBody.append(buffer.data(), dwDownloaded);

    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return !outBody.empty();
}

bool HttpDownloadToFile(const std::wstring& host, const std::wstring& path, const std::string& destPath)
{
    HINTERNET hSession = WinHttpOpen(
        L"WraithUpdater/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpAddRequestHeaders(
        hRequest,
        L"X-Wraith-Client: 1\r\n",
        -1L,
        WINHTTP_ADDREQ_FLAG_ADD
    );

    BOOL bResult = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );
    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    HANDLE hFile = CreateFileA(
        destPath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD dwSize = 0;
    bool ok = true;

    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0)
            break;

        std::vector<char> buffer(dwSize);
        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded) || dwDownloaded == 0)
            break;

        DWORD dwWritten = 0;
        if (!WriteFile(hFile, buffer.data(), dwDownloaded, &dwWritten, nullptr) || dwWritten != dwDownloaded)
        {
            ok = false;
            break;
        }

    } while (dwSize > 0);

    CloseHandle(hFile);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ok;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    std::string gtaDir;
    if (!GetGtaDirectory(gtaDir))
    {
        MessageBoxA(
            nullptr,
            xorstr_("Could not locate GTA:SA via SA-MP registry.\nIs SA-MP installed correctly?"),
            xorstr_("Wraith-AC Updater"),
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    gtaDir = EnsureSlash(gtaDir);
    const std::string asiFileName = xorstr_("WRAITH-AC.asi");
    const std::string dllFileName = xorstr_("onnxruntime.dll");
    const std::string asiDest = gtaDir + asiFileName;
    const std::string dllDest = gtaDir + dllFileName;

    const std::string hostStr = xorstr_("anticheat.fsc-clan.eu");
    const std::string tokenPathStr = xorstr_("/get_token.php");
    const std::string baseUpdatePathStr = xorstr_("/wraith_update.php?token=");

    std::wstring host = ToWide(hostStr);
    std::wstring tokenPath = ToWide(tokenPathStr);

    std::string tokenResponse;
    if (!HttpGetBody(host, tokenPath, tokenResponse))
    {
        MessageBoxA(
            nullptr,
            xorstr_("Failed to contact update server (token)."),
            xorstr_("Wraith-AC Updater"),
            MB_ICONERROR | MB_OK
        );
        return 2;
    }

    TrimToken(tokenResponse);
    if (tokenResponse.empty())
    {
        MessageBoxA(
            nullptr,
            xorstr_("Update server returned an empty token."),
            xorstr_("Wraith-AC Updater"),
            MB_ICONERROR | MB_OK
        );
        return 3;
    }

    std::string fullUpdatePathStr = baseUpdatePathStr + tokenResponse;
    std::wstring fullUpdatePath = ToWide(fullUpdatePathStr);

    // Download ASI (default)
    if (!HttpDownloadToFile(host, fullUpdatePath, asiDest))
    {
        MessageBoxA(nullptr, xorstr_("Failed to download WRAITH-AC.asi"), xorstr_("Wraith-AC"), MB_ICONERROR | MB_OK);
        return 4;
    }

    // Download DLL with file parameter
    std::string dllUpdatePath = fullUpdatePathStr + xorstr_("&file=dll");
    std::wstring wDllUpdatePath = ToWide(dllUpdatePath);

    if (!HttpDownloadToFile(host, wDllUpdatePath, dllDest))
    {
        MessageBoxA(nullptr, xorstr_("Failed to download onnxruntime.dll"), xorstr_("Wraith-AC"), MB_ICONERROR | MB_OK);
        return 5;
    }

    std::string msg = xorstr_("Wraith-AC download completed. Launch SA:MP now.");
    MessageBoxA(
        nullptr,
        msg.c_str(),
        xorstr_("Wraith-AC Updater"),
        MB_ICONINFORMATION | MB_OK
    );

    return 0;
}
