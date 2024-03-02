package eu.vcmi.vcmi.util;

import eu.vcmi.vcmi.BuildConfig;

/**
 * @author F
 */

public class Log
{
    private static final boolean LOGGING_ENABLED_CONSOLE = BuildConfig.DEBUG;
    private static final String TAG_PREFIX = "VCMI/";
    private static final String STATIC_TAG = "static";

    private static void log(final int priority, final Object obj, final String msg)
    {
        logInternal(priority, tag(obj), msg);
    }

    private static void logInternal(final int priority, final String tagString, final String msg)
    {
        if (LOGGING_ENABLED_CONSOLE)
        {
            android.util.Log.println(priority, TAG_PREFIX + tagString, msg);
        }
    }

    private static String formatPriority(final int priority)
    {
        switch (priority)
        {
            default:
                return "?";
            case android.util.Log.VERBOSE:
                return "V";
            case android.util.Log.DEBUG:
                return "D";
            case android.util.Log.INFO:
                return "I";
            case android.util.Log.WARN:
                return "W";
            case android.util.Log.ERROR:
                return "E";
        }
    }

    private static String tag(final Object obj)
    {
        if (obj == null)
        {
            return "null";
        }
        return obj.getClass().getSimpleName();
    }

    public static void v(final String msg)
    {
        logInternal(android.util.Log.VERBOSE, STATIC_TAG, msg);
    }

    public static void d(final String msg)
    {
        logInternal(android.util.Log.DEBUG, STATIC_TAG, msg);
    }

    public static void i(final String msg)
    {
        logInternal(android.util.Log.INFO, STATIC_TAG, msg);
    }

    public static void w(final String msg)
    {
        logInternal(android.util.Log.WARN, STATIC_TAG, msg);
    }

    public static void e(final String msg)
    {
        logInternal(android.util.Log.ERROR, STATIC_TAG, msg);
    }

    public static void v(final Object obj, final String msg)
    {
        log(android.util.Log.VERBOSE, obj, msg);
    }

    public static void d(final Object obj, final String msg)
    {
        log(android.util.Log.DEBUG, obj, msg);
    }

    public static void i(final Object obj, final String msg)
    {
        log(android.util.Log.INFO, obj, msg);
    }

    public static void w(final Object obj, final String msg)
    {
        log(android.util.Log.WARN, obj, msg);
    }

    public static void e(final Object obj, final String msg)
    {
        log(android.util.Log.ERROR, obj, msg);
    }

    public static void e(final Object obj, final String msg, final Throwable e)
    {
        log(android.util.Log.ERROR, obj, msg + "\n" + android.util.Log.getStackTraceString(e));
    }

    public static void e(final String msg, final Throwable e)
    {
        logInternal(android.util.Log.ERROR, STATIC_TAG, msg + "\n" + android.util.Log.getStackTraceString(e));
    }
}
