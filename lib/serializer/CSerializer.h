/*
 * CSerializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

const std::string SAVEGAME_MAGIC = "VCMISVG";

/// Helper to detect classes with user-provided serialize(S&, int version) method
template<class S, class T>
struct is_serializeable
{
	using Yes = char (&)[1];
	using No = char (&)[2];

	template<class U>
	static Yes test(U * data, S* arg1 = nullptr, typename std::enable_if_t<std::is_void_v<decltype(data->serialize(*arg1))>> * = nullptr);
	static No test(...);
	static const bool value = sizeof(Yes) == sizeof(is_serializeable::test((typename std::remove_reference_t<typename std::remove_cv_t<T>>*)nullptr));
};

/// Base class for deserializers
class IBinaryReader
{
public:
	virtual ~IBinaryReader() = default;
	virtual int read(std::byte * data, unsigned size) = 0;
};

/// Base class for serializers
class IBinaryWriter
{
public:
	virtual ~IBinaryWriter() = default;
	virtual int write(const std::byte * data, unsigned size) = 0;
};

VCMI_LIB_NAMESPACE_END
