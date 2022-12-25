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

#define REGISTER_BUILDER(type, method) registerBuilder(type, std::bind(method, this, std::placeholders::_1))

class InterfaceObjectConfigurable: public CIntObject
{
public:
	InterfaceObjectConfigurable(int used=0, Point offset=Point());
	InterfaceObjectConfigurable(const JsonNode & config, int used=0, Point offset=Point());

protected:
	
	using BuilderFunction = std::function<std::shared_ptr<CIntObject>(const JsonNode &)>;
	void registerBuilder(const std::string &, BuilderFunction);
	
	//must be called after adding callbacks
	void build(const JsonNode & config);
	
	void addCallback(const std::string & callbackName, std::function<void(int)> callback);
	JsonNode variables;
	
	template<class T>
	const std::shared_ptr<T> widget(const std::string & name) const
	{
		auto iter = widgets.find(name);
		if(iter == widgets.end())
			return nullptr;
		return std::dynamic_pointer_cast<T>(iter->second);
	}
	
	void deleteWidget(const std::string & name);
		
	//basic serializers
	Point readPosition(const JsonNode &) const;
	Rect readRect(const JsonNode &) const;
	ETextAlignment readTextAlignment(const JsonNode &) const;
	SDL_Color readColor(const JsonNode &) const;
	EFonts readFont(const JsonNode &) const;
	std::string readText(const JsonNode &) const;
	std::pair<std::string, std::string> readHintText(const JsonNode &) const;
	int readKeycode(const JsonNode &) const;
	
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
	std::shared_ptr<CIntObject> buildWidget(JsonNode config) const;
	
private:
	
	int unnamedObjectId = 0;
	std::map<std::string, BuilderFunction> builders;
	std::map<std::string, std::shared_ptr<CIntObject>> widgets;
	std::map<std::string, std::function<void(int)>> callbacks;
};
