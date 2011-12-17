#include "StdInc.h"
#include "CKeyShortcut.h"

void CKeyShortcut::keyPressed(const SDL_KeyboardEvent & key)
{
	if(vstd::contains(assignedKeys,key.keysym.sym))
	{
		bool prev = pressedL;
		if(key.state == SDL_PRESSED) 
		{
			pressedL = true;
			clickLeft(true, prev);
		} 
		else 
		{
			pressedL = false;
			clickLeft(false, prev);
		}
	}
}