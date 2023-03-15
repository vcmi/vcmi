package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class PointerModeSettingController
    extends LauncherSettingWithDialogController<PointerModeSettingController.PointerMode, Config>
{
    public PointerModeSettingController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<PointerMode> dialog()
    {
        return new PointerModeSettingDialog();
    }

    @Override
    public void onItemChosen(final PointerMode item)
    {
        mConfig.setPointerMode(item == PointerMode.RELATIVE);
        mConfig.updateSwipe(item.supportsSwipe());
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_pointermode_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }
        return mActivity.getString(R.string.launcher_btn_pointermode_subtitle,
            PointerModeSettingDialog.pointerModeToUserString(mActivity, getPointerMode()));
    }

    private PointerMode getPointerMode()
    {
        if(mConfig.getPointerModeIsRelative())
        {
            return PointerMode.RELATIVE;
        }

        if(mConfig.mSwipeEnabled)
        {
            return PointerMode.NORMAL_WITH_SWIPE;
        }

        return PointerMode.NORMAL;
    }

    public enum PointerMode
    {
        NORMAL,
        NORMAL_WITH_SWIPE,
        RELATIVE;

        public boolean supportsSwipe()
        {
            return this == NORMAL_WITH_SWIPE;
        }
    }
}
