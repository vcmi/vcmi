/*
 * ExceptionsCommon.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class DLL_LINKAGE DataLoadingException: public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};
