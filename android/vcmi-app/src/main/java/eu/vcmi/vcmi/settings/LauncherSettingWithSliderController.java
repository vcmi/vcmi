package eu.vcmi.vcmi.settings;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatSeekBar;
import android.view.View;
import android.widget.SeekBar;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public abstract class LauncherSettingWithSliderController<T, Conf> extends LauncherSettingController<T, Conf>
{
    private AppCompatSeekBar mSlider;
    private final int mSliderMin;
    private final int mSliderMax;

    protected LauncherSettingWithSliderController(final AppCompatActivity act, final int min, final int max)
    {
        super(act);
        mSliderMin = min;
        mSliderMax = max;
    }

    @Override
    protected void childrenInit(final View root)
    {
        mSlider = (AppCompatSeekBar) root.findViewById(R.id.inc_launcher_btn_slider);
        if (mSliderMax <= mSliderMin)
        {
            throw new IllegalArgumentException("slider min>=max");
        }
        mSlider.setMax(mSliderMax - mSliderMin);
        mSlider.setOnSeekBarChangeListener(new OnValueChangedListener());
    }

    protected abstract void onValueChanged(final int v);
    protected abstract int currentValue();

    @Override
    public void updateContent()
    {
        super.updateContent();
        mSlider.setProgress(currentValue() + mSliderMin);
    }

    @Override
    protected String subText()
    {
        return null; // not used with slider settings
    }

    @Override
    public void onClick(final View v)
    {
        // not used with slider settings
    }

    private class OnValueChangedListener implements SeekBar.OnSeekBarChangeListener
    {
        @Override
        public void onProgressChanged(final SeekBar seekBar, final int progress, final boolean fromUser)
        {
            if (fromUser)
            {
                onValueChanged(progress);
            }
        }

        @Override
        public void onStartTrackingTouch(final SeekBar seekBar)
        {

        }

        @Override
        public void onStopTrackingTouch(final SeekBar seekBar)
        {

        }
    }
}
