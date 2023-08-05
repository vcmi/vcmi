package eu.vcmi.vcmi;

import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.Bundle;

import androidx.annotation.Nullable;
import com.google.android.material.snackbar.Snackbar;

import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.DefaultItemAnimator;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.common.GooglePlayServicesRepairableException;
import com.google.android.gms.common.GooglePlayServicesUtil;
import com.google.android.gms.security.ProviderInstaller;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

import eu.vcmi.vcmi.content.ModBaseViewHolder;
import eu.vcmi.vcmi.content.ModsAdapter;
import eu.vcmi.vcmi.mods.VCMIMod;
import eu.vcmi.vcmi.mods.VCMIModContainer;
import eu.vcmi.vcmi.mods.VCMIModsRepo;
import eu.vcmi.vcmi.util.InstallModAsync;
import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.ServerResponse;

/**
 * @author F
 */
public class ActivityMods extends ActivityWithToolbar
{
    private static final boolean ENABLE_REPO_DOWNLOADING = true;
    private static final String REPO_URL = "https://raw.githubusercontent.com/vcmi/vcmi-mods-repository/develop/vcmi-1.3.json";
    private VCMIModsRepo mRepo;
    private RecyclerView mRecycler;

    private VCMIModContainer mModContainer;
    private TextView mErrorMessage;
    private View mProgress;
    private ModsAdapter mModsAdapter;

