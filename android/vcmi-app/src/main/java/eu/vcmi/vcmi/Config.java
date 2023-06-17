package eu.vcmi.vcmi;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;

import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public class Config
{
    public static final String DEFAULT_LANGUAGE = "english";
    public static final int DEFAULT_MUSIC_VALUE = 5;
    public static final int DEFAULT_SOUND_VALUE = 5;

    public String mLanguage;
    public int mVolumeSound;
    public int mVolumeMusic;
    private String adventureAi;
    private double mPointerSpeedMultiplier;
    private boolean mUseRelativePointer;
    private JSONObject mRawObject;

    private boolean mIsModified;

    private static JSONObject accessNode(final JSONObject baseObj, String type)
    {
        if (baseObj == null)
        {
            return null;
        }

        return baseObj.optJSONObject(type);
    }

    private static double loadDouble(final JSONObject node, final String key, final double fallback)
    {
        if (node == null)
        {
            return fallback;
        }

        return node.optDouble(key, fallback);
    }

    @SuppressWarnings("unchecked")
    private static <T> T loadEntry(final JSONObject node, final String key, final T fallback)
    {
        if (node == null)
        {
            return fallback;
        }
        final Object value = node.opt(key);
        return value == null ? fallback : (T) value;
    }

    public static Config load(final JSONObject obj)
    {
        Log.v("loading config from json: " + obj.toString());
        final Config config = new Config();
        final JSONObject general = accessNode(obj, "general");
        final JSONObject server = accessNode(obj, "server");
        config.mLanguage = loadEntry(general, "language", DEFAULT_LANGUAGE);
        config.mVolumeSound = loadEntry(general, "sound", DEFAULT_SOUND_VALUE);
        config.mVolumeMusic = loadEntry(general, "music", DEFAULT_MUSIC_VALUE);
        config.adventureAi = loadEntry(server, "playerAI", "Nullkiller");
        config.mUseRelativePointer = loadEntry(general, "userRelativePointer", false);
        config.mPointerSpeedMultiplier = loadDouble(general, "relativePointerSpeedMultiplier", 1.0);

        config.mRawObject = obj;
        return config;
    }

    public void updateLanguage(final String s)
    {
        mLanguage = s;
        mIsModified = true;
    }

    public void updateSound(final int i)
    {
        mVolumeSound = i;
        mIsModified = true;
    }

    public void updateMusic(final int i)
    {
        mVolumeMusic = i;
        mIsModified = true;
    }

    public void setAdventureAi(String ai)
    {
        adventureAi = ai;
        mIsModified = true;
    }

    public String getAdventureAi()
    {
        return this.adventureAi == null ? "Nullkiller" : this.adventureAi;
    }

    public void setPointerSpeedMultiplier(float speedMultiplier)
    {
        mPointerSpeedMultiplier = speedMultiplier;
        mIsModified = true;
    }

    public float getPointerSpeedMultiplier()
    {
        return (float)mPointerSpeedMultiplier;
    }

    public void setPointerMode(boolean isRelative)
    {
        mUseRelativePointer = isRelative;
        mIsModified = true;
    }

    public boolean getPointerModeIsRelative()
    {
        return mUseRelativePointer;
    }

    public void save(final File location) throws IOException, JSONException
    {
        if (!needsSaving(location))
        {
            Log.d(this, "Config doesn't need saving");
            return;
        }
        try
        {
            final String configString = toJson();
            FileUtil.write(location, configString);
            Log.v(this, "Saved config: " + configString);
        }
        catch (final Exception e)
        {
            Log.e(this, "Could not save config", e);
            throw e;
        }
    }

    private boolean needsSaving(final File location)
    {
        return mIsModified || !location.exists();
    }

    private String toJson() throws JSONException
    {
        final JSONObject generalNode = accessNode(mRawObject, "general");
        final JSONObject serverNode = accessNode(mRawObject, "server");

        final JSONObject root = mRawObject == null ? new JSONObject() : mRawObject;
        final JSONObject general = generalNode == null ? new JSONObject() : generalNode;
        final JSONObject server = serverNode == null ? new JSONObject() : serverNode;

        if (mLanguage != null)
        {
            general.put("language", mLanguage);
        }

        general.put("music", mVolumeMusic);
        general.put("sound", mVolumeSound);
        general.put("userRelativePointer", mUseRelativePointer);
        general.put("relativePointerSpeedMultiplier", mPointerSpeedMultiplier);
        root.put("general", general);

        if(this.adventureAi != null)
        {
            server.put("playerAI", this.adventureAi);
            root.put("server", server);
        }

        return root.toString();
    }
}
