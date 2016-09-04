#include "StdInc.h"
#include "sdldisplayquery.h"

#include <QString>
#include <QTextStream>

#include <SDL.h>
#include <SDL_video.h>

QStringList getDisplays()
{
  if(SDL_Init(SDL_INIT_VIDEO))
    return QStringList("default display");

  const int displays = SDL_GetNumVideoDisplays();
  QStringList list;

  for (int display = 0; display < displays; ++display)
    {
      SDL_Rect rect;

      if (SDL_GetDisplayBounds (display, &rect))
	continue;

      QString string;
      QTextStream(&string) << display << " - " << rect.w << "x" << rect.h << " (at " << rect.x << ", " << rect.y << ")";

      list << string;
    }

  SDL_Quit();
  return list;
}
