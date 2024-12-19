/*
 * Serializeable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

// Tag class that acts as base for all classes that can be serialized by pointer
class Serializeable
{
public:
    virtual ~Serializeable() = default;
};

VCMI_LIB_NAMESPACE_END
