package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class ScreenScaleSettingController extends LauncherSettingWithDialogController<ScreenScaleSettingController.ScreenScale, Config>
{
    public ScreenScaleSettingController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<ScreenScale> dialog()
    {
        return new ScreenScaleSettingDialog(mActivity);
    }

    @Override
    public void onItemChosen(final ScreenScale item)
    {
        mConfig.updateScale(item.mScreenScale);
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_scale_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }
        return mConfig.mScreenScale <= 0
               ? mActivity.getString(R.string.launcher_btn_scale_subtitle_unknown)
               : mActivity.getString(R.string.launcher_btn_scale_subtitle, mConfig.mScreenScale);
    }

    public static class ScreenScale
    {
        public int mScreenScale;

        public ScreenScale(final int scale)
        {
            mScreenScale = scale;
        }
    }
}
