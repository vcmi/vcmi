package eu.vcmi.vcmi;

import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.util.Notifications;

/**
 * Asks before the quit button of the background notification actually ends the game.
 */
public class ActivityQuitConfirm extends AppCompatActivity
{
    @Override
    protected void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setFinishOnTouchOutside(false);

        new AlertDialog.Builder(this)
            .setTitle(R.string.quit_title)
            .setMessage(R.string.quit_message)
            .setPositiveButton(android.R.string.ok, (dialog, which) -> quit())
            .setNegativeButton(android.R.string.cancel, (dialog, which) -> finish())
            .setOnCancelListener(dialog -> finish())
            .show();
    }

    private void quit()
    {
        Notifications.stopService(this);
        finishAndRemoveTask();

        // on mobile the server is a thread of the client process, so this ends both
        Runtime.getRuntime().exit(0);
    }
}
