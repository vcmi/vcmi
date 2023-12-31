package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class PointerMultiplierSettingController
        extends LauncherSettingWithDialogController<Float, Config>
{
    public PointerMultiplierSettingController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<Float> dialog()
    {
        return new PointerMultiplierSettingDialog();
    }

    @Override
    public void onItemChosen(final Float item)
    {
        mConfig.setPointerSpeedMultiplier(item);
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_pointermulti_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }

        String pointerModeString = PointerMultiplierSettingDialog.pointerMultiplierToUserString(
                mConfig.getPointerSpeedMultiplier());

        return mActivity.getString(R.string.launcher_btn_pointermulti_subtitle, pointerModeString);
    }
}
