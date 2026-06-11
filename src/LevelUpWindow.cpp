void LevelUpWindow::showWindow()
{
    // ...
    if (isShowingRewardWindow())
    {
        // Delay showing the reward window to prevent crashes
        CGui->addTimer(100, [this] { showRewardWindow(); });
    }
    else
    {
        showRewardWindow();
    }
}

void LevelUpWindow::showRewardWindow()
{
    // ...
    // Ensure the level up window is fully closed before showing the reward window
    if (isLevelUpWindowOpen())
    {
        closeLevelUpWindow();
        CGui->addTimer(100, [this] { showRewardWindow(); });
        return;
    }
    // ...
}