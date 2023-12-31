package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class SoundSettingController extends LauncherSettingWithSliderController<Integer, Config>
{
    public SoundSettingController(final AppCompatActivity act)
    {
        super(act, 0, 100);
    }

    @Override
    protected void onValueChanged(final int v)
    {
        mConfig.updateSound(v);
        updateContent();
    }

    @Override
    protected int currentValue()
    {
        if (mConfig == null)
        {
            return Config.DEFAULT_SOUND_VALUE;
        }
        return mConfig.mVolumeSound;
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_sound_title);
    }
}
