/*
 * WindowHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class IShowActivatable;

class WindowHandler
{
	/// list of interfaces. front = bottom-most (background), back = top-most (foreground)
	/// (includes adventure map, window interfaces, all kind of active dialogs, and so on)
	std::vector<std::shared_ptr<IShowActivatable>> windowsStack;

	/// Temporary list of recently popped windows
	std::vector<std::shared_ptr<IShowActivatable>> disposed;

public:
	/// forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void totalRedraw();

	/// update only top interface and draw background from buffer, sets a flag, method gets called at the end of the rendering
	void simpleRedraw();

	/// called whenever user selects different resolution, requiring to center/resize all windows
	void onScreenResize();

	/// deactivate old top interface, activates this one and pushes to the top
	void pushInt(std::shared_ptr<IShowActivatable> newInt);

	/// creates window of class T and pushes it to the top
	template <typename T, typename ... Args>
	void pushIntT(Args && ... args);

	/// pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front
	void popInts(int howMany);

	/// removes given interface from the top and activates next
	void popInt(std::shared_ptr<IShowActivatable> top);

	/// returns top interface
	std::shared_ptr<IShowActivatable> topInt() const;

	/// should be called after frame has been rendered to screen
	void onFrameRendered();

	/// returns current number of windows in the stack
	size_t count() const;

	/// erases all currently existing windows from the stacl
	void clear();

	/// returns all existing windows of selected type
	template <typename T>
	std::vector<std::shared_ptr<T>> findInts() const;
};

template <typename T, typename ... Args>
void WindowHandler::pushIntT(Args && ... args)
{
	auto newInt = std::make_shared<T>(std::forward<Args>(args)...);
	pushInt(newInt);
}

template <typename T>
std::vector<std::shared_ptr<T>> WindowHandler::findInts() const
{
	std::vector<std::shared_ptr<T>> result;

	for (auto const & window : windowsStack)
	{
		std::shared_ptr<T> casted = std::dynamic_pointer_cast<T>(window);

		if (casted)
			result.push_back(casted);
	}

	return result;
}
