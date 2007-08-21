#ifndef CSCREENHANDLER_H
#define CSCREENHANDLER_H

struct SDL_Thread;

class CScreenHandler
{
private:
	SDL_Thread * myth;
public:
	void initScreen();
	void updateScreen();
};



#endif //CSCREENHANDLER_H