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

class InterfaceBuilder: public CIntObject
{
public:
	InterfaceBuilder();
	InterfaceBuilder(const JsonNode & config);

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
		return dynamic_pointer_cast<T>(iter->second);
	}
	
private:
	
	std::map<std::string, std::shared_ptr<CIntObject>> widgets;
	std::map<std::string, std::function<void(int)>> callbacks;
	
	std::shared_ptr<CIntObject> buildWidget(const JsonNode & config);
	
	std::string buildText(const JsonNode & param) const;
};
