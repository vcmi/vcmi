/*
 * Registry.cpp, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */

#include "StdInc.h"
#include "Registry.h"

#include "../../ScopeGuard.h"

#include <windows.h>

namespace
{
std::optional<std::wstring> readString(const std::wstring & subKey, const std::wstring & valueName, REGSAM registryView)
{
	HKEY registryKey = nullptr;
	if(RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_QUERY_VALUE | registryView, &registryKey) != ERROR_SUCCESS)
		return std::nullopt;
	auto closeRegistryKey = vstd::makeScopeGuard([registryKey]() { RegCloseKey(registryKey); });

	DWORD type = 0;
	DWORD size = 0;
	constexpr DWORD acceptedTypes = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND;
	const LSTATUS sizeResult = RegGetValueW(registryKey, nullptr, valueName.c_str(), acceptedTypes, &type, nullptr, &size);
	if(sizeResult != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || size < sizeof(wchar_t))
		return std::nullopt;

	std::wstring value(size / sizeof(wchar_t), L'\0');
	const LSTATUS valueResult = RegGetValueW(registryKey, nullptr, valueName.c_str(), acceptedTypes, &type, value.data(), &size);
	if(valueResult != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || size % sizeof(wchar_t) != 0)
		return std::nullopt;

	while(!value.empty() && value.back() == L'\0')
		value.pop_back();
	if(value.empty())
		return std::nullopt;

	if(type == REG_EXPAND_SZ)
	{
		const DWORD expandedSize = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
		if(expandedSize > 0)
		{
			std::wstring expanded(expandedSize, L'\0');
			if(ExpandEnvironmentStringsW(value.c_str(), expanded.data(), expandedSize) == expandedSize)
			{
				expanded.resize(expandedSize - 1);
				return expanded;
			}
		}
	}

	return value;
}
}

std::optional<std::wstring> readCurrentUserRegistryString(const std::wstring & subKey, const std::wstring & valueName)
{
	if(const auto value = readString(subKey, valueName, KEY_WOW64_64KEY))
		return value;
	return readString(subKey, valueName, KEY_WOW64_32KEY);
}

bool writeCurrentUserRegistryString(const std::wstring & subKey, const std::wstring & valueName, const std::wstring & value)
{
	HKEY registryKey = nullptr;
	if(RegCreateKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &registryKey, nullptr) != ERROR_SUCCESS
		&& RegCreateKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_32KEY, nullptr, &registryKey, nullptr) != ERROR_SUCCESS)
		return false;
	auto closeRegistryKey = vstd::makeScopeGuard([registryKey]() { RegCloseKey(registryKey); });

	const auto valueSize = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
	return RegSetKeyValueW(registryKey, nullptr, valueName.c_str(), REG_SZ, value.c_str(), valueSize) == ERROR_SUCCESS;
}

void removeCurrentUserRegistryValue(const std::wstring & subKey, const std::wstring & valueName)
{
	for(const REGSAM registryView : { KEY_WOW64_64KEY, KEY_WOW64_32KEY })
	{
		HKEY registryKey = nullptr;
		if(RegOpenKeyExW(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_SET_VALUE | registryView, &registryKey) == ERROR_SUCCESS)
		{
			RegDeleteValueW(registryKey, valueName.c_str());
			RegCloseKey(registryKey);
		}
	}
}
