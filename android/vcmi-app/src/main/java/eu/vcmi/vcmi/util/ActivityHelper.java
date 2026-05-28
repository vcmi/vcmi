package eu.vcmi.vcmi.util;

import android.app.Activity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

/**
 * Shared helpers for VCMI Activities.
 */
public class ActivityHelper
{
	/**
	 * Applies immersive full-screen mode (hide navigation + status bar)
	 * to the given Activity. Call from onCreate() and onWindowFocusChanged().
	 */
	public static void applyImmersiveFullscreen(Activity activity)
	{
		Window window = activity.getWindow();
		View decorView = window.getDecorView();

		window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

		decorView.setSystemUiVisibility(
				View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
				| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_FULLSCREEN
				| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
		);

		WindowInsetsControllerCompat insetsController = WindowCompat.getInsetsController(window, decorView);
		if (insetsController != null)
		{
			insetsController.hide(WindowInsetsCompat.Type.systemBars());
			insetsController.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
		}
	}
}
