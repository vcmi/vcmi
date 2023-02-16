package eu.vcmi.vcmi.content;

import android.content.Context;
import androidx.recyclerview.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.mods.VCMIMod;
import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public class ModsAdapter extends RecyclerView.Adapter<ModBaseViewHolder>
{
    private static final int NESTING_WIDTH_PER_LEVEL = 16;
    private static final int VIEWTYPE_MOD = 0;
    private static final int VIEWTYPE_FAILED_MOD = 1;
    private final List<ModItem> mDataset = new ArrayList<>();
    private final IOnItemAction mItemListener;

    public ModsAdapter(final IOnItemAction itemListener)
    {
        mItemListener = itemListener;
    }

    @Override
    public ModBaseViewHolder onCreateViewHolder(final ViewGroup parent, final int viewType)
    {
        switch (viewType)
        {
            case VIEWTYPE_MOD:
                return new ModsViewHolder(parent);
            case VIEWTYPE_FAILED_MOD:
                return new ModBaseViewHolder(parent);
            default:
                Log.e(this, "Unhandled view type: " + viewType);
                return null;
        }
    }

    @Override
    public void onBindViewHolder(final ModBaseViewHolder holder, final int position)
    {
        final ModItem item = mDataset.get(position);
        final int viewType = getItemViewType(position);

        final Context ctx = holder.itemView.getContext();
        holder.mModNesting.getLayoutParams().width = item.mNestingLevel * NESTING_WIDTH_PER_LEVEL;
        switch (viewType)
        {
            case VIEWTYPE_MOD:
                final ModsViewHolder modHolder = (ModsViewHolder) holder;
                modHolder.mModName.setText(item.mMod.mName + ", " + item.mMod.mVersion);
                modHolder.mModType.setText(item.mMod.mModType);
                if (item.mMod.mSize > 0)
                {
                    modHolder.mModSize.setVisibility(View.VISIBLE);
                    // TODO unit conversion
                    modHolder.mModSize.setText(String.format(Locale.getDefault(), "%.1f kB", item.mMod.mSize / 1024.0f));
                }
                else
                {
                    modHolder.mModSize.setVisibility(View.GONE);
                }
                modHolder.mModAuthor.setText(ctx.getString(R.string.mods_item_author_template, item.mMod.mAuthor));
                modHolder.mStatusIcon.setImageResource(selectModStatusIcon(item.mMod.mActive));

                modHolder.mDownloadBtn.setVisibility(View.GONE);
                modHolder.mDownloadProgress.setVisibility(View.GONE);
                modHolder.mUninstall.setVisibility(View.GONE);

                if(!item.mMod.mSystem)
                {
                    if (item.mDownloadProgress != null)
                    {
                        modHolder.mDownloadProgress.setText(item.mDownloadProgress);
                        modHolder.mDownloadProgress.setVisibility(View.VISIBLE);
                    }
                    else if (!item.mMod.mInstalled)
                    {
                        modHolder.mDownloadBtn.setVisibility(View.VISIBLE);
                    }
                    else if (item.mMod.installationFolder != null)
                    {
                        modHolder.mUninstall.setVisibility(View.VISIBLE);
                    }

                    modHolder.itemView.setOnClickListener(v -> mItemListener.onItemPressed(item, holder));
                    modHolder.mStatusIcon.setOnClickListener(v -> mItemListener.onTogglePressed(item, holder));
                    modHolder.mDownloadBtn.setOnClickListener(v -> mItemListener.onDownloadPressed(item, holder));
                    modHolder.mUninstall.setOnClickListener(v -> mItemListener.onUninstall(item, holder));
                }

                break;
            case VIEWTYPE_FAILED_MOD:
                holder.mModName.setText(ctx.getString(R.string.mods_failed_mod_loading, item.mMod.mName));
                break;
            default:
                Log.e(this, "Unhandled view type: " + viewType);
                break;
        }
    }

    private int selectModStatusIcon(final boolean active)
    {
        // TODO distinguishing mods that aren't downloaded or have an update available
        if (active)
        {
            return R.drawable.ic_star_full;
        }
        return R.drawable.ic_star_empty;
    }

    @Override
    public int getItemViewType(final int position)
    {
        return mDataset.get(position).mMod.mLoadedCorrectly ? VIEWTYPE_MOD : VIEWTYPE_FAILED_MOD;
    }

    @Override
    public int getItemCount()
    {
        return mDataset.size();
    }

    public void attachSubmods(final ModItem mod, final RecyclerView.ViewHolder vh)
    {
        int adapterPosition = vh.getAdapterPosition();
        final List<ModItem> submods = new ArrayList<>();

        for (VCMIMod v : mod.mMod.submods())
        {
            ModItem modItem = new ModItem(v, mod.mNestingLevel + 1);
            submods.add(modItem);
        }

        mDataset.addAll(adapterPosition + 1, submods);
        notifyItemRangeInserted(adapterPosition + 1, submods.size());
    }

    public void detachSubmods(final ModItem mod, final RecyclerView.ViewHolder vh)
    {
        final int adapterPosition = vh.getAdapterPosition();
        final int checkedPosition = adapterPosition + 1;
        int detachedElements = 0;
        while (checkedPosition < mDataset.size() && mDataset.get(checkedPosition).mNestingLevel > mod.mNestingLevel)
        {
            ++detachedElements;
            mDataset.remove(checkedPosition);
        }
        notifyItemRangeRemoved(checkedPosition, detachedElements);
    }

    public void updateModsList(List<VCMIMod> mods)
    {
        mDataset.clear();

        List<ModItem> list = new ArrayList<>();

        for (VCMIMod mod : mods)
        {
            ModItem modItem = new ModItem(mod);
            list.add(modItem);
        }

        mDataset.addAll(list);

        notifyDataSetChanged();
    }

    public void modInstalled(ModItem mod, File modFolder)
    {
        try
        {
            mod.mMod.updateFromModInfo(modFolder);
            mod.mMod.mLoadedCorrectly = true;
            mod.mMod.mActive = true; // active by default
            mod.mMod.mInstalled = true;
            mod.mMod.installationFolder = modFolder;
            mod.mDownloadProgress = null;
            notifyItemChanged(mDataset.indexOf(mod));
        }
        catch (Exception ex)
        {
            Log.e("Failed to install mod", ex);
        }
    }

    public void downloadProgress(ModItem mod, String progress)
    {
        mod.mDownloadProgress = progress;
        notifyItemChanged(mDataset.indexOf(mod));
    }

    public void modRemoved(ModItem item)
    {
        int itemIndex = mDataset.indexOf(item);

        if(item.mMod.mArchiveUrl != null && item.mMod.mArchiveUrl != "")
        {
            item.mMod.mInstalled = false;
            item.mMod.installationFolder = null;

            notifyItemChanged(itemIndex);
        }
        else
        {
            mDataset.remove(item);
            notifyItemRemoved(itemIndex);
        }
    }

    public interface IOnItemAction
    {
        void onItemPressed(final ModItem mod, final RecyclerView.ViewHolder vh);

        void onDownloadPressed(final ModItem mod, final RecyclerView.ViewHolder vh);

        void onTogglePressed(ModItem item, ModBaseViewHolder holder);

        void onUninstall(ModItem item, ModBaseViewHolder holder);
    }

    public static class ModItem
    {
        public final VCMIMod mMod;
        public int mNestingLevel;
        public boolean mExpanded;
        public String mDownloadProgress;

        public ModItem(final VCMIMod mod)
        {
            this(mod, 0);
        }

        public ModItem(final VCMIMod mod, final int nestingLevel)
        {
            mMod = mod;
            mNestingLevel = nestingLevel;
            mExpanded = false;
        }

        @Override
        public String toString()
        {
            return mMod.toString();
        }
    }
}
