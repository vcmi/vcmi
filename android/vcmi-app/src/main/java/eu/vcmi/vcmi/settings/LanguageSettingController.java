package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class LanguageSettingController extends LauncherSettingWithDialogController<String, Config>
{
    public LanguageSettingController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<String> dialog()
    {
        return new LanguageSettingDialog();
    }

    @Override
    public void onItemChosen(final String item)
    {
        mConfig.updateLanguage(item);
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_language_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }
        return mConfig.mLanguage == null || mConfig.mLanguage.isEmpty()
               ? mActivity.getString(R.string.launcher_btn_language_subtitle_unknown)
               : mActivity.getString(R.string.launcher_btn_language_subtitle, mConfig.mLanguage);
    }
}
