/*
 * ICacheableAsset.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Asset that can estimate how much memory it holds, so that a cache can budget it
class ICacheableAsset
{
public:
	virtual size_t bytesUsed() const = 0;

	virtual ~ICacheableAsset() = default;
};
