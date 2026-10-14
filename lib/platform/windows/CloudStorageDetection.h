/*
 * CloudStorageDetection.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include <windows.h>

#include <cwchar>
#include <string>

namespace VCMI::Windows
{
inline bool isEnvironmentPathPrefix(const wchar_t * path, const wchar_t * variableName)
{
	const DWORD requiredSize = GetEnvironmentVariableW(variableName, nullptr, 0);
	if(requiredSize == 0)
		return false;

	std::wstring expanded(requiredSize, L'\0');
	const DWORD pathLength = GetEnvironmentVariableW(variableName, expanded.data(), requiredSize);
	if(pathLength == 0 || pathLength >= requiredSize)
		return false;
	expanded.resize(pathLength);
	while(expanded.size() > 3 && (expanded.back() == L'\\' || expanded.back() == L'/'))
		expanded.pop_back();

	const size_t environmentPathLength = expanded.size();
	if(std::wcslen(path) < environmentPathLength)
		return false;

	return _wcsnicmp(path, expanded.c_str(), environmentPathLength) == 0
		&& (path[environmentPathLength] == L'\0' || path[environmentPathLength] == L'\\' || path[environmentPathLength] == L'/');
}

inline bool isOneDrivePath(const wchar_t * path)
{
	if(!path || !*path)
		return false;

	// OneDrive exposes different variables for personal and organization accounts.
	return isEnvironmentPathPrefix(path, L"OneDrive")
		|| isEnvironmentPathPrefix(path, L"OneDriveConsumer")
		|| isEnvironmentPathPrefix(path, L"OneDriveCommercial");
}

inline bool isCloudStoragePath(const wchar_t * path)
{
	if(!path || !*path)
		return false;
	if(isOneDrivePath(path))
		return true;

	// The Cloud Files API covers all registered sync providers, but is unavailable on old Windows.
	// Loading it dynamically keeps this code usable on Windows 7 and later.
	using GetSyncRootInfoByPath = HRESULT(WINAPI *)(LPCWSTR, int, PVOID, DWORD, DWORD *);
	static const HMODULE cloudApi = []
	{
		wchar_t systemDirectory[MAX_PATH];
		const UINT length = GetSystemDirectoryW(systemDirectory, MAX_PATH);
		return length > 0 && length < MAX_PATH
			? LoadLibraryW((std::wstring(systemDirectory, length) + L"\\CldApi.dll").c_str())
			: nullptr;
	}();
	static const auto getSyncRootInfoByPath = cloudApi
		? reinterpret_cast<GetSyncRootInfoByPath>(GetProcAddress(cloudApi, "CfGetSyncRootInfoByPath"))
		: nullptr;
	if(!getSyncRootInfoByPath)
		return false;

	std::wstring candidate(path);
	while(candidate.size() > 3 && (candidate.back() == L'\\' || candidate.back() == L'/'))
		candidate.pop_back();
	constexpr int basicSyncRootInfoClass = 0;
	while(!candidate.empty())
	{
		LARGE_INTEGER syncRootFileId{};
		if(SUCCEEDED(getSyncRootInfoByPath(candidate.c_str(), basicSyncRootInfoClass, &syncRootFileId, sizeof(syncRootFileId), nullptr)))
			return true;

		const size_t separator = candidate.find_last_of(L"\\/");
		if(separator == std::wstring::npos || separator < 3)
			break;
		candidate.resize(separator);
	}
	return false;
}
}
