﻿// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <file_utils.h>

#include <cstring>
#include <fstream>
#include <string>

#include "details/ie_exception.hpp"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

// for PATH_MAX
#ifndef _WIN32
# include <limits.h>
# include <unistd.h>
#endif

#include "details/ie_so_pointer.hpp"
#include "details/os/os_filesystem.hpp"

#if defined(WIN32) || defined(WIN64)
// Copied from linux libc sys/stat.h:
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

long long FileUtils::fileSize(const char* charfilepath) {
#if defined(ENABLE_UNICODE_PATH_SUPPORT) && defined(_WIN32)
    std::wstring widefilename = InferenceEngine::details::multiByteCharToWString(charfilepath);
    const wchar_t* fileName = widefilename.c_str();
#else
    const char* fileName = charfilepath;
#endif
    std::ifstream in(fileName, std::ios_base::binary | std::ios_base::ate);
    return in.tellg();
}

void FileUtils::readAllFile(const std::string& string_file_name, void* buffer, size_t maxSize) {
    std::ifstream inputFile;
    // Chromium libC++ only support char* type for file name.
    inputFile.open(string_file_name, std::ios::binary | std::ios::in);
    if (!inputFile.is_open()) THROW_IE_EXCEPTION << "cannot open file " << string_file_name;
    if (!inputFile.read(reinterpret_cast<char*>(buffer), maxSize)) {
        inputFile.close();
        THROW_IE_EXCEPTION << "cannot read " << maxSize << " bytes from file " << string_file_name;
    }

    inputFile.close();
}

namespace InferenceEngine {

namespace {

template <typename C, typename = InferenceEngine::details::enableIfSupportedChar<C> >
std::basic_string<C> getPathName(const std::basic_string<C>& s) {
    size_t i = s.rfind(FileUtils::FileTraits<C>::FileSeparator, s.length());
    if (i != std::string::npos) {
        return (s.substr(0, i));
    }

    return {};
}

}  // namespace

static std::string getIELibraryPathA() {
#ifdef _WIN32
    char ie_library_path[4096];
    HMODULE hm = NULL;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)getIELibraryPath, &hm)) {
        THROW_IE_EXCEPTION << "GetModuleHandle returned " << GetLastError();
    }
    GetModuleFileNameA(hm, (LPSTR)ie_library_path, sizeof(ie_library_path));
    return getPathName(std::string(ie_library_path));
#else
#ifdef USE_STATIC_IE
#ifdef __APPLE__
    Dl_info info;
    dladdr(reinterpret_cast<void*>(getIELibraryPath), &info);
    std::string path = getPathName(std::string(info.dli_fname)).c_str();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string path = getPathName(std::string(result, (count > 0) ? count : 0));
#endif  // __APPLE__
    return FileUtils::makePath(path, std::string( "lib"));
#else
    Dl_info info;
    dladdr(reinterpret_cast<void*>(getIELibraryPath), &info);
    return getPathName(std::string(info.dli_fname)).c_str();
#endif  // USE_STATIC_IE
#endif  // _WIN32
}

#ifdef ENABLE_UNICODE_PATH_SUPPORT

std::wstring getIELibraryPathW() {
#if defined(_WIN32) || defined(_WIN64)
    wchar_t ie_library_path[4096];
    HMODULE hm = NULL;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCWSTR)getIELibraryPath, &hm)) {
        THROW_IE_EXCEPTION << "GetModuleHandle returned " << GetLastError();
    }
    GetModuleFileNameW(hm, (LPWSTR)ie_library_path, sizeof(ie_library_path));
    return getPathName(std::wstring(ie_library_path));
#else
    return details::multiByteCharToWString(getIELibraryPathA().c_str());
#endif
}

#endif  // ENABLE_UNICODE_PATH_SUPPORT

std::string getIELibraryPath() {
#ifdef ENABLE_UNICODE_PATH_SUPPORT
    return details::wStringtoMBCSstringChar(getIELibraryPathW());
#else
    return getIELibraryPathA();
#endif
}

}  // namespace InferenceEngine
