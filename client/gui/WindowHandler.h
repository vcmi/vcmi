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
public:
	/// list of interfaces - front=foreground; back = background (includes adventure map, window interfaces, all kind of active dialogs, and so on)
	std::list<std::shared_ptr<IShowActivatable>> listInt;

	std::vector<std::shared_ptr<IShowActivatable>> disposed;

	// objs to blit
	std::vector<std::shared_ptr<IShowActivatable>> objsToBlit;

	/// forces total redraw (using showAll), sets a flag, method gets called at the end of the rendering
	void totalRedraw();

	/// update only top interface and draw background from buffer, sets a flag, method gets called at the end of the rendering
	void simpleRedraw();

	/// called whenever user selects different resolution, requiring to center/resize all windows
	void onScreenResize();

	/// deactivate old top interface, activates this one and pushes to the top
	void pushInt(std::shared_ptr<IShowActivatable> newInt);
	template <typename T, typename ... Args>
	void pushIntT(Args && ... args)
	{
		auto newInt = std::make_shared<T>(std::forward<Args>(args)...);
		pushInt(newInt);
	}

	/// pops one or more interfaces - deactivates top, deletes and removes given number of interfaces, activates new front
	void popInts(int howMany);

	/// removes given interface from the top and activates next
	void popInt(std::shared_ptr<IShowActivatable> top);

	/// returns top interface
	std::shared_ptr<IShowActivatable> topInt();


	void onFrameRendered();
};
