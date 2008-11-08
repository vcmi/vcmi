#pragma once
#include "../global.h"
class CAdvMapInt;
namespace config
{
	struct ClientConfig
	{
		int resx, resy, bpp, fullscreen; //client resolution/colours
		int port, localInformation;
		std::string server, //server address (e.g. 127.0.0.1)
			defaultAI; //dll name
	};
	struct AdventureMapConfig
	{
		struct ButtonInfo
		{
			std::string hoverName, //shows in statusbar when hovered
				helpBox, //shows in pop-up when r-clicked
				defName;
			std::vector<std::string> additionalDefs;
			void (CAdvMapInt::*func)(); //function in advmapint bound to that button
			int x, y; //position on the screen
			bool playerColoured; //if true button will be colored to main player's color (works properly only for appropriate 8bpp graphics)
		};
		struct Minimap
		{
			int x, y, w, h;
		} minimap;
		std::vector<ButtonInfo> buttons;
	};
	struct GUIOptions
	{
		AdventureMapConfig ac;
	};
	class CConfigHandler
	{
	public:
		ClientConfig cc;
		GUIOptions gc;
		void init();
		CConfigHandler(void);
		~CConfigHandler(void);
	};
}
extern config::CConfigHandler conf;