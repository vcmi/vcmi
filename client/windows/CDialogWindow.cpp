/*
 * CDialogWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CDialogWindow.h"

#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CTextButton.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"
#include "render/Colors.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

CDialogWindow::CDialogWindow(const Point & size, const std::string & title, bool hasCancel)
	: WindowBase(KEYBOARD)
{
	OBJECT_CONSTRUCTION;

	const Point screen = ENGINE->screenDimensions();
	pos.w = std::min(size.x, screen.x - 2 * PADDING);
	pos.h = std::min(size.y, screen.y - 2 * PADDING);

	auto blueBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	blueBackground->setPlayerColor(PlayerColor(1));
	background = blueBackground;

	frame = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 40), UiTheme::BORDER, 2);

	titleLabel = std::make_shared<CLabel>(pos.w / 2, TITLE_HEIGHT / 2, UiTheme::FONT_TITLE, ETextAlignment::CENTER, UiTheme::TEXT_TITLE, title);

	contentHolder = std::make_shared<CIntObject>();
	contentHolder->pos = Rect(pos.x + PADDING, pos.y + TITLE_HEIGHT, pos.w - 2 * PADDING, pos.h - TITLE_HEIGHT - FOOTER_HEIGHT);

	acceptButton = std::make_shared<CTextButton>(Rect(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT), LIBRARY->generaltexth->translate("vcmi.ui.ok"), [this]() { accept(); });
	if(hasCancel)
		cancelButton = std::make_shared<CTextButton>(Rect(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT), LIBRARY->generaltexth->translate("vcmi.ui.cancel"), [this]() { cancel(); });

	layoutButtons();
	center();
}

void CDialogWindow::layoutButtons()
{
	const int y = pos.y + pos.h - FOOTER_HEIGHT + (FOOTER_HEIGHT - BUTTON_HEIGHT) / 2;
	int x = pos.x + pos.w - PADDING - BUTTON_WIDTH;

	acceptButton->moveTo(Point(x, y));
	if(cancelButton)
	{
		cancelButton->moveTo(Point(x, y));
		acceptButton->moveTo(Point(x - BUTTON_WIDTH - UiTheme::SPACING, y));
		x -= BUTTON_WIDTH + UiTheme::SPACING;
	}

	for(auto i = extraButtons.rbegin(); i != extraButtons.rend(); ++i)
	{
		x -= (*i)->pos.w + UiTheme::SPACING;
		(*i)->moveTo(Point(x, y));
	}
}

CIntObject * CDialogWindow::content() const
{
	return contentHolder.get();
}

Point CDialogWindow::contentSize() const
{
	return contentHolder->pos.dimensions();
}

void CDialogWindow::setTitle(const std::string & title)
{
	titleLabel->setText(title);
}

void CDialogWindow::setAcceptText(const std::string & text)
{
	acceptButton->setText(text);
}

void CDialogWindow::setCancelText(const std::string & text)
{
	if(cancelButton)
		cancelButton->setText(text);
}

void CDialogWindow::blockAccept(bool on)
{
	acceptButton->block(on);
}

std::shared_ptr<CTextButton> CDialogWindow::addButton(const std::string & text, const std::function<void()> & callback)
{
	OBJECT_CONSTRUCTION;

	auto button = std::make_shared<CTextButton>(Rect(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT), text, callback);
	extraButtons.push_back(button);
	layoutButtons();
	return button;
}

void CDialogWindow::accept()
{
	if(onAccept && !onAccept())
		return;

	auto callback = afterAccept;
	close();

	if(callback)
		callback();
}

void CDialogWindow::cancel()
{
	auto callback = onCancel;
	close();

	if(callback)
		callback();
}

void CDialogWindow::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_ACCEPT && !acceptButton->isBlocked())
		accept();
	else if(key == EShortcut::GLOBAL_CANCEL)
		cancel();
}
