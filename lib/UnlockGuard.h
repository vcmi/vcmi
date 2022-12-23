/*
 * UnlockGuard.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
	namespace detail
	{
		template<typename Mutex>
		class unlock_policy
		{
		protected:
			void unlock(Mutex &m)
			{
				m.unlock();
			}
			void lock(Mutex &m)
			{
				m.lock();
			}
		};

		template<typename Mutex>
		class unlock_shared_policy
		{
		protected:
			void unlock(Mutex &m)
			{
				m.unlock_shared();
			}
			void lock(Mutex &m)
			{
				m.lock_shared();
			}
		};
	}


	//similar to boost::lock_guard but UNlocks for the scope + assertions
	template<typename Mutex, typename LockingPolicy = detail::unlock_policy<Mutex> >
	class unlock_guard : LockingPolicy
	{
	private:
		Mutex* m;

		explicit unlock_guard(unlock_guard&);
		unlock_guard& operator=(unlock_guard&);
	public:
		explicit unlock_guard(Mutex& m_):
		m(&m_)
		{
			this->unlock(*m);
		}

		unlock_guard()
		{
			m = nullptr;
		}

		unlock_guard(unlock_guard &&other)
			: m(other.m)
		{
			other.m = nullptr;
		}

		void release()
		{
			m = nullptr;
		}

		~unlock_guard()
		{
			if(m)
				this->lock(*m);
		}
	};

	template<typename Mutex>
	unlock_guard<Mutex, detail::unlock_policy<Mutex> > makeUnlockGuard(Mutex &m_)
	{
		return unlock_guard<Mutex, detail::unlock_policy<Mutex> >(m_);
	}
	template<typename Mutex>
	unlock_guard<Mutex, detail::unlock_policy<Mutex> > makeEmptyGuard(Mutex &)
	{
		return unlock_guard<Mutex, detail::unlock_policy<Mutex> >();
	}
	template<typename Mutex>
	unlock_guard<Mutex, detail::unlock_policy<Mutex> > makeUnlockGuardIf(Mutex &m_, bool shallUnlock)
	{
		return shallUnlock 
			? makeUnlockGuard(m_)
			: unlock_guard<Mutex, detail::unlock_policy<Mutex> >();
	}
	template<typename Mutex>
	unlock_guard<Mutex, detail::unlock_shared_policy<Mutex> > makeUnlockSharedGuard(Mutex &m_)
	{
		return unlock_guard<Mutex, detail::unlock_shared_policy<Mutex> >(m_);
	}
	template<typename Mutex>
	unlock_guard<Mutex, detail::unlock_shared_policy<Mutex> > makeUnlockSharedGuardIf(Mutex &m_, bool shallUnlock)
	{
		return shallUnlock 
			? makeUnlockSharedGuard(m_)
			: unlock_guard<Mutex, detail::unlock_shared_policy<Mutex> >();
	}

	typedef unlock_guard<boost::shared_mutex, detail::unlock_shared_policy<boost::shared_mutex> > unlock_shared_guard;
}

VCMI_LIB_NAMESPACE_END
