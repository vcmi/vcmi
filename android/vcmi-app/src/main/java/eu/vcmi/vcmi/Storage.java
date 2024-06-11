package eu.vcmi.vcmi;

import android.content.Context;

import java.io.File;

public class Storage
{
    public static File getVcmiDataDir(Context context)
    {
        File root = context.getExternalFilesDir(null);
        return new File(root, Const.VCMI_DATA_ROOT_FOLDER_NAME);
    }
}
