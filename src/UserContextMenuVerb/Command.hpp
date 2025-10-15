#pragma once

String FindFilePath(StrView str, StrView defs = L"");
VOID SetCommandEnvVarsPath(const Path& path);
Pair<String,INT> GetCommandIcon(const Json& json);

enum class CommandType : uint32_t
{
    // File | Directory
    File      = 0x00000001,
    Drive     = 0x00000002,
    Directory = 0x00000004,

    // Background
    Unknown    = 0x00000008, // This PC | Quick Access
    Desktop    = 0x00000010, // CLSID_ShellDesktop
    Background = 0x00000020, // Current Folder
};

struct Command
{
    Json json;
    BOOL isDesktop{ };
    String background;
    Vector<Pair<String,DWORD>> items;

    HRESULT Invoke(HWND hWnd, const Json& json);
};
