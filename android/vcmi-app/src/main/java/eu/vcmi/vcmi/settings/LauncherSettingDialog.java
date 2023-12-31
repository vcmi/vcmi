package eu.vcmi.vcmi.settings;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AlertDialog;

import java.util.ArrayList;

import java.util.List;

import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public abstract class LauncherSettingDialog<T> extends DialogFragment
{
    protected final List<T> mDataset;
    private IOnItemChosen<T> mObserver;

    protected LauncherSettingDialog(final List<T> dataset)
    {
        mDataset = dataset;
    }

    public void observe(final IOnItemChosen<T> observer)
    {
        mObserver = observer;
    }

    protected abstract CharSequence itemName(T item);

    protected abstract int dialogTitleResId();

    @NonNull
    @Override
    public Dialog onCreateDialog(final Bundle savedInstanceState)
    {
        List<CharSequence> list = new ArrayList<>();

        for (T t : mDataset)
        {
            CharSequence charSequence = itemName(t);
            list.add(charSequence);
        }

        return new AlertDialog.Builder(getActivity())
            .setTitle(dialogTitleResId())
            .setItems(
                    list.toArray(new CharSequence[0]),
                    this::onItemChosenInternal)
            .create();
    }

    private void onItemChosenInternal(final DialogInterface dialog, final int index)
    {
        final T chosenItem = mDataset.get(index);
        Log.d(this, "Chosen item: " + chosenItem);
        dialog.dismiss();
        if (mObserver != null)
        {
            mObserver.onItemChosen(chosenItem);
        }
    }

    public interface IOnItemChosen<V>
    {
        void onItemChosen(V item);
    }
}
