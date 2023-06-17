package eu.vcmi.vcmi;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import org.json.JSONObject;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.content.AsyncLauncherInitialization;
import eu.vcmi.vcmi.settings.AdventureAiController;
import eu.vcmi.vcmi.settings.LanguageSettingController;
import eu.vcmi.vcmi.settings.CopyDataController;
import eu.vcmi.vcmi.settings.ExportDataController;
import eu.vcmi.vcmi.settings.LauncherSettingController;
import eu.vcmi.vcmi.settings.ModsBtnController;
import eu.vcmi.vcmi.settings.MusicSettingController;
import eu.vcmi.vcmi.settings.PointerModeSettingController;
import eu.vcmi.vcmi.settings.PointerMultiplierSettingController;
import eu.vcmi.vcmi.settings.ScreenResSettingController;
import eu.vcmi.vcmi.settings.SoundSettingController;
import eu.vcmi.vcmi.settings.StartGameController;
import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.SharedPrefs;

/**
 * @author F
 */
public class ActivityLauncher extends ActivityWithToolbar
{
    public static final int PERMISSIONS_REQ_CODE = 123;

    private final List<LauncherSettingController<?, ?>> mActualSettings = new ArrayList<>();
    private View mProgress;
    private TextView mErrorMessage;
    private Config mConfig;
    private LauncherSettingController<ScreenResSettingController.ScreenRes, Config> mCtrlScreenRes;
    private LauncherSettingController<String, Config> mCtrlLanguage;
    private LauncherSettingController<PointerModeSettingController.PointerMode, Config> mCtrlPointerMode;
    private LauncherSettingController<Void, Void> mCtrlStart;
    private LauncherSettingController<Float, Config> mCtrlPointerMulti;
    private LauncherSettingController<Integer, Config> mCtrlSoundVol;
    private LauncherSettingController<Integer, Config> mCtrlMusicVol;
    private LauncherSettingController<String, Config> mAiController;
    private CopyDataController mCtrlCopy;
    private ExportDataController mCtrlExport;

    private final AsyncLauncherInitialization.ILauncherCallbacks mInitCallbacks = new AsyncLauncherInitialization.ILauncherCallbacks()
    {
        @Override
        public Activity ctx()
        {
            return ActivityLauncher.this;
        }

        @Override
        public SharedPrefs prefs()
        {
            return mPrefs;
        }

        @Override
        public void onInitSuccess()
        {
            loadConfigFile();
            mCtrlStart.show();
            mCtrlCopy.show();
            mCtrlExport.show();
            for (LauncherSettingController<?, ?> setting: mActualSettings) {
                setting.show();
            }
            mErrorMessage.setVisibility(View.GONE);
            mProgress.setVisibility(View.GONE);
        }

        @Override
        public void onInitFailure(final AsyncLauncherInitialization.InitResult result)
        {
            mCtrlCopy.show();
            if (result.mFailSilently)
            {
                return;
            }
            ActivityLauncher.this.onInitFailure(result);
        }
    };

    @Override
    public void onCreate(final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        if (savedInstanceState == null) // only clear the log if this is initial onCreate and not config change
        {
            Log.init();
        }

        Log.i(this, "Starting launcher");

        setContentView(R.layout.activity_launcher);
        initToolbar(R.string.launcher_title, true);

        mProgress = findViewById(R.id.launcher_progress);
        mErrorMessage = (TextView) findViewById(R.id.launcher_error);
        mErrorMessage.setVisibility(View.GONE);

        ((TextView) findViewById(R.id.launcher_version_info)).setText(getString(R.string.launcher_version, BuildConfig.VERSION_NAME));

        initSettingsGui();
    }

    @Override
    public void onStart()
    {
        super.onStart();
        new AsyncLauncherInitialization(mInitCallbacks).execute((Void) null);
    }

    @Override
    public void onBackPressed()
    {
        saveConfig();
        super.onBackPressed();
    }

    @Override
    public boolean onCreateOptionsMenu(final Menu menu)
    {
        getMenuInflater().inflate(R.menu.menu_launcher, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item)
    {
        if (item.getItemId() == R.id.menu_launcher_about)
        {
            startActivity(new Intent(this, ActivityAbout.class));
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent resultData)
    {
        if(requestCode == CopyDataController.PICK_EXTERNAL_VCMI_DATA_TO_COPY
            && resultCode == Activity.RESULT_OK)
        {
            Uri uri;

            if (resultData != null)
            {
                uri = resultData.getData();

                mCtrlCopy.copyData(uri);
            }

            return;
        }

        if(requestCode == ExportDataController.PICK_DIRECTORY_TO_EXPORT
                && resultCode == Activity.RESULT_OK)
        {
            Uri uri = null;
            if (resultData != null)
            {
                uri = resultData.getData();

                mCtrlExport.copyData(uri);
            }

            return;
        }

        super.onActivityResult(requestCode, resultCode, resultData);
    }

    public void requestStoragePermissions()
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            requestPermissions(
                    new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    PERMISSIONS_REQ_CODE);
        }
    }

