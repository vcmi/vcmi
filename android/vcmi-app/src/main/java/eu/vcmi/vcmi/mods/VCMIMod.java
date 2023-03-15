package eu.vcmi.vcmi.mods;

import android.text.TextUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import eu.vcmi.vcmi.BuildConfig;
import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public class VCMIMod
{
    protected final Map<String, VCMIMod> mSubmods;
    public String mId;
    public String mName;
    public String mDesc;
    public String mVersion;
    public String mAuthor;
    public String mContact;
    public String mModType;
    public String mArchiveUrl;
    public long mSize;
    public File installationFolder;

    // config values
    public boolean mActive;
    public boolean mInstalled;
    public boolean mValidated;
    public String mChecksum;

    // internal
    public boolean mLoadedCorrectly;
    public boolean mSystem;

    protected VCMIMod()
    {
        mSubmods = new HashMap<>();
    }

    public static VCMIMod buildFromRepoJson(final String id,
                                            final JSONObject obj,
                                            JSONObject modDownloadData)
    {
        final VCMIMod mod = new VCMIMod();
        mod.mId = id.toLowerCase(Locale.US);
        mod.mName = obj.optString("name");
        mod.mDesc = obj.optString("description");
        mod.mVersion = obj.optString("version");
        mod.mAuthor = obj.optString("author");
        mod.mContact = obj.optString("contact");
        mod.mModType = obj.optString("modType");
        mod.mArchiveUrl = modDownloadData.optString("download");
        mod.mSize = obj.optLong("size");
        mod.mLoadedCorrectly = true;
        return mod;
    }

    public static VCMIMod buildFromConfigJson(final String id, final JSONObject obj) throws JSONException
    {
        final VCMIMod mod = new VCMIMod();
        mod.updateFromConfigJson(id, obj);
        mod.mInstalled = true;
        return mod;
    }

    public static VCMIMod buildFromModInfo(final File modPath) throws IOException, JSONException
    {
        final VCMIMod mod = new VCMIMod();
        if (!mod.updateFromModInfo(modPath))
        {
            return mod;
        }
        mod.mLoadedCorrectly = true;
        mod.mActive = true; // active by default
        mod.mInstalled = true;
        mod.installationFolder = modPath;

        return mod;
    }

    protected static Map<String, VCMIMod> loadSubmods(final List<File> modsList) throws IOException, JSONException
    {
        final Map<String, VCMIMod> submods = new HashMap<>();
        for (final File f : modsList)
        {
            if (!f.isDirectory())
            {
                Log.w("VCMI", "Non-directory encountered in mods dir: " + f.getName());
                continue;
            }

            final VCMIMod submod = buildFromModInfo(f);
            if (submod == null)
            {
                Log.w(null, "Could not build mod in folder " + f + "; ignoring");
                continue;
            }

            submods.put(submod.mId, submod);
        }
        return submods;
    }

    public void updateFromConfigJson(final String id, final JSONObject obj) throws JSONException
    {
        if(mSystem)
        {
            return;
        }

        mId = id.toLowerCase(Locale.US);
        mActive = obj.optBoolean("active");
        mValidated = obj.optBoolean("validated");
        mChecksum = obj.optString("checksum");

        final JSONObject submods = obj.optJSONObject("mods");
        if (submods != null)
        {
            updateChildrenFromConfigJson(submods);
        }
    }

    protected void updateChildrenFromConfigJson(final JSONObject submods) throws JSONException
    {
        final JSONArray names = submods.names();
        for (int i = 0; i < names.length(); ++i)
        {
            final String modId = names.getString(i);
            final String normalizedModId = modId.toLowerCase(Locale.US);
            if (!mSubmods.containsKey(normalizedModId))
            {
                Log.w(this, "Mod present in config but not found in /Mods; ignoring: " + modId);
                continue;
            }

            mSubmods.get(normalizedModId).updateFromConfigJson(modId, submods.getJSONObject(modId));
        }
    }

    public boolean updateFromModInfo(final File modPath) throws IOException, JSONException
    {
        final File modInfoFile = new File(modPath, "mod.json");
        if (!modInfoFile.exists())
        {
            Log.w(this, "Mod info doesn't exist");
            mName = modPath.getAbsolutePath();
            return false;
        }
        try
        {
            final JSONObject modInfoContent = new JSONObject(FileUtil.read(modInfoFile));
            mId = modPath.getName().toLowerCase(Locale.US);
            mName = modInfoContent.optString("name");
            mDesc = modInfoContent.optString("description");
            mVersion = modInfoContent.optString("version");
            mAuthor = modInfoContent.optString("author");
            mContact = modInfoContent.optString("contact");
            mModType = modInfoContent.optString("modType");
            mSystem = mId.equals("vcmi");

            final File submodsDir = new File(modPath, "Mods");
            if (submodsDir.exists())
            {
                final List<File> submodsFiles = new ArrayList<>();
                Collections.addAll(submodsFiles, submodsDir.listFiles());
                mSubmods.putAll(loadSubmods(submodsFiles));
            }
            return true;
        }
        catch (final JSONException ex)
        {
            mName = modPath.getAbsolutePath();
            return false;
        }
    }

    @Override
    public String toString()
    {
        if (!BuildConfig.DEBUG)
        {
            return "";
        }
        return String.format("mod:[id:%s,active:%s,submods:[%s]]", mId, mActive, TextUtils.join(",", mSubmods.values()));
    }

    protected void submodsToJson(final JSONObject modsRoot) throws JSONException
    {
        for (final VCMIMod submod : mSubmods.values())
        {
            final JSONObject submodEntry = new JSONObject();
            submod.toJsonInternal(submodEntry);
            modsRoot.put(submod.mId, submodEntry);
        }
    }

    protected void toJsonInternal(final JSONObject root) throws JSONException
    {
        root.put("active", mActive);
        root.put("validated", mValidated);
        if (!TextUtils.isEmpty(mChecksum))
        {
            root.put("checksum", mChecksum);
        }
        if (!mSubmods.isEmpty())
        {
            JSONObject submods = new JSONObject();
            submodsToJson(submods);
            root.put("mods", submods);
        }
    }

    public boolean hasSubmods()
    {
        return !mSubmods.isEmpty();
    }

    public List<VCMIMod> submods()
    {
        final ArrayList<VCMIMod> ret = new ArrayList<>();

        ret.addAll(mSubmods.values());

        Collections.sort(ret, new Comparator<VCMIMod>()
        {
            @Override
            public int compare(VCMIMod left, VCMIMod right)
            {
                return left.mName.compareTo(right.mName);
            }
        });

        return ret;
    }

    protected void updateFrom(VCMIMod other)
    {
        this.mModType = other.mModType;
        this.mAuthor = other.mAuthor;
        this.mDesc = other.mDesc;
        this.mArchiveUrl = other.mArchiveUrl;
    }
}
