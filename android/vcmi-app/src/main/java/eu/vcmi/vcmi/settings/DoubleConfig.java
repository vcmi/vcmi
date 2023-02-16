package eu.vcmi.vcmi.settings;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.util.SharedPrefs;

/**
 * @author F
 */
public class DoubleConfig
{
    public final Config mConfig;
    public final SharedPrefs mPrefs;

    public DoubleConfig(final Config config, final SharedPrefs prefs)
    {
        mConfig = config;
        mPrefs = prefs;
    }
}
