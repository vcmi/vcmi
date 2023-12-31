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
        final File testH3data = new File(baseDir, "data");
        final File testH3DATA = new File(baseDir, "DATA");
        return testH3Data.exists() || testH3data.exists() || testH3DATA.exists();
    }

    public static String getH3DataFolder(Context context){
        return getVcmiDataDir(context).getAbsolutePath();
    }
}
