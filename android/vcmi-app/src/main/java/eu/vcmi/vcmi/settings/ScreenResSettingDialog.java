package eu.vcmi.vcmi.settings;

import android.app.Activity;

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
public class ScreenResSettingDialog extends LauncherSettingDialog<ScreenResSettingController.ScreenRes>
{
    public ScreenResSettingDialog(Activity mActivity)
    {
        super(loadResolutions(mActivity));
    }

    @Override
    protected int dialogTitleResId()
    {
        return R.string.launcher_btn_res_title;
    }

    @Override
    protected CharSequence itemName(final ScreenResSettingController.ScreenRes item)
    {
        return item.toString();
    }

    private static List<ScreenResSettingController.ScreenRes> loadResolutions(Activity activity)
    {
        List<ScreenResSettingController.ScreenRes> availableResolutions = new ArrayList<>();

        try
        {
            File modsFolder = new File(Storage.getVcmiDataDir(activity), "Mods");
            Queue<File> folders = new ArrayDeque<File>();
            folders.offer(modsFolder);

            while (!folders.isEmpty())
            {
                File folder = folders.poll();
                File[] children = folder.listFiles();

                if(children == null) continue;

                for (File child : children)
                {
                    if (child.isDirectory())
                    {
                        folders.add(child);
                    }
                    else if (child.getName().equals("resolutions.json"))
                    {
                        JSONArray resolutions = new JSONObject(FileUtil.read(child))
                                .getJSONArray("GUISettings");

                        for(int index = 0; index < resolutions.length(); index++)
                        {
                            try
                            {
                                JSONObject resolution = resolutions
                                        .getJSONObject(index)
                                        .getJSONObject("resolution");

                                availableResolutions.add(new ScreenResSettingController.ScreenRes(
                                        resolution.getInt("x"),
                                        resolution.getInt("y")
                                ));
                            }
                            catch (Exception ex)
                            {
                                ex.printStackTrace();
                            }
                        }
                    }
                }
            }

            if(availableResolutions.isEmpty())
            {
                availableResolutions.add(new ScreenResSettingController.ScreenRes(800, 600));
            }
        }
        catch(Exception ex)
        {
            ex.printStackTrace();

            availableResolutions.clear();

            availableResolutions.add(new ScreenResSettingController.ScreenRes(800, 600));
        }

        return availableResolutions;
    }
}
