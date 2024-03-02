package eu.vcmi.vcmi;

import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.Nullable;

import eu.vcmi.vcmi.VcmiSDLActivity;

import org.libsdl.app.SDL;

/**
 * @author F
 */
public class ActivityLauncher extends org.qtproject.qt5.android.bindings.QtActivity
{
    public boolean justLaunched = true;

    @Override
    public void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        justLaunched = savedInstanceState == null;
        SDL.setContext(this);
    }

    public void onLaunchGameBtnPressed()
    {
        startActivity(new Intent(ActivityLauncher.this, VcmiSDLActivity.class));
    }
}
