package eu.vcmi.vcmi.settings;

import android.content.Context;

import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class PointerModeSettingDialog extends LauncherSettingDialog<PointerModeSettingController.PointerMode>
{
    private static final List<PointerModeSettingController.PointerMode> POINTER_MODES = new ArrayList<>();

    static
    {
        POINTER_MODES.add(PointerModeSettingController.PointerMode.NORMAL);
        POINTER_MODES.add(PointerModeSettingController.PointerMode.RELATIVE);
    }

    public PointerModeSettingDialog()
    {
        super(POINTER_MODES);
    }

    public static String pointerModeToUserString(
            final Context ctx,
            final PointerModeSettingController.PointerMode pointerMode)
    {
        switch (pointerMode)
        {
            default:
                return "";
            case NORMAL:
                return ctx.getString(R.string.misc_pointermode_normal);
            case RELATIVE:
                return ctx.getString(R.string.misc_pointermode_relative);
        }
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_pointermode_title;
    }

    @Override
    protected CharSequence itemName(final PointerModeSettingController.PointerMode item)
    {
        return pointerModeToUserString(getContext(), item);
    }

}
