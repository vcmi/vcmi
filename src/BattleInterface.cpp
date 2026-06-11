void BattleInterface::showVictoryWindow()
{
    // ...
    if (isLevelUpWindowOpen())
    {
        // Delay showing the victory window to prevent crashes
        CGui->addTimer(100, [this] { showVictoryWindow(); });
        return;
    }
    // ...
}