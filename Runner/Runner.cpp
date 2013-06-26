// Runner.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;

void runBattle(string battle, string left, string right)
{
	string command = "VCMI_client -b" + battle + " --ai=" + left + " --ai=" + right + " --noGUI";
	system(command.c_str());
}

int _tmain(int argc, _TCHAR* argv[])
{
	const int N = 5;

	std::vector<std::string> ais;
	ais.push_back("StupidAI");
	ais.push_back("BattleAI");

	std::vector<std::string> battles;
	battles.push_back("bitwa1.json");
	battles.push_back("bitwa2.json");
	battles.push_back("bitwa3.json");
	battles.push_back("bitwa4.json");
	battles.push_back("bitwa5.json");
	battles.push_back("bitwa6.json");

	for(auto &battle : battles)
	{
		auto runN = [&](string left, string right)
		{
			for(int i  =0; i < N; i++)
				runBattle(battle, left, right);
		};

		runN(ais[0], ais[0]);
		runN(ais[0], ais[1]);
		runN(ais[1], ais[0]);
		runN(ais[1], ais[1]);
	}


	return 0;
}

