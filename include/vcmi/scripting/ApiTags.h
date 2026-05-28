/*
 * ApiTags.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

class TagSerializable {};
class TagCopyable {};
class TagRawPointer {};
class TagSharedPointer {};

template<typename Derived>
class ApiTag
{
public:
    using ScriptingApiName = Derived;
};

/// Tag for classes that are accessible via scripting and support serialization to/from scripting values
/// Intended for use with POD types that consist solely from public fields, with no methods
/// Class must have `serializeScript(Handler h)` method that describes how to convert it into key-value form
/// Since this class is always represented as a table with its data members, it can't have any callable methods
/// New instances: Scripts can create of this class by constructing correctly structured table
/// Empty (null/nil) values are not permitted for this type
/// Lifetime: independent. Lua maintains own copy of the object
template<typename Derived>
class ApiSerializable : public ApiTag<Derived>, public TagSerializable
{};

/// Tag for simple classes that can be passed as a copy into scripting system
/// Script state will maintain own, single copy of this object
/// WARNING: avoid mutable (non-const) methods - prefer returning copy
/// Since script maintains single copy of the object, attempt to modify object in one table / variable will modify all of them
/// New instances: scripts can not create new instances of this class
/// Empty (null/nil) values are not permitted for this type
/// Lifetime: independent. Lua maintains own copy of the object
template<typename Derived>
class ApiCopyable : public ApiTag<Derived>, public TagCopyable
{};

/// Tag for classes that can be passed as a raw pointer into scripting system
/// New instances: scripts can not create new instances of this class
/// Empty (null/nil) values are permitted
/// Lifetime: managed by C++. WARNING: may result in garbage pointer if script keeps this pointer past the object lifetime!
template<typename Derived>
class ApiRawPointer : public ApiTag<Derived>, public TagRawPointer
{};

/// Tag for classes that can be passed as a shared pointer into scripting system
/// New instances: scripts can not create new instances of this class
/// Empty (null/nil) values are permitted
/// Lifetime: may be extended till scripting garbage collection
template<typename Derived>
class ApiSharedPointer : public ApiTag<Derived>, public TagSharedPointer
{};

}

VCMI_LIB_NAMESPACE_END
