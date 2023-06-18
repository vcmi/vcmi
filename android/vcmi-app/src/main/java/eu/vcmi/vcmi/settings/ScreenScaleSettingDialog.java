package eu.vcmi.vcmi.settings;

import android.app.Activity;
import android.graphics.Point;
import android.view.WindowMetrics;

import org.json.JSONArray;
import org.json.JSONObject;
import java.io.File;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.Storage;
import eu.vcmi.vcmi.util.FileUtil;

/**
 * @author F
 */
public class ScreenScaleSettingDialog extends LauncherSettingDialog<ScreenScaleSettingController.ScreenScale>
{
    public ScreenScaleSettingDialog(Activity mActivity)
    {
        super(loadScales(mActivity));
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_scale_title;
    }

    @Override
    protected CharSequence itemName(final ScreenScaleSettingController.ScreenScale item)
    {
        return item.toString();
    }

    public static int[] getSupportedScalingRange(Activity activity) {
        Point screenRealSize = new Point();
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            WindowMetrics windowMetrics = activity.getWindowManager().getCurrentWindowMetrics();
            screenRealSize.x = windowMetrics.getBounds().width();
            screenRealSize.y = windowMetrics.getBounds().height();
        } else {
            activity.getWindowManager().getDefaultDisplay().getRealSize(screenRealSize);
        }

        if (screenRealSize.x < screenRealSize.y) {
            int tmp = screenRealSize.x;
            screenRealSize.x = screenRealSize.y;
            screenRealSize.y = tmp;
        }

        // H3 resolution, any resolution smaller than that is not correctly supported
        Point minResolution = new Point(800, 600);
        // arbitrary limit on *downscaling*. Allow some downscaling, if requested by user. Should be generally limited to 100+ for all but few devices
        double minimalScaling = 50;

        Point renderResolution = screenRealSize;
        double maximalScalingWidth = 100.0 * renderResolution.x / minResolution.x;
        double maximalScalingHeight = 100.0 * renderResolution.y / minResolution.y;
        double maximalScaling = Math.min(maximalScalingWidth, maximalScalingHeight);

        return new int[] { (int)minimalScaling, (int)maximalScaling };
    }

    private static List<ScreenScaleSettingController.ScreenScale> loadScales(Activity activity)
    {
        List<ScreenScaleSettingController.ScreenScale> availableScales = new ArrayList<>();

        try
        {
            int[] supportedScalingRange = getSupportedScalingRange(activity);
            for (int i = 0; i <= supportedScalingRange[1] + 10 - 1; i += 10)
            {
                if (i >= supportedScalingRange[0])
                    availableScales.add(new ScreenScaleSettingController.ScreenScale(i));
            }

            if(availableScales.isEmpty())
            {
                availableScales.add(new ScreenScaleSettingController.ScreenScale(100));
            }
        }
        catch(Exception ex)
        {
            ex.printStackTrace();

            availableScales.clear();

            availableScales.add(new ScreenScaleSettingController.ScreenScale(100));
        }

        return availableScales;
    }
}
