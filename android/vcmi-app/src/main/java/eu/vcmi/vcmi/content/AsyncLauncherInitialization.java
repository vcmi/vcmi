package eu.vcmi.vcmi.content;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Environment;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.Const;
import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.Storage;
import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.LegacyConfigReader;
import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.SharedPrefs;

/**
 * @author F
 */
public class AsyncLauncherInitialization extends AsyncTask<Void, Void, AsyncLauncherInitialization.InitResult>
{
    private final WeakReference<ILauncherCallbacks> mCallbackRef;

    public AsyncLauncherInitialization(final ILauncherCallbacks callback)
    {
        mCallbackRef = new WeakReference<>(callback);
    }

    private InitResult init()
    {
        InitResult initResult = handleDataFoldersInitialization();

        if (!initResult.mSuccess)
        {
            return initResult;
        }
        Log.d(this, "Folders check passed");

        return initResult;
    }

    private InitResult handleDataFoldersInitialization()
    {
        final ILauncherCallbacks callbacks = mCallbackRef.get();

        if (callbacks == null)
        {
            return InitResult.failure("Internal error");
        }

        final Context ctx = callbacks.ctx();
        final File vcmiDir = Storage.getVcmiDataDir(ctx);

        final File internalDir = ctx.getFilesDir();
        final File vcmiInternalDir = new File(internalDir, Const.VCMI_DATA_ROOT_FOLDER_NAME);
        Log.i(this, "Using " + vcmiDir.getAbsolutePath() + " as root vcmi dir");

        if(!vcmiInternalDir.exists()) vcmiInternalDir.mkdir();
        if(!vcmiDir.exists()) vcmiDir.mkdir();

        if (!Storage.testH3DataFolder(ctx))
        {
            // no h3 data present -> instruct user where to put it
            return InitResult.failure(
                ctx.getString(
                    R.string.launcher_error_h3_data_missing,
                        Storage.getVcmiDataDir(ctx)));
        }

        final File testVcmiData = new File(vcmiInternalDir, "Mods/vcmi/mod.json");
        final boolean internalVcmiDataExisted = testVcmiData.exists();
        if (!internalVcmiDataExisted && !FileUtil.unpackVcmiDataToInternalDir(vcmiInternalDir, ctx.getAssets()))
        {
            // no h3 data present -> instruct user where to put it
            return InitResult.failure(ctx.getString(R.string.launcher_error_vcmi_data_internal_missing));
        }

        final String previousInternalDataHash = callbacks.prefs().load(SharedPrefs.KEY_CURRENT_INTERNAL_ASSET_HASH, null);
        final String currentInternalDataHash = FileUtil.readAssetsStream(ctx.getAssets(), "internalDataHash.txt");
        if (currentInternalDataHash == null || previousInternalDataHash == null || !currentInternalDataHash.equals(previousInternalDataHash))
        {
            // we should update the data only if it existed previously (hash is bound to be empty if we have just created the data)
            if (internalVcmiDataExisted)
            {
                Log.i(this, "Internal data needs to be created/updated; old hash=" + previousInternalDataHash
                            + ", new hash=" + currentInternalDataHash);
                if (!FileUtil.reloadVcmiDataToInternalDir(vcmiInternalDir, ctx.getAssets()))
                {
                    return InitResult.failure(ctx.getString(R.string.launcher_error_vcmi_data_internal_update));
                }
            }
            callbacks.prefs().save(SharedPrefs.KEY_CURRENT_INTERNAL_ASSET_HASH, currentInternalDataHash);
        }

        return InitResult.success();
    }

    @Override
    protected InitResult doInBackground(final Void... params)
    {
        return init();
    }

    @Override
    protected void onPostExecute(final InitResult initResult)
    {
        final ILauncherCallbacks callbacks = mCallbackRef.get();
        if (callbacks == null)
        {
            return;
        }

        if (initResult.mSuccess)
        {
            callbacks.onInitSuccess();
        }
        else
        {
            callbacks.onInitFailure(initResult);
        }
    }

    public interface ILauncherCallbacks
    {
        Activity ctx();

        SharedPrefs prefs();

        void onInitSuccess();

        void onInitFailure(InitResult result);
    }

    public static final class InitResult
    {
        public final boolean mSuccess;
        public final String mMessage;
        public boolean mFailSilently;

        public InitResult(final boolean success, final String message)
        {
            mSuccess = success;
            mMessage = message;
        }

        @Override
        public String toString()
        {
            return String.format("success: %s (%s)", mSuccess, mMessage);
        }

        public static InitResult failure(String message)
        {
            return new InitResult(false, message);
        }

        public static InitResult success()
        {
            return new InitResult(true, "");
        }
    }
}
