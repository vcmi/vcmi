package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class CodepageSettingController extends LauncherSettingWithDialogController<String, Config>
{
    public CodepageSettingController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<String> dialog()
    {
        return new CodepageSettingDialog();
    }

    @Override
    public void onItemChosen(final String item)
    {
        mConfig.updateCodepage(item);
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_cp_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }
        return mConfig.mCodepage == null || mConfig.mCodepage.isEmpty()
               ? mActivity.getString(R.string.launcher_btn_cp_subtitle_unknown)
               : mActivity.getString(R.string.launcher_btn_cp_subtitle, mConfig.mCodepage);
    }
}
