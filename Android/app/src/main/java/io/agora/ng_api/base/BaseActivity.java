package io.agora.ng_api.base;

import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ProgressBar;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import io.agora.ng_api.util.ExampleUtil;

public abstract class BaseActivity extends AppCompatActivity {
    private AlertDialog mLoadingDialog;
    public boolean isNightMode;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        isNightMode = ExampleUtil.isNightMode();
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        setupTheme();
    }

    public void showLoadingDialog(boolean cancelable) {
        if (mLoadingDialog == null) {
            mLoadingDialog = new AlertDialog.Builder(this).create();
            mLoadingDialog.getWindow().getDecorView().setBackgroundColor(Color.TRANSPARENT);

            ProgressBar progressBar = new ProgressBar(this);
            progressBar.setIndeterminate(true);
//            progressBar.setPadding(0, (int) ExampleUtil.dp2px(12), 0, (int) ExampleUtil.dp2px(12));
            progressBar.setLayoutParams(new FrameLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

            mLoadingDialog.setView(progressBar);
//            mLoadingDialog.setContentView(progressBar);
        }
        mLoadingDialog.setCancelable(cancelable);
        mLoadingDialog.show();
    }

    public void dismissLoading() {
        if (mLoadingDialog != null) {
            mLoadingDialog.dismiss();
        }
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if(isNightMode != ExampleUtil.isNightMode()){
            isNightMode = !isNightMode;
            onUIModeChanges();
            setupTheme();
        }
    }

    /**
     * dark mode & system bar
     *
     * @return isDarkMode
     */
    private void setupTheme() {
        WindowInsetsControllerCompat wic = new WindowInsetsControllerCompat(getWindow(), getWindow().getDecorView());

        wic.setAppearanceLightStatusBars(!isNightMode);
        wic.setAppearanceLightNavigationBars(!isNightMode);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getWindow().setNavigationBarColor(Color.TRANSPARENT);
        }
    }

    /**
     * For not recreate Activity
     * We must change color manually
     */
    protected abstract void onUIModeChanges();

}
