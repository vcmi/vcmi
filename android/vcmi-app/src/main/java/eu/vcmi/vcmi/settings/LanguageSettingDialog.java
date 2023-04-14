package eu.vcmi.vcmi.settings;

import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class LanguageSettingDialog extends LauncherSettingDialog<String>
{
    private static final List<String> AVAILABLE_LANGUAGES = new ArrayList<>();

    static
    {
        AVAILABLE_LANGUAGES.add("english");
        AVAILABLE_LANGUAGES.add("chinese");
        AVAILABLE_LANGUAGES.add("french");
        AVAILABLE_LANGUAGES.add("german");
        AVAILABLE_LANGUAGES.add("korean");
        AVAILABLE_LANGUAGES.add("polish");
        AVAILABLE_LANGUAGES.add("russian");
        AVAILABLE_LANGUAGES.add("spanish");
        AVAILABLE_LANGUAGES.add("ukrainian");
        AVAILABLE_LANGUAGES.add("other_cp1250");
        AVAILABLE_LANGUAGES.add("other_cp1251");
        AVAILABLE_LANGUAGES.add("other_cp1252");
    }

    public LanguageSettingDialog()
    {
        super(AVAILABLE_LANGUAGES);
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_language_title;
    }

    @Override
    protected CharSequence itemName(final String item)
    {
        return item;
    }
}
