package eu.vcmi.vcmi.util;

import android.annotation.TargetApi;
import android.text.TextUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;

import eu.vcmi.vcmi.Const;

/**
 * helper used to retrieve old vcmi config (currently only needed for h3 data path in order to migrate the data to the new location)
 *
 * @author F
 */
public final class LegacyConfigReader
{
    private static void skipBools(final ObjectInputStream stream, final int num) throws IOException
    {
        for (int i = 0; i < num; ++i)
        {
            stream.readBoolean();
        }
    }

    private static void skipInts(final ObjectInputStream stream, final int num) throws IOException
    {
        for (int i = 0; i < num; ++i)
        {
            stream.readInt();
        }
    }

    @TargetApi(Const.SUPPRESS_TRY_WITH_RESOURCES_WARNING)
    public static Config load(final File basePath)
    {
        final File settingsFile = new File(basePath, "/libsdl-settings.cfg");
        if (!settingsFile.exists())
        {
            Log.i("Legacy config file doesn't exist");
            return null;
        }

        try (final ObjectInputStream stream = new ObjectInputStream(new FileInputStream(settingsFile)))
        {
            if (stream.readInt() != 5)
            {
                return null;
            }
            skipBools(stream, 5);
            skipInts(stream, 9);
            skipBools(stream, 2);
            skipInts(stream, 2);
            stream.readBoolean();
            skipInts(stream, 2);
            skipInts(stream, stream.readInt());
            stream.readInt();
            skipInts(stream, 6);
            stream.readInt();
            skipBools(stream, 8);
            stream.readInt();
            stream.readInt();
            for (int i = 0; i < 4; i++)
            {
                stream.readInt();
                stream.readBoolean();
            }
            skipInts(stream, 5);
            final StringBuilder b = new StringBuilder();
            final int len = stream.readInt();
            for (int i = 0; i < len; i++)
            {
                b.append(stream.readChar());
            }

            final Config config = new Config();
            config.mDataPath = b.toString();
            Log.v("Retrieved legacy data folder name: " + config.mDataPath);
            if (!TextUtils.isEmpty(config.mDataPath) && new File(config.mDataPath).exists())
            {
                // return config only if there actually is a chance of retrieving old data
                return config;
            }
            Log.i("Couldn't find valid data in legacy config");
        }
        catch (final Exception e)
        {
            Log.i("Couldn't load legacy config");
        }

        return null;
    }

    public static class Config
    {
        public String mDataPath;
    }
}