    @Override
    protected void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mods);
        initToolbar(R.string.mods_title);

        mRepo = new VCMIModsRepo();

        mProgress = findViewById(R.id.mods_progress);

        mErrorMessage = (TextView) findViewById(R.id.mods_error_text);
        mErrorMessage.setVisibility(View.GONE);

        mRecycler = (RecyclerView) findViewById(R.id.mods_recycler);
        mRecycler.setItemAnimator(new DefaultItemAnimator());
        mRecycler.setLayoutManager(new LinearLayoutManager(this));
        mRecycler.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        mRecycler.setVisibility(View.GONE);

        mModsAdapter = new ModsAdapter(new OnAdapterItemAction());
        mRecycler.setAdapter(mModsAdapter);

        new AsyncLoadLocalMods().execute((Void) null);

        try {
            ProviderInstaller.installIfNeeded(this);
        } catch (GooglePlayServicesRepairableException e) {
            GooglePlayServicesUtil.getErrorDialog(e.getConnectionStatusCode(), this, 0);
        } catch (GooglePlayServicesNotAvailableException e) {
            Log.e("SecurityException", "Google Play Services not available.");
        }
    }

    private void loadLocalModData() throws IOException, JSONException
    {
        final File dataRoot = Storage.getVcmiDataDir(this);
        final String internalDataRoot = getFilesDir() + "/" + Const.VCMI_DATA_ROOT_FOLDER_NAME;

        final File modsRoot = new File(dataRoot,"/Mods");
        final File internalModsRoot = new File(internalDataRoot + "/Mods");
        if (!modsRoot.exists() && !internalModsRoot.exists())
        {
            Log.w(this, "We don't have mods folders");
            return;
        }
        final File[] modsFiles = modsRoot.listFiles();
        final File[] internalModsFiles = internalModsRoot.listFiles();
        final List<File> topLevelModsFolders = new ArrayList<>();
        if (modsFiles != null && modsFiles.length > 0)
        {
            Collections.addAll(topLevelModsFolders, modsFiles);
        }
        if (internalModsFiles != null && internalModsFiles.length > 0)
        {
            Collections.addAll(topLevelModsFolders, internalModsFiles);
        }
        mModContainer = VCMIModContainer.createContainer(topLevelModsFolders);

        final File modConfigFile = new File(dataRoot, "config/modSettings.json");
        if (!modConfigFile.exists())
        {
            Log.w(this, "We don't have mods config");
            return;
        }

        JSONObject rootConfigObj = new JSONObject(FileUtil.read(modConfigFile));
        JSONObject activeMods = rootConfigObj.getJSONObject("activeMods");
        mModContainer.updateContainerFromConfigJson(activeMods, rootConfigObj.optJSONObject("core"));

        Log.i(this, "Loaded mods: " + mModContainer);
    }

    @Override
    public boolean onCreateOptionsMenu(final Menu menu)
    {
        final MenuInflater menuInflater = getMenuInflater();
        menuInflater.inflate(R.menu.menu_mods, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item)
    {
        if (item.getItemId() == R.id.menu_mods_download_repo)
        {
            Log.i(this, "Should download repo now...");
            if (ENABLE_REPO_DOWNLOADING)
            {
                mProgress.setVisibility(View.VISIBLE);
                mRepo.init(REPO_URL, new OnModsRepoInitialized()); // disabled because the json is broken anyway
            }
            else
            {
                Snackbar.make(findViewById(R.id.mods_data_root), "Loading repo is disabled for now, because .json can't be parsed anyway",
                    Snackbar.LENGTH_LONG).show();
            }
        }
        return super.onOptionsItemSelected(item);
    }

    private void handleNoData()
    {
        mProgress.setVisibility(View.GONE);
        mRecycler.setVisibility(View.GONE);
        mErrorMessage.setVisibility(View.VISIBLE);
        mErrorMessage.setText("Could not load local mods list");
    }

    private void saveModSettingsToFile()
    {
        mModContainer.saveToFile(
                new File(
                        Storage.getVcmiDataDir(this),
                        "config/modSettings.json"));
    }

    private class OnModsRepoInitialized implements VCMIModsRepo.IOnModsRepoDownloaded
    {
        @Override
        public void onSuccess(ServerResponse<List<VCMIMod>> response)
        {
            Log.i(this, "Initialized mods repo");
			if (mModContainer == null)
			{
				handleNoData();
			}
			else
			{
				mModContainer.updateFromRepo(response.mContent);
				mModsAdapter.updateModsList(mModContainer.submods());
				mProgress.setVisibility(View.GONE);
			}
        }

        @Override
        public void onError(final int code)
        {
            Log.i(this, "Mods repo error: " + code);
        }
    }

    private class AsyncLoadLocalMods extends AsyncTask<Void, Void, Void>
    {
        @Override
        protected void onPreExecute()
        {
            mProgress.setVisibility(View.VISIBLE);
        }

        @Override
        protected Void doInBackground(final Void... params)
        {
            try
            {
                loadLocalModData();
            }
            catch (IOException e)
            {
                Log.e(this, "Loading local mod data failed", e);
            }
            catch (JSONException e)
            {
                Log.e(this, "Parsing local mod data failed", e);
            }
            return null;
        }

        @Override
        protected void onPostExecute(final Void aVoid)
        {
            if (mModContainer == null || !mModContainer.hasSubmods())
            {
                handleNoData();
            }
            else
            {
                mProgress.setVisibility(View.GONE);
                mRecycler.setVisibility(View.VISIBLE);
                mModsAdapter.updateModsList(mModContainer.submods());
            }
        }
    }

    private class OnAdapterItemAction implements ModsAdapter.IOnItemAction
    {
        @Override
        public void onItemPressed(final ModsAdapter.ModItem mod, final RecyclerView.ViewHolder vh)
        {
            Log.i(this, "Mod pressed: " + mod);
            if (mod.mMod.hasSubmods())
            {
                if (mod.mExpanded)
                {
                    mModsAdapter.detachSubmods(mod, vh);
                }
                else
                {
                    mModsAdapter.attachSubmods(mod, vh);
                    mRecycler.scrollToPosition(vh.getAdapterPosition() + 1);
                }
                mod.mExpanded = !mod.mExpanded;
            }
        }

        @Override
        public void onDownloadPressed(final ModsAdapter.ModItem mod, final RecyclerView.ViewHolder vh)
        {
            Log.i(this, "Mod download pressed: " + mod);
            mModsAdapter.downloadProgress(mod, "0%");
            installModAsync(mod);
        }

        @Override
        public void onTogglePressed(final ModsAdapter.ModItem item, final ModBaseViewHolder holder)
        {
            if(!item.mMod.mSystem && item.mMod.mInstalled)
            {
                item.mMod.mActive = !item.mMod.mActive;
                mModsAdapter.notifyItemChanged(holder.getAdapterPosition());
                saveModSettingsToFile();
            }
        }

        @Override
        public void onUninstall(ModsAdapter.ModItem item, ModBaseViewHolder holder)
        {
            File installationFolder = item.mMod.installationFolder;
            ActivityMods activity = ActivityMods.this;

            if(installationFolder != null){
                new AlertDialog.Builder(activity)
                    .setTitle(activity.getString(R.string.mods_removal_title, item.mMod.mName))
                    .setMessage(activity.getString(R.string.mods_removal_confirmation, item.mMod.mName))
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setNegativeButton(android.R.string.no, null)
                    .setPositiveButton(android.R.string.yes, (dialog, whichButton) ->
                    {
                        FileUtil.clearDirectory(installationFolder);
                        installationFolder.delete();

                        mModsAdapter.modRemoved(item);
                    })
                    .show();
            }
        }
    }

    private void installModAsync(ModsAdapter.ModItem mod){
        File dataDir = Storage.getVcmiDataDir(this);
        File modFolder = new File(
                new File(dataDir, "Mods"),
                mod.mMod.mId.toLowerCase(Locale.US));

        InstallModAsync modInstaller = new InstallModAsync(
            modFolder,
            this,
            new InstallModCallback(mod)
        );

        modInstaller.execute(mod.mMod.mArchiveUrl);
    }

    public class InstallModCallback implements InstallModAsync.PostDownload
    {
        private ModsAdapter.ModItem mod;

        public InstallModCallback(ModsAdapter.ModItem mod)
        {
            this.mod = mod;
        }

        @Override
        public void downloadDone(Boolean succeed, File modFolder)
        {
            if(succeed){
                mModsAdapter.modInstalled(mod, modFolder);
            }
        }

        @Override
        public void downloadProgress(String... progress)
        {
            if(progress.length > 0)
            {
                mModsAdapter.downloadProgress(mod, progress[0]);
            }
        }
    }
}
