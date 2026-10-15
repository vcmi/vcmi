/*
 * CDialogWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CLabel;
class CTextButton;
class CFilledTexture;
class TransparentFilledRectangle;

/// Window with a title, a content area and a row of buttons at the bottom
class CDialogWindow : public WindowBase
{
	static constexpr int TITLE_HEIGHT = 34;
	static constexpr int FOOTER_HEIGHT = 40;
	static constexpr int BUTTON_WIDTH = 90;
	static constexpr int BUTTON_HEIGHT = 26;

	std::shared_ptr<CFilledTexture> background;
	std::shared_ptr<TransparentFilledRectangle> frame;
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CIntObject> contentHolder;
	std::shared_ptr<CTextButton> acceptButton;
	std::shared_ptr<CTextButton> cancelButton;
	std::vector<std::shared_ptr<CTextButton>> extraButtons;

	void layoutButtons();

protected:
	/// Parent for the controls of the dialog. Their positions are relative to the content area
	CIntObject * content() const;

public:
	static constexpr int PADDING = 10;

	/// Called when the accept button is pressed. The dialog stays open if it returns false
	std::function<bool()> onAccept;
	/// Called after the dialog has been accepted and closed
	std::function<void()> afterAccept;
	/// Called when the dialog is closed without accepting
	std::function<void()> onCancel;

	/// size is the size of the whole window, shrunk to fit on the screen if needed
	CDialogWindow(const Point & size, const std::string & title, bool hasCancel = true);

	/// Size of the area available for the controls
	Point contentSize() const;

	void setTitle(const std::string & title);
	void setAcceptText(const std::string & text);
	void setCancelText(const std::string & text);
	void blockAccept(bool on);

	/// Adds a button to the left of the accept button. The dialog stays open when it is pressed
	std::shared_ptr<CTextButton> addButton(const std::string & text, const std::function<void()> & callback);

	/// Runs onAccept and closes the dialog on success
	void accept();
	/// Closes the dialog and runs onCancel
	void cancel();

	void keyPressed(EShortcut key) override;
};
