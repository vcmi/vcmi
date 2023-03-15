package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import android.view.View;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class ModsBtnController extends LauncherSettingController<Void, Void>
{
    private View.OnClickListener mOnSelectedAction;

    public ModsBtnController(final AppCompatActivity act, final View.OnClickListener onSelectedAction)
    {
        super(act);
        mOnSelectedAction = onSelectedAction;
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_mods_title);
    }

    @Override
    protected String subText()
    {
        return mActivity.getString(R.string.launcher_btn_mods_subtitle);
    }

    @Override
    public void onClick(final View v)
    {
        mOnSelectedAction.onClick(v);
    }
}
