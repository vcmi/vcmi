package eu.vcmi.vcmi.util;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.util.DisplayMetrics;

/**
 * @author F
 */

public final class Utils
{
    private static String sAppVersionCache;

    private Utils()
    {
    }

    public static String appVersionName(final Context ctx)
    {
        if (sAppVersionCache == null)
        {
            final PackageManager pm = ctx.getPackageManager();
            try
            {
                final PackageInfo info = pm.getPackageInfo(ctx.getPackageName(), PackageManager.GET_META_DATA);
                return sAppVersionCache = info.versionName;
            }
            catch (final PackageManager.NameNotFoundException e)
            {
                Log.e(ctx, "Couldn't resolve app version", e);
            }
        }
        return sAppVersionCache;
    }

    public static float convertDpToPx(final Context ctx, final float dp)
    {
        return convertDpToPx(ctx.getResources(), dp);
    }

    public static float convertDpToPx(final Resources res, final float dp)
    {
        return dp * res.getDisplayMetrics().density;
    }

    public static float convertPxToDp(final Context ctx, final float px)
    {
        return convertPxToDp(ctx.getResources(), px);
    }

    public static float convertPxToDp(final Resources res, final float px)
    {
        return px / res.getDisplayMetrics().density;
    }
}
