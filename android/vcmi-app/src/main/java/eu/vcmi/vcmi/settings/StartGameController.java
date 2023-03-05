package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import android.view.View;

import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.util.GeneratedVersion;

/**
 * @author F
 */
public class StartGameController extends LauncherSettingController<Void, Void>
{
    private View.OnClickListener mOnSelectedAction;

    public StartGameController(final AppCompatActivity act, final View.OnClickListener onSelectedAction)
    {
        super(act);
        mOnSelectedAction = onSelectedAction;
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_start_title);
    }

    @Override
    protected String subText()
    {
        return mActivity.getString(R.string.launcher_btn_start_subtitle, GeneratedVersion.VCMI_VERSION);
    }

    @Override
    public void onClick(final View v)
    {
        mOnSelectedAction.onClick(v);
    }
}
