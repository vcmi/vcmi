package eu.vcmi.vcmi.settings;

import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class CodepageSettingDialog extends LauncherSettingDialog<String>
{
    private static final List<String> AVAILABLE_CODEPAGES = new ArrayList<>();

    static
    {
        AVAILABLE_CODEPAGES.add("CP1250");
        AVAILABLE_CODEPAGES.add("CP1251");
        AVAILABLE_CODEPAGES.add("CP1252");
        AVAILABLE_CODEPAGES.add("GBK");
        AVAILABLE_CODEPAGES.add("GB2312");
    }

    public CodepageSettingDialog()
    {
        super(AVAILABLE_CODEPAGES);
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_cp_title;
    }

    @Override
    protected CharSequence itemName(final String item)
    {
        return item;
    }
}
