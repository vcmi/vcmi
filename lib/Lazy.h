/*
 * Lazy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <functional>
#include <memory>

VCMI_LIB_NAMESPACE_BEGIN

/// Lazily-initialized value. Holds either a factory that produces the value on
/// first access, or an already-materialized value. There is no "partially
/// initialized" state: once accessed, the value is complete.
template <typename T>
class Lazy
{
public:
	using Factory = std::function<std::unique_ptr<T>()>;

	Lazy() = default;

	explicit Lazy(Factory factory)
		: factory(std::move(factory))
	{
	}

	explicit Lazy(std::unique_ptr<T> value)
		: value(std::move(value))
	{
	}

	Lazy & operator=(std::unique_ptr<T> v)
	{
		value = std::move(v);
		factory = nullptr;
		return *this;
	}

	Lazy & operator=(Factory f)
	{
		factory = std::move(f);
		value.reset();
		return *this;
	}

	T * operator->() const
	{
		ensure();
		return value.get();
	}

	T & operator*() const
	{
		ensure();
		return *value;
	}

	T * get() const
	{
		ensure();
		return value.get();
	}

	/// true if a value is present or can be produced on demand
	explicit operator bool() const
	{
		return value != nullptr || factory != nullptr;
	}

	bool isInitialized() const
	{
		return value != nullptr;
	}

	void reset()
	{
		value.reset();
		factory = nullptr;
	}

	template <typename Handler>
	void serialize(Handler & h)
	{
		ensure();
		h & value;
	}

private:
	void ensure() const
	{
		if(!value && factory)
			value = factory();
	}

	Factory factory;
	mutable std::unique_ptr<T> value;
};

VCMI_LIB_NAMESPACE_END
