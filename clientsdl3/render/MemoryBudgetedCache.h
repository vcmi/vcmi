/*
 * MemoryBudgetedCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "render/ICacheableAsset.h"
#include "render/ImageLocator.h"

/// Keeps recently used assets alive under a memory budget, so that an asset is not decoded
/// again right after its last user dropped it.
/// Reached from several threads: netpacks load assets, workers upscale them.
class MemoryBudgetedCache
{
public:
	/// Def files and images share one budget, so an asset is identified by either key
	using Key = std::variant<AnimationPath, SharedImageLocator>;

private:
	struct Entry
	{
		Key key;
		std::shared_ptr<ICacheableAsset> value;
		size_t bytes;
	};

	std::mutex mutex;
	std::list<Entry> order; // front = most recently used
	std::map<Key, std::list<Entry>::iterator> index;
	size_t usedBytes = 0;
	size_t budgetBytes = 0;

	/// Assets stored since the last re-measure. Walking every entry is not free, so it happens
	/// on a stride rather than on every store.
	size_t storesSinceRefresh = 0;
	static constexpr size_t storesBetweenRefreshes = 512;

	/// Assets this cache gave up. Held until the rendering thread reclaims them: another owner may
	/// still point at one, and only that thread may free what a draw could be reading.
	std::vector<std::shared_ptr<ICacheableAsset>> evictedAssets;

	/// Moves entries over budget out of the index and onto the reclaim list
	void evict()
	{
		while(usedBytes > budgetBytes && !order.empty())
		{
			Entry & oldest = order.back();
			usedBytes -= oldest.bytes;
			evictedAssets.push_back(std::move(oldest.value));
			index.erase(oldest.key);
			order.pop_back();
		}
	}

public:
	void setBudget(size_t bytes)
	{
		std::lock_guard lock(mutex);
		budgetBytes = bytes;
		evict();
	}

	/// Retains value as most recently used. Its size is re-taken here and, for entries nobody
	/// looks up again, by the periodic refresh - assets grow lazily as scaled variants are
	/// generated and textures uploaded.
	void store(const Key & key, const std::shared_ptr<ICacheableAsset> & value)
	{
		if(!value)
			return;

		const size_t bytes = value->bytesUsed();

		bool refreshDue = false;

		{
			std::lock_guard lock(mutex);

			auto it = index.find(key);
			if(it == index.end())
			{
				order.push_front(Entry{key, value, bytes});
				index[key] = order.begin();
			}
			else
			{
				usedBytes -= it->second->bytes;
				it->second->value = value;
				it->second->bytes = bytes;
				order.splice(order.begin(), order, it->second);
			}

			usedBytes += bytes;
			evict();

			refreshDue = ++storesSinceRefresh >= storesBetweenRefreshes;
			if(refreshDue)
				storesSinceRefresh = 0;
		}

		// outside the lock - it takes the same one
		if(refreshDue)
			refresh();
	}

	/// Re-measures every entry and evicts what no longer fits. An asset is stored right after it
	/// is loaded, long before its scaled variants exist and its texture has been uploaded, and
	/// nothing re-measures it while another owner keeps it from being looked up again - so
	/// without this the budget is compared against a fraction of what is really held.
	void refresh()
	{
		std::lock_guard lock(mutex);

		usedBytes = 0;
		for(Entry & entry : order)
		{
			entry.bytes = entry.value->bytesUsed();
			usedBytes += entry.bytes;
		}

		evict();
	}

	/// Hands over what was evicted since the last call, for the rendering thread to reclaim
	std::vector<std::shared_ptr<ICacheableAsset>> takeEvicted()
	{
		std::lock_guard lock(mutex);

		std::vector<std::shared_ptr<ICacheableAsset>> result;
		result.swap(evictedAssets);
		return result;
	}

	void clear()
	{
		std::list<Entry> dropped;
		{
			std::lock_guard lock(mutex);
			dropped.swap(order);
			index.clear();
			usedBytes = 0;
			evictedAssets.clear();
		}
	}
};
