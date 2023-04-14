package eu.vcmi.vcmi.viewmodels;

import android.view.View;

import androidx.lifecycle.ViewModel;
import androidx.databinding.PropertyChangeRegistry;
import androidx.databinding.Observable;

/**
 * @author F
 */
public class ObservableViewModel extends ViewModel implements Observable
{
    private PropertyChangeRegistry callbacks = new PropertyChangeRegistry();

    @Override
    public void addOnPropertyChangedCallback(
            Observable.OnPropertyChangedCallback callback)
    {
        callbacks.add(callback);
    }

    @Override
    public void removeOnPropertyChangedCallback(
            Observable.OnPropertyChangedCallback callback)
    {
        callbacks.remove(callback);
    }

    public int visible(boolean isVisible)
    {
        return isVisible ? View.VISIBLE : View.GONE;
    }

    /**
     * Notifies observers that all properties of this instance have changed.
     */
    void notifyChange() {
        callbacks.notifyCallbacks(this, 0, null);
    }

    /**
     * Notifies observers that a specific property has changed. The getter for the
     * property that changes should be marked with the @Bindable annotation to
     * generate a field in the BR class to be used as the fieldId parameter.
     *
     * @param fieldId The generated BR id for the Bindable field.
     */
    void notifyPropertyChanged(int fieldId) {
        callbacks.notifyCallbacks(this, fieldId, null);
    }
}