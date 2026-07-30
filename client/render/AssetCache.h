/*
 * AssetCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Memory-budgeted LRU cache that keeps recently used assets alive.
///
/// RenderHandler tracks loaded .def files and images in weak_ptr maps. Those maps
/// only find an asset while somebody else still holds it - the moment the last
/// user drops it the asset is destroyed, and the next request re-reads the file
/// from disk, re-decodes it and re-runs the xBRZ upscaler. Profiling an ordinary
/// session showed this happening constantly for animated terrain: LAVATL.def was
/// read from disk 774 times in 46 seconds, and 82% of all .def reads were re-reads
/// of a file that had already been parsed.
///
/// This cache holds an *additional* strong reference to the most recently used
/// assets so they survive until the memory budget pushes them out. It deliberately
/// does not change lookup semantics: the weak_ptr maps stay authoritative and this
/// class only stops entries from expiring too eagerly. Evicting an entry that is
/// still in use elsewhere is harmless - it simply becomes weakly-tracked again.
///
/// Entry cost is an estimate in bytes, refreshed on every access, since assets
/// (notably ScalableImageShared) grow lazily as scaled variants are generated.
template<typename Key, typename Value>
class MemoryBudgetedCache
{
public:
	explicit MemoryBudgetedCache(size_t budgetBytes)
		: budgetBytes(budgetBytes)
	{
	}

	/// Retains `value` under `key` and marks it as most recently used.
	/// `costBytes` is the current estimated memory footprint of the asset.
	void retain(const Key & key, const std::shared_ptr<Value> & value, size_t costBytes)
	{
		if(!value)
			return;

		// Anything evicted is released after the lock is dropped: destroying an
		// asset frees SDL surfaces, which is not work we want to do under a lock
		// other threads are waiting on.
		std::vector<std::shared_ptr<Value>> evicted;

		{
			std::lock_guard lock(mutex);

			auto indexIter = index.find(key);
			if(indexIter != index.end())
			{
				auto entryIter = indexIter->second;
				usedBytes -= entryIter->costBytes;
				entryIter->value = value;
				entryIter->costBytes = costBytes;
				usedBytes += costBytes;
				order.splice(order.begin(), order, entryIter);
			}
			else
			{
				order.push_front(Entry{key, value, costBytes});
				index[key] = order.begin();
				usedBytes += costBytes;
			}

			evictLocked(evicted);
		}
	}

	/// Marks an existing entry as most recently used and refreshes its cost.
	/// Returns false if the key is not retained by this cache.
	bool touch(const Key & key, size_t costBytes)
	{
		std::vector<std::shared_ptr<Value>> evicted;

		{
			std::lock_guard lock(mutex);

			auto indexIter = index.find(key);
			if(indexIter == index.end())
				return false;

			auto entryIter = indexIter->second;
			usedBytes -= entryIter->costBytes;
			entryIter->costBytes = costBytes;
			usedBytes += costBytes;
			order.splice(order.begin(), order, entryIter);

			evictLocked(evicted);
		}
		return true;
	}

	void remove(const Key & key)
	{
		std::shared_ptr<Value> evicted;

		{
			std::lock_guard lock(mutex);

			auto indexIter = index.find(key);
			if(indexIter == index.end())
				return;

			evicted = indexIter->second->value;
			usedBytes -= indexIter->second->costBytes;
			order.erase(indexIter->second);
			index.erase(indexIter);
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

	void setBudget(size_t bytes)
	{
		std::vector<std::shared_ptr<Value>> evicted;

		{
			std::lock_guard lock(mutex);
			budgetBytes = bytes;
			evictLocked(evicted);
		}
	}

	size_t getBudget() const
	{
		std::lock_guard lock(mutex);
		return budgetBytes;
	}

	size_t getUsedBytes() const
	{
		std::lock_guard lock(mutex);
		return usedBytes;
	}

	size_t getEntryCount() const
	{
		std::lock_guard lock(mutex);
		return index.size();
	}

private:
	struct Entry
	{
		Key key;
		std::shared_ptr<Value> value;
		size_t costBytes;
	};

	/// front() is the most recently used entry
	std::list<Entry> order;
	std::map<Key, typename std::list<Entry>::iterator> index;

	mutable std::mutex mutex;
	size_t budgetBytes;
	size_t usedBytes = 0;

	/// Drops least recently used entries until the budget is met. Must be called
	/// with the lock held; ownership of dropped values is moved to `evicted` so
	/// the caller can destroy them outside the lock.
	void evictLocked(std::vector<std::shared_ptr<Value>> & evicted)
	{
		while(usedBytes > budgetBytes && !order.empty())
		{
			// Never evict the entry that was just inserted - a single asset larger
			// than the whole budget would otherwise thrash on every access.
			if(order.size() == 1)
				break;

			Entry & victim = order.back();
			usedBytes -= victim.costBytes;
			evicted.push_back(std::move(victim.value));
			index.erase(victim.key);
			order.pop_back();
		}
	}
};

namespace AssetCache
{
	/// Total physical RAM in bytes, or 0 when it cannot be determined.
	size_t getTotalSystemMemory();

	/// Memory budget for the asset retention caches.
	/// `configuredMegabytes` comes from settings; 0 means "derive from system RAM".
	size_t getRetentionBudget(int configuredMegabytes);
}
