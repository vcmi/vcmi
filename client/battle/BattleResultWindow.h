/*
 * BattleResultWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/texts/MetaString.h"
#include "../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN
struct BattleResult;
VCMI_LIB_NAMESPACE_END

class CLabel;
class CButton;
class CAnimImage;
class VideoWidget;
class CPlayerInterface;
class CTextBox;

struct BattleResultResources
{
	VideoPath prologueVideo;
	VideoPath loopedVideo;
	AudioPath musicName;
	MetaString resultText;
};

/// Class which is responsible for showing the battle result window
class BattleResultWindow : public WindowBase
{
private:
	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::shared_ptr<CButton> exit;
	std::shared_ptr<CButton> repeat;
	std::vector<std::shared_ptr<CAnimImage>> icons;
	std::shared_ptr<CTextBox> description;
	std::shared_ptr<VideoWidget> videoPlayer;
	CPlayerInterface & owner;

	BattleResultResources getResources(const BattleResult & br);

	void buttonPressed(int button); //internal function for button callbacks
public:
	BattleResultWindow(const BattleResult & br, CPlayerInterface & _owner, bool allowReplay = false);

	void bExitf(); //exit button callback
	void bRepeatf(); //repeat button callback
	std::function<void(int result)> resultCallback; //callback receiving which button was pressed

	void activate() override;
};
