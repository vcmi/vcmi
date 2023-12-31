package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import android.view.View;

import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public abstract class LauncherSettingWithDialogController<T, Conf> extends LauncherSettingController<T, Conf>
    implements LauncherSettingDialog.IOnItemChosen<T>
{
    public static final String SETTING_DIALOG_ID = "settings.dialog";

    protected LauncherSettingWithDialogController(final AppCompatActivity act)
    {
        super(act);
    }

    @Override
    public void onClick(final View v)
    {
        Log.i(this, "Showing dialog");
        final LauncherSettingDialog<T> dialog = dialog();
        dialog.observe(this); // TODO rebinding dialogs on activity config changes
        dialog.show(mActivity.getSupportFragmentManager(), SETTING_DIALOG_ID);
    }

    protected abstract LauncherSettingDialog<T> dialog();
}
