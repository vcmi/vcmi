package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import eu.vcmi.vcmi.Config;
import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class AdventureAiController extends LauncherSettingWithDialogController<String, Config>
{
    public AdventureAiController(final AppCompatActivity activity)
    {
        super(activity);
    }

    @Override
    protected LauncherSettingDialog<String> dialog()
    {
        return new AdventureAiSelectionDialog();
    }

    @Override
    public void onItemChosen(final String item)
    {
        mConfig.setAdventureAi(item);
        updateContent();
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_adventure_ai_title);
    }

    @Override
    protected String subText()
    {
        if (mConfig == null)
        {
            return "";
        }

        return mConfig.getAdventureAi();
    }
}
