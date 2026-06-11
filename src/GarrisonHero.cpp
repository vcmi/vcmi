void GarrisonHero::levelUp()
{
    // ...
    if (isGarrisonHeroInTown())
    {
        // Delay showing the level up window to prevent crashes
        CGui->addTimer(100, [this] { showLevelUpWindow(); });
    }
    else
    {
        showLevelUpWindow();
    }
}