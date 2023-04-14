package eu.vcmi.vcmi;

import android.content.Context;
import java.io.File;
import java.io.IOException;
import eu.vcmi.vcmi.util.FileUtil;

public class Storage
{
    public static File getVcmiDataDir(Context context)
    {
        File root = context.getExternalFilesDir(null);

        return new File(root, Const.VCMI_DATA_ROOT_FOLDER_NAME);
    }

    public static boolean testH3DataFolder(Context context)
    {
        return testH3DataFolder(getVcmiDataDir(context));
    }

    public static boolean testH3DataFolder(final File baseDir)
    {
        final File testH3Data = new File(baseDir, "Data");
        return testH3Data.exists();
    }

    public static String getH3DataFolder(Context context){
        return getVcmiDataDir(context).getAbsolutePath();
    }
}
