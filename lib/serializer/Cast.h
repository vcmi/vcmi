/*
 * Cast.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

template<class T, class F>
inline const T * dynamic_ptr_cast(const F * ptr)
{
	return dynamic_cast<const T *>(ptr);
}

template<class T, class F>
inline T * dynamic_ptr_cast(F * ptr)
{
	return dynamic_cast<T *>(ptr);
}

VCMI_LIB_NAMESPACE_END
