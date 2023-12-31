package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class MusicSettingController extends LauncherSettingWithSliderController<Integer, Config>
{
    public MusicSettingController(final AppCompatActivity act)
    {
        super(act, 0, 100);
    }

    @Override
    protected void onValueChanged(final int v)
    {
        mConfig.updateMusic(v);
        updateContent();
    }

    @Override
    protected int currentValue()
    {
        if (mConfig == null)
        {
            return Config.DEFAULT_MUSIC_VALUE;
        }
        return mConfig.mVolumeMusic;
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_music_title);
    }
}
