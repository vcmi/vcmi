package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import android.view.View;
import android.widget.TextView;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public abstract class LauncherSettingController<TSetting, TConf> implements View.OnClickListener
{
    protected AppCompatActivity mActivity;
    protected TConf mConfig;
    private View mSettingViewRoot;
    private TextView mSettingsTextMain;
    private TextView mSettingsTextSub;

    LauncherSettingController(final AppCompatActivity act)
    {
        mActivity = act;
    }

    public final LauncherSettingController<TSetting, TConf> init(final int rootViewResId)
    {
        return init(rootViewResId, null);
    }

    public final LauncherSettingController<TSetting, TConf> init(final int rootViewResId, final TConf config)
    {
        mSettingViewRoot = mActivity.findViewById(rootViewResId);
        mSettingViewRoot.setOnClickListener(this);
        mSettingsTextMain = (TextView) mSettingViewRoot.findViewById(R.id.inc_launcher_btn_main);
        mSettingsTextSub = (TextView) mSettingViewRoot.findViewById(R.id.inc_launcher_btn_sub);
        childrenInit(mSettingViewRoot);
        updateConfig(config);
        updateContent();
        return this;
    }

    protected void childrenInit(final View root)
    {

    }

    public void updateConfig(final TConf conf)
    {
        mConfig = conf;
        updateContent();
    }

    public void updateContent()
    {
        mSettingsTextMain.setText(mainText());
        if (mSettingsTextSub != null)
        {
            mSettingsTextSub.setText(subText());
        }
    }

    protected abstract String mainText();

    protected abstract String subText();

    public void show()
    {
        mSettingViewRoot.setVisibility(View.VISIBLE);
    }

    public void hide()
    {
        mSettingViewRoot.setVisibility(View.GONE);
    }
}
