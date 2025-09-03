package eu.vcmi.vcmi;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.Nullable;

import java.io.File;

import eu.vcmi.vcmi.VcmiSDLActivity;
import eu.vcmi.vcmi.util.FileUtil;

import org.libsdl.app.SDL;

/**
 * @author F
 */
public class ActivityLauncher extends org.qtproject.qt5.android.bindings.QtActivity
{
    private static final int PICK_EXTERNAL_VCMI_DATA_TO_COPY = 1;

    public boolean justLaunched = true;

    @Override
    public void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        justLaunched = savedInstanceState == null;
        SDL.setContext(this);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent resultData)
    {
        if (requestCode == PICK_EXTERNAL_VCMI_DATA_TO_COPY && resultCode == Activity.RESULT_OK)
        {
            if (resultData != null && FileUtil.copyData(resultData.getData(), this))
                NativeMethods.heroesDataUpdate();
            return;
        }

        super.onActivityResult(requestCode, resultCode, resultData);
    }

    public void copyHeroesData()
    {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI,
            Uri.fromFile(new File(Environment.getExternalStorageDirectory(), "vcmi-data"))
        );
        startActivityForResult(intent, PICK_EXTERNAL_VCMI_DATA_TO_COPY);
    }

    public void keepScreenOn(boolean isEnabled)
    {
        if(isEnabled)
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        else
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    public void onLaunchGameBtnPressed()
    {
        startActivity(new Intent(ActivityLauncher.this, VcmiSDLActivity.class));
    }
}
