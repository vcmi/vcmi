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
		std::shared_ptr<const ICacheableAsset> value;
		size_t bytes;
	};

	std::mutex mutex;
	std::list<Entry> order; // front = most recently used
	std::map<Key, std::list<Entry>::iterator> index;
	size_t usedBytes = 0;
	size_t budgetBytes = 0;

	/// Moves entries over budget into `evicted`, to be released by the caller
	void evict(std::vector<std::shared_ptr<const ICacheableAsset>> & evicted)
	{
		while(usedBytes > budgetBytes && !order.empty())
		{
			Entry & oldest = order.back();
			usedBytes -= oldest.bytes;
			evicted.push_back(std::move(oldest.value));
			index.erase(oldest.key);
			order.pop_back();
		}
	}

public:
	void setBudget(size_t bytes)
	{
		std::vector<std::shared_ptr<const ICacheableAsset>> evicted;
		{
			std::lock_guard lock(mutex);
			budgetBytes = bytes;
			evict(evicted);
		}
	}

	/// Retains value as most recently used. Its size is re-taken on every access, since
	/// assets grow lazily as scaled variants are generated.
	void store(const Key & key, const std::shared_ptr<const ICacheableAsset> & value)
	{
		if(!value)
			return;

		const size_t bytes = value->bytesUsed();

		// released after the lock: destroying an asset frees SDL surfaces, which should
		// not happen while other threads wait on the mutex
		std::vector<std::shared_ptr<const ICacheableAsset>> evicted;

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
			evict(evicted);
		}
	}

	void clear()
	{
		std::list<Entry> evicted;
		{
			std::lock_guard lock(mutex);
			evicted.swap(order);
			index.clear();
			usedBytes = 0;
		}
	}
};
