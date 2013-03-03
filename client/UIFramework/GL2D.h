#pragma once

namespace GL2D
{
	void initVideo(ui32 w, ui32 h, bool fullscreen);
	void attachToCurrentThread();
	bool setScreenRes(ui32 w, ui32 h, bool fullscreen);

	inline ui32 getScreenWidth() { extern ui32 screenWidth; return screenWidth; };
	inline ui32 getScreenHeight() { extern ui32 screenHeight; return screenHeight; };

	void assignTexture(ui32 slot, ui32 type, ui32 handle);
	void useNoShader();
	void useColorizeShader(const float cm[4][4]);
	void usePaletteBitmapShader(si32 x, si32 y);
	void usePaletteBitmapShader(si32 x, si32 y, const float cm[4][4]);
}