    private void initSettingsGui()
    {
        mCtrlStart = new StartGameController(this, v -> onLaunchGameBtnPressed()).init(R.id.launcher_btn_start);
        (mCtrlCopy = new CopyDataController(this)).init(R.id.launcher_btn_copy);
        (mCtrlExport = new ExportDataController(this)).init(R.id.launcher_btn_export);
        new ModsBtnController(this, v -> startActivity(new Intent(ActivityLauncher.this, ActivityMods.class))).init(R.id.launcher_btn_mods);
        mCtrlLanguage = new LanguageSettingController(this).init(R.id.launcher_btn_cp, mConfig);
        mCtrlPointerMode = new PointerModeSettingController(this).init(R.id.launcher_btn_pointer_mode, mConfig);
        mCtrlPointerMulti = new PointerMultiplierSettingController(this).init(R.id.launcher_btn_pointer_multi, mConfig);
        mCtrlSoundVol = new SoundSettingController(this).init(R.id.launcher_btn_volume_sound, mConfig);
        mCtrlMusicVol = new MusicSettingController(this).init(R.id.launcher_btn_volume_music, mConfig);
        mAiController = new AdventureAiController(this).init(R.id.launcher_btn_adventure_ai, mConfig);

        mActualSettings.clear();
        mActualSettings.add(mCtrlLanguage);
        mActualSettings.add(mCtrlPointerMode);
        mActualSettings.add(mCtrlPointerMulti);
        mActualSettings.add(mCtrlSoundVol);
        mActualSettings.add(mCtrlMusicVol);
        mActualSettings.add(mAiController);

        mCtrlStart.hide(); // start is initially hidden, until we confirm that everything is okay via AsyncLauncherInitialization
        mCtrlCopy.hide();
        mCtrlExport.hide();
    }

    private void onLaunchGameBtnPressed()
    {
        saveConfig();
        startActivity(new Intent(ActivityLauncher.this, VcmiSDLActivity.class));
    }

    private void saveConfig()
    {
        if (mConfig == null)
        {
            return;
        }

        try
        {
            mConfig.save(new File(FileUtil.configFileLocation(Storage.getVcmiDataDir(this))));
        }
        catch (final Exception e)
        {
            Toast.makeText(this, getString(R.string.launcher_error_config_saving_failed, e.getMessage()), Toast.LENGTH_LONG).show();
        }
    }

    private void loadConfigFile()
    {
        try
        {
            final String settingsFileContent = FileUtil.read(
                    new File(FileUtil.configFileLocation(Storage.getVcmiDataDir(this))));

            mConfig = Config.load(new JSONObject(settingsFileContent));
        }
        catch (final Exception e)
        {
            Log.e(this, "Could not load config file", e);
            mConfig = new Config();
        }
        onConfigUpdated();
    }

    private void onConfigUpdated()
    {
        updateCtrlConfig(mCtrlLanguage, mConfig);
        updateCtrlConfig(mCtrlPointerMode, mConfig);
        updateCtrlConfig(mCtrlPointerMulti, mConfig);
        updateCtrlConfig(mCtrlSoundVol, mConfig);
        updateCtrlConfig(mCtrlMusicVol, mConfig);
        updateCtrlConfig(mAiController, mConfig);
    }

    private <TSetting, TConf> void updateCtrlConfig(
            final LauncherSettingController<TSetting, TConf> ctrl,
            final TConf config)
    {
        if (ctrl != null)
        {
            ctrl.updateConfig(config);
        }
    }

    private void onInitFailure(final AsyncLauncherInitialization.InitResult initResult)
    {
        Log.d(this, "Init failed with " + initResult);

        mProgress.setVisibility(View.GONE);
        mCtrlStart.hide();

        for (LauncherSettingController<?, ?> setting: mActualSettings)
        {
            setting.hide();
        }

        mErrorMessage.setVisibility(View.VISIBLE);
        mErrorMessage.setText(initResult.mMessage);
    }
}
