/*
 * CIntObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GuiJsonLoader.h"

#include "CGuiHandler.h"
#include "SDL_Extensions.h"
#include "../CMessage.h"
#include "../widgets/Buttons.h"
#include "../../lib/CModHandler.h"

class UiContainer : public virtual CIntObject
{
private:
	std::vector<std::shared_ptr<CIntObject>> childrenSmart;

public:
	void addChild(std::shared_ptr<CIntObject> obj)
	{
		childrenSmart.push_back(obj);
		//addChild(*obj);
	}
};

UiDataBinder::UiDataBinder(IUiController & controller)
	:handlers(controller.getHandlers())
{
}

std::function<void()> UiDataBinder::bindHandler(JsonNode & handlerNode)
{
	const std::string controller = "@controller.";

	auto handlerExpression = handlerNode.String();

	if(handlerExpression.find(controller) == 0)
	{
		handlerExpression = handlerExpression.substr(controller.length());
		auto argsPos = handlerExpression.find('(');
		THandlerArgs args;

		if(argsPos > 0)
		{
			auto argsStr = handlerExpression.substr(argsPos + 1, handlerExpression.length() - argsPos - 1);

			boost::split(args, argsStr, [](char c) { return c == ' ' || c == ','; });

			handlerExpression = handlerExpression.substr(0, argsPos);
		}

		if(vstd::contains(handlers, handlerExpression))
		{
			auto handler = handlers[handlerExpression];

			return std::bind(handler, args);
		}
	}

	logGlobal->error("Malformed handler expression: " + handlerNode.String());
	return []() {};
}

Rect IControlFactory::getControlRect(Rect viewport, Point contentSize, JsonNode & node)
{
	Rect result;

	if(contentSize.x == 0) contentSize.x = viewport.w;
	if(contentSize.y == 0) contentSize.y = viewport.h;

	if(!node["width"].isNull()) contentSize.x = node["width"].Integer();
	if(!node["height"].isNull()) contentSize.y = node["height"].Integer();

	if(!node["left"].isNull())
		result.x = viewport.x + node["left"].Integer();
	else if(!node["right"].isNull())
		result.x = viewport.x + viewport.w - node["right"].Integer() - contentSize.x;
	else
		result.x = viewport.x + (viewport.w - contentSize.x) / 2;

	if(!node["top"].isNull())
		result.y = viewport.y + node["top"].Integer();
	else if(!node["bottom"].isNull())
		result.y = viewport.y + viewport.h - node["bottom"].Integer() - contentSize.y;
	else
		result.y = viewport.y + (viewport.h - contentSize.y) / 2;

	result.w = contentSize.x;
	result.h = contentSize.y;

	return result;
}

class ContainerFactory : public IControlFactory
{
private:
	IControlFactory * parent;

public:
	ContainerFactory(IControlFactory * parent) : parent(parent) {}

	virtual std::shared_ptr<CIntObject> createControl(Rect viewport, JsonNode & node, UiDataBinder & dataBinder) const
	{
		std::shared_ptr<UiContainer> container = std::make_shared<UiContainer>();

		for(auto child : node["items"].Vector())
		{
			container->addChild(parent->createControl(viewport, child, dataBinder));
		}

		return container;
	}
};

class GridFactory : public IControlFactory
{
private:
	IControlFactory * parent;

public:
	GridFactory(IControlFactory * parent) : parent(parent) {}

	virtual std::shared_ptr<CIntObject> createControl(Rect viewport, JsonNode & node, UiDataBinder & dataBinder) const
	{
		std::shared_ptr<UiContainer> container = std::make_shared<UiContainer>();
		Rect controlRect = getControlRect(viewport, Point(0, 0), node);

		int columns = 1, rows = 1;

		if(!node["columns"].isNull()) columns = node["columns"].Integer();
		if(!node["rows"].isNull()) rows = node["rows"].Integer();

		Point cellSize(controlRect.w / columns, controlRect.h / rows);

		int num = 0;

		for(auto child : node["items"].Vector())
		{
			Rect childViewport(
				controlRect.x + (num % columns) * cellSize.x,
				controlRect.y + (num / columns) * cellSize.y,
				cellSize.x,
				cellSize.y
			);

			container->addChild(parent->createControl(childViewport, child, dataBinder));
			num++;
		}

		return container;
	}
};

class ButtonFactory : public IControlFactory
{
public:
	virtual std::shared_ptr<CIntObject> createControl(Rect viewport, JsonNode & node, UiDataBinder & dataBinder) const
	{
		std::shared_ptr<CButton> button;

		button.reset(new CButton(
			Point(0, 0),
			node["def"].String(),
			CButton::tooltip(node["hover"].String())));

		Rect controlRect = getControlRect(viewport, Point(button->pos.w, button->pos.h), node);

		button->setPosition(controlRect.topLeft());

		if(!node["click"].isNull())
			button->addCallback(dataBinder.bindHandler(node["click"]));

		return button;
	}
};

class CompositeControlFactory : public IControlFactory
{
private:
	std::map<std::string, std::unique_ptr<IControlFactory>> subFactories;

public:
	CompositeControlFactory()
	{
		subFactories["Button"].reset(new ButtonFactory());
		subFactories["Container"].reset(new ContainerFactory(this));
		subFactories["Grid"].reset(new GridFactory(this));
	}

	virtual std::shared_ptr<CIntObject> createControl(Rect viewport, JsonNode & node, UiDataBinder & dataBinder) const
	{
		return subFactories.at(node["type"].String())->createControl(viewport, node, dataBinder);
	}
};

std::shared_ptr<CIntObject> GuiJsonLoader::loadJson(Rect viewport, ResourceID res, IUiController & controller)
{
	JsonNode controlsJson(res);

	CompositeControlFactory factory;
	UiDataBinder binder(controller);

	return factory.createControl(viewport, controlsJson, binder);
}