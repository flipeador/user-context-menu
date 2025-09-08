#pragma once

String FindFilePath(StrView str, StrView defs = L"");

enum class CommandType : uint32_t
{
    // File | Directory
    File      = 0x00000001, // File
    Drive     = 0x00000002, // File
    Directory = 0x00000004, // Directory

    // Background
    Unknown    = 0x00000008, // (This PC | Quick Access)
    Desktop    = 0x00000010,
    Background = 0x00000020,
};

struct Command
{
    Json json;
    BOOL isDesktop{ };
    String background;
    Vector<Pair<String, DWORD>> items;

    HRESULT Invoke(HWND hWnd, const Json& json);
};
