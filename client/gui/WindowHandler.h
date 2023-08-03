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
	/// list of windows. front = bottom-most (background), back = top-most (foreground)
	/// (includes adventure map, window windows, all kind of active dialogs, and so on)
	std::vector<std::shared_ptr<IShowActivatable>> windowsStack;

	/// Temporary list of recently popped windows
	std::vector<std::shared_ptr<IShowActivatable>> disposed;

	bool totalRedrawRequested = false;

	/// returns top windows
	std::shared_ptr<IShowActivatable> topWindowImpl() const;

	/// forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void totalRedrawImpl();

	/// update only top windows and draw background from buffer, sets a flag, method gets called at the end of the rendering
	void simpleRedrawImpl();

public:
	/// forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void totalRedraw();

	/// update only top windows and draw background from buffer, sets a flag, method gets called at the end of the rendering
	void simpleRedraw();

	/// called whenever user selects different resolution, requiring to center/resize all windows
	void onScreenResize();

	/// deactivate old top windows, activates this one and pushes to the top
	void pushWindow(std::shared_ptr<IShowActivatable> newInt);

	/// creates window of class T and pushes it to the top
	template <typename T, typename ... Args>
	void createAndPushWindow(Args && ... args);

	/// pops one or more windows - deactivates top, deletes and removes given number of windows, activates new front
	void popWindows(int howMany);

	/// returns true if current top window is a right-click popup
	bool isTopWindowPopup() const;

	/// removes given windows from the top and activates next
	void popWindow(std::shared_ptr<IShowActivatable> top);

	/// returns true if selected interface is on top
	bool isTopWindow(std::shared_ptr<IShowActivatable> window) const;
	bool isTopWindow(IShowActivatable * window) const;

	/// returns top window if it matches requested class
	template <typename T>
	std::shared_ptr<T> topWindow() const;

	/// should be called after frame has been rendered to screen
	void onFrameRendered();

	/// returns current number of windows in the stack
	size_t count() const;

	/// erases all currently existing windows from the stack
	void clear();

	/// returns all existing windows of selected type
	template <typename T>
	std::vector<std::shared_ptr<T>> findWindows() const;
};

template <typename T, typename ... Args>
void WindowHandler::createAndPushWindow(Args && ... args)
{
	auto newWindow = std::make_shared<T>(std::forward<Args>(args)...);
	pushWindow(newWindow);
}

template <typename T>
std::vector<std::shared_ptr<T>> WindowHandler::findWindows() const
{
	std::vector<std::shared_ptr<T>> result;

	for(const auto & window : windowsStack)
	{
		std::shared_ptr<T> casted = std::dynamic_pointer_cast<T>(window);

		if (casted)
			result.push_back(casted);
	}
	return result;
}

template <typename T>
std::shared_ptr<T> WindowHandler::topWindow() const
{
	return std::dynamic_pointer_cast<T>(topWindowImpl());
}
