/*
* InterfaceBuilder.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "CIntObject.h"

#include "../../lib/JsonNode.h"

class CPicture;
class CLabel;
class CToggleGroup;
class CToggleButton;
class CButton;
class CLabelGroup;
class CSlider;
class CAnimImage;
class CShowableAnim;
class CFilledTexture;

class InterfaceObjectConfigurable: public CIntObject
{
public:
	InterfaceObjectConfigurable(int used=0, Point offset=Point());
	InterfaceObjectConfigurable(const JsonNode & config, int used=0, Point offset=Point());

protected:
	//must be called after adding callbacks
	void init(const JsonNode & config);
	
	void addCallback(const std::string & callbackName, std::function<void(int)> callback);
	
	template<class T>
	const std::shared_ptr<T> widget(const std::string & name) const
	{
		auto iter = widgets.find(name);
		if(iter == widgets.end())
			return nullptr;
		return std::dynamic_pointer_cast<T>(iter->second);
	}
	
	const JsonNode & variable(const std::string & name) const;
	
	//basic serializers
	Point readPosition(const JsonNode &) const;
	Rect readRect(const JsonNode &) const;
	ETextAlignment readTextAlignment(const JsonNode &) const;
	SDL_Color readColor(const JsonNode &) const;
	EFonts readFont(const JsonNode &) const;
	std::string readText(const JsonNode &) const;
	std::pair<std::string, std::string> readHintText(const JsonNode &) const;
	
	//basic widgets
	std::shared_ptr<CPicture> buildPicture(const JsonNode &) const;
	std::shared_ptr<CLabel> buildLabel(const JsonNode &) const;
	std::shared_ptr<CToggleGroup> buildToggleGroup(const JsonNode &) const;
	std::shared_ptr<CToggleButton> buildToggleButton(const JsonNode &) const;
	std::shared_ptr<CButton> buildButton(const JsonNode &) const;
	std::shared_ptr<CLabelGroup> buildLabelGroup(const JsonNode &) const;
	std::shared_ptr<CSlider> buildSlider(const JsonNode &) const;
	std::shared_ptr<CAnimImage> buildImage(const JsonNode &) const;
	std::shared_ptr<CShowableAnim> buildAnimation(const JsonNode &) const;
	std::shared_ptr<CFilledTexture> buildTexture(const JsonNode &) const;
	
	
	//composite widgets
	virtual std::shared_ptr<CIntObject> buildCustomWidget(const JsonNode & config);
	std::shared_ptr<CIntObject> buildWidget(const JsonNode & config) const;
	
private:
	
	std::map<std::string, std::shared_ptr<CIntObject>> widgets;
	std::map<std::string, std::function<void(int)>> callbacks;
	JsonNode variables;
};
