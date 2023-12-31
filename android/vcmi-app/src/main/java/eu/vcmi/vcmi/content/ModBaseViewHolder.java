package eu.vcmi.vcmi.content;

import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import eu.vcmi.vcmi.R;

/**
 * @author F
 */
public class ModBaseViewHolder extends RecyclerView.ViewHolder
{
    final View mModNesting;
    final TextView mModName;

    ModBaseViewHolder(final View parentView)
    {
        this(
            LayoutInflater.from(parentView.getContext()).inflate(
                R.layout.mod_base_adapter_item,
                (ViewGroup) parentView,
                false),
            true);
    }

    protected ModBaseViewHolder(final View v, final boolean internal)
    {
        super(v);
        mModNesting = itemView.findViewById(R.id.mods_adapter_item_nesting);
        mModName = (TextView) itemView.findViewById(R.id.mods_adapter_item_name);
    }
}
