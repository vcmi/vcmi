package eu.vcmi.vcmi.settings;

import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class AdventureAiSelectionDialog extends LauncherSettingDialog<String>
{
    private static final List<String> AVAILABLE_AI = new ArrayList<>();

    static
    {
        AVAILABLE_AI.add("VCAI");
        AVAILABLE_AI.add("Nullkiller");
    }

    public AdventureAiSelectionDialog()
    {
        super(AVAILABLE_AI);
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_adventure_ai_title;
    }

    @Override
    protected CharSequence itemName(final String item)
    {
        return item;
    }
}
