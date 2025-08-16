#pragma once

/***************************************************
 * WINDOWS
***************************************************/

#pragma comment(lib, "pathcch.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // bruh

#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <shobjidl_core.h>

#undef ShellExecute
#undef GetFullPathName
#undef OutputDebugString
#undef GetModuleFileName
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable
#undef ExpandEnvironmentStrings

#define LONG_MAXPATH UNICODE_STRING_MAX_CHARS // 32767

/***************************************************
 * STD LIBRARY
***************************************************/

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

#include <new>
#include <regex>
#include <string>
#include <format>
#include <ranges>
#include <utility>
#include <fstream>
#include <iostream>
#include <iterator>
#include <concepts>
#include <filesystem>
#include <type_traits>
#include <experimental/generator>

namespace std {
    template <typename T>
    using generator = experimental::generator<T>;
}

using String = std::wstring;
using StrView = std::wstring_view;
using Path = std::filesystem::path;

#define NEW_NOTHROW(_) (new(std::nothrow) _)

#define STR_EMPTY(_1, _2) (_1.empty() ? (_2) : (_1))
#define STR_NULL_IF_EMPTY(_) ((_).empty() ? nullptr : (_).data())

/***************************************************
 * JSON LIBRARY
***************************************************/

#include "lib/json.hpp"

using Json = nlohmann::json;

using JsonArr = Json::array_t;
using JsonObj = Json::object_t;
using JsonStr = Json::string_t;

/***************************************************
 * PROJECT
***************************************************/

#pragma warning(disable : 4456) // declaration of 'X' hides previous local declaration

using PPV = void**;

// main.cpp
extern ULONG g_count;
extern HMODULE g_hModule;

#include "lib/cef.hpp"
#include "lib/com.hpp"
#include "lib/util.hpp"

#include "Package.hpp"
#include "ClassFactory.hpp"
#include "ExplorerCommand.hpp"
#include "EnumExplorerCommand.hpp"
