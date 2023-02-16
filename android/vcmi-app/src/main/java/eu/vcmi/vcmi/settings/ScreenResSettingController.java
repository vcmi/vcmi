package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class ScreenResSettingController extends LauncherSettingWithDialogController<ScreenResSettingController.ScreenRes, Config>
{
    public ScreenResSettingController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<ScreenRes> dialog()
    {
        return new ScreenResSettingDialog(mActivity);
    }

    @Override
    public void onItemChosen(final ScreenRes item)
    {
        mConfig.updateResolution(item.mWidth, item.mHeight);
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_res_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }
        return mConfig.mResolutionWidth <= 0 || mConfig.mResolutionHeight <= 0
               ? mActivity.getString(R.string.launcher_btn_res_subtitle_unknown)
               : mActivity.getString(R.string.launcher_btn_res_subtitle, mConfig.mResolutionWidth, mConfig.mResolutionHeight);
    }

    public static class ScreenRes
    {
        public int mWidth;
        public int mHeight;

        public ScreenRes(final int width, final int height)
        {
            mWidth = width;
            mHeight = height;
        }

        @Override
        public String toString()
        {
            return mWidth + "x" + mHeight;
        }
    }
}
