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

	/// Frees whatever the asset can build again on demand. Called when a cache drops the asset
	/// while another owner still holds it - without this, evicting only releases a pointer.
	/// Rendering thread only.
	virtual void releaseMemory() {}

	virtual ~ICacheableAsset() = default;
};
