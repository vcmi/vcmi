#ifndef CPREGAMETEXTHANDLER_H
#define CPREGAMETEXTHANDLER_H

#include <string>

class CPreGameTextHandler //handles pre - game texts
{
public:
	std::string mainNewGame, mainLoadGame, mainHighScores, mainCredits, mainQuit; //right - click texts in main menu
	void loadTexts();
};


#endif //CPREGAMETEXTHANDLER_H