package io.agora.ng_api.base;

import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ProgressBar;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;
import androidx.core.view.WindowInsetsControllerCompat;

import io.agora.ng_api.R;
import io.agora.ng_api.util.ExampleUtil;

public abstract class BaseActivity extends AppCompatActivity {
    private AlertDialog mLoadingDialog;
    public boolean isNightMode;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        isNightMode = ExampleUtil.isNightMode();
        setupTheme();
    }

    public void showLoadingDialog(boolean cancelable) {
        if (mLoadingDialog == null) {
            mLoadingDialog = new AlertDialog.Builder(this).create();
            mLoadingDialog.getWindow().getDecorView().setBackgroundColor(Color.TRANSPARENT);


            int paddingMedium = (int) getResources().getDimension(R.dimen.space_medium);

            FrameLayout container = new FrameLayout(this);
            CardView cardView = new CardView(this);
            cardView.setCardBackgroundColor(ExampleUtil.getColorInt(this,R.attr.colorSurface));
            cardView.setContentPadding(paddingMedium,paddingMedium,paddingMedium,paddingMedium);

            LayoutParams lp = new FrameLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
            lp.gravity = Gravity.CENTER;
            cardView.setLayoutParams(lp);

            ProgressBar progressBar = new ProgressBar(this);
            progressBar.setIndeterminate(true);
            progressBar.setLayoutParams(new FrameLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

            cardView.addView(progressBar);
            container.addView(cardView);
            mLoadingDialog.setView(container);
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
     */
    private void setupTheme() {
        WindowInsetsControllerCompat wic = new WindowInsetsControllerCompat(getWindow(), getWindow().getDecorView());

        wic.setAppearanceLightStatusBars(!isNightMode);
        wic.setAppearanceLightNavigationBars(!isNightMode);

    }

    /**
     * For not recreate Activity
     * We must change color manually
     */
    protected abstract void onUIModeChanges();

}
