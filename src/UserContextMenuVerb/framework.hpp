#pragma once

/***************************************************
 * WINDOWS
***************************************************/

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // bruh

#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <shobjidl_core.h>

#undef ShellExecute
#undef DragQueryFile
#undef OutputDebugString
#undef GetFileAttributes
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable
#undef ExpandEnvironmentStrings

/***************************************************
 * STD LIBRARY
***************************************************/

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
#include <experimental/generator>

template <typename T> using Vector = std::vector<T>;
template <typename T> using Optional = std::optional<T>;
template <typename T> using IList = std::initializer_list<T>;
template <typename T1, typename T2> using Pair = std::pair<T1, T2>;
template <typename T> using Generator = std::experimental::generator<T>;

using String = std::wstring;
using StrView = std::wstring_view;
using Path = std::filesystem::path;

#define NEW_NOTHROW(_) (new(std::nothrow) _)
#define STR_OPT_DATA(_) ((_) ? (_)->data() : nullptr)
#define STR_NOT_EMPTY_OR(_1, _2) ((_1).empty() ? (_2) : (_1))
#define STR_NULL_IF_EMPTY(_) ((_).empty() ? nullptr : (_).data())

/***************************************************
 * JSON LIBRARY
***************************************************/

#include "lib/json.hpp"

using Json = nlohmann::json;
using JsonArr = Json::array_t;
using JsonObj = Json::object_t;

inline Optional<std::string_view> JsonGetStr(const Json& json)
{
    if (json.is_string())
        return json.get_ref<const std::string&>();
    return std::nullopt;
}

#define JSON_GET_STR(_) JsonGetStr(_).value_or("") // view
#define JSON_GET_WSTR(_) MapStr(JSON_GET_STR(_))   // copy

/***************************************************
 * PROJECT
***************************************************/

#define PATH_MAX (UNICODE_STRING_MAX_CHARS - 1) // 32766

#define EXTERN extern "C"
#define EXPORT EXTERN HRESULT

using PPV = void**;
using RIID = const IID&;

struct DllInitObject
{
    BOOL isDarkTheme;
};

// main.cpp
extern ULONG g_count;
extern HMODULE g_hModule;
extern DllInitObject* g_pInitObj;

#include "lib/cef.hpp"
#include "lib/com.hpp"
#include "lib/util.hpp"

#include "Package.hpp"
#include "ClassFactory.hpp"

#include "Command.hpp"
#include "ContextMenu.hpp"
#include "ExplorerCommand.hpp"
#include "EnumExplorerCommand.hpp"
