package eu.vcmi.vcmi.settings;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class PointerMultiplierSettingDialog extends LauncherSettingDialog<Float>
{
    private static final List<Float> AVAILABLE_MULTIPLIERS = new ArrayList<>();

    static
    {
        AVAILABLE_MULTIPLIERS.add(1.0f);
        AVAILABLE_MULTIPLIERS.add(1.25f);
        AVAILABLE_MULTIPLIERS.add(1.5f);
        AVAILABLE_MULTIPLIERS.add(1.75f);
        AVAILABLE_MULTIPLIERS.add(2.0f);
        AVAILABLE_MULTIPLIERS.add(2.5f);
        AVAILABLE_MULTIPLIERS.add(3.0f);
    }

    public PointerMultiplierSettingDialog()
    {
        super(AVAILABLE_MULTIPLIERS);
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_pointermode_title;
    }

    @Override
    protected CharSequence itemName(final Float item)
    {
        return pointerMultiplierToUserString(item);
    }

    public static String pointerMultiplierToUserString(final float multiplier)
    {
        return String.format(Locale.US, "%.2fx", multiplier);
    }
}
