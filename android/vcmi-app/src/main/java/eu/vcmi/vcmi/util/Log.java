package eu.vcmi.vcmi.util;

import android.os.Environment;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DateFormat;
import java.util.Date;

import eu.vcmi.vcmi.BuildConfig;
import eu.vcmi.vcmi.Const;

/**
 * @author F
 */

public class Log
{
    private static final boolean LOGGING_ENABLED_CONSOLE = BuildConfig.DEBUG;
    private static final boolean LOGGING_ENABLED_FILE = true;
    private static final String FILELOG_PATH = "/" + Const.VCMI_DATA_ROOT_FOLDER_NAME + "/cache/VCMI_launcher.log";
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
        if (LOGGING_ENABLED_FILE) // this is probably very inefficient, but should be enough for now...
        {
            try
            {
                final BufferedWriter fileWriter = new BufferedWriter(new FileWriter(Environment.getExternalStorageDirectory() + FILELOG_PATH, true));
                fileWriter.write(String.format("[%s] %s: %s\n", formatPriority(priority), tagString, msg));
                fileWriter.flush();
                fileWriter.close();
            }
            catch (IOException ignored)
            {
            }
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

    public static void init()
    {
        if (LOGGING_ENABLED_FILE) // clear previous log
        {
            try
            {
                final BufferedWriter fileWriter = new BufferedWriter(new FileWriter(Environment.getExternalStorageDirectory() + FILELOG_PATH, false));
                fileWriter.write("Starting VCMI launcher log, " + DateFormat.getDateTimeInstance().format(new Date()) + "\n");
                fileWriter.flush();
                fileWriter.close();
            }
            catch (IOException ignored)
            {
            }
        }
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
