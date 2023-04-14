package eu.vcmi.vcmi;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import android.view.MenuItem;
import android.view.ViewStub;

/**
 * @author F
 */
public abstract class ActivityWithToolbar extends ActivityBase
{
    @Override
    public void setContentView(final int layoutResId)
    {
        super.setContentView(R.layout.activity_toolbar_wrapper);
        final ViewStub contentStub = (ViewStub) findViewById(R.id.toolbar_wrapper_content_stub);
        contentStub.setLayoutResource(layoutResId);
        contentStub.inflate();
    }

    @Override
    public boolean onOptionsItemSelected(final MenuItem item)
    {
        if (item.getItemId() == android.R.id.home)
        {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    protected void initToolbar(final int textResId)
    {
        initToolbar(textResId, false);
    }

    protected void initToolbar(final int textResId, final boolean isTopLevelActivity)
    {
        final Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        toolbar.setTitle(textResId);

        if (!isTopLevelActivity)
        {
            final ActionBar bar = getSupportActionBar();
            if (bar != null)
            {
                bar.setDisplayHomeAsUpEnabled(true);
            }
        }
    }
}
