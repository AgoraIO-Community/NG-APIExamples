package io.agora.ng_api.ui.activity;

import android.content.res.ColorStateList;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.graphics.ColorUtils;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.widget.ImageViewCompat;
import androidx.fragment.app.Fragment;
import androidx.navigation.NavArgument;
import androidx.navigation.NavController;
import androidx.navigation.NavDestination;
import androidx.navigation.Navigation;
import androidx.transition.Transition;
import androidx.transition.TransitionManager;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.transition.MaterialContainerTransform;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseActivity;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.bean.DemoInfo;
import io.agora.ng_api.databinding.ActivityMainBinding;
import io.agora.ng_api.ui.fragment.DescriptionFragment;
import io.agora.ng_api.util.ExampleUtil;

import java.util.ArrayList;
import java.util.List;

public final class MainActivity extends BaseActivity {
    private final List<DemoInfo> demoInfoList = new ArrayList<>();
    private ActivityMainBinding mBinding;
    private NavController.OnDestinationChangedListener onDestinationChangedListener;

    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBinding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(mBinding.getRoot());
        initView();
        initControlView();
    }

    /**
     * OnDestinationChangedListener setup
     * Put this code block inside onCreate cause NPE
     */
    @Override
    protected void onStart() {
        super.onStart();
        getNavController().removeOnDestinationChangedListener(onDestinationChangedListener);
        getNavController().addOnDestinationChangedListener(onDestinationChangedListener);
    }

    private void initView() {
        // FAB and CardView animation
        MaterialContainerTransform transformExpand = ExampleUtil.getTransform(mBinding.tFabMain, mBinding.cardView);
        transformExpand.addListener(new Transition.TransitionListener() {
            @Override
            public void onTransitionStart(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(false);
                mBinding.scrimMain.setVisibility(View.VISIBLE);
                mBinding.scrimMain.animate().alpha(1).start();
            }

            @Override
            public void onTransitionEnd(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(true);
            }

            @Override
            public void onTransitionCancel(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionPause(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionResume(@NonNull Transition transition) {

            }
        });
        MaterialContainerTransform transformClose = ExampleUtil.getTransform(mBinding.cardView, mBinding.tFabMain);
        transformClose.addListener(new Transition.TransitionListener() {
            @Override
            public void onTransitionStart(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(false);
                mBinding.tFabMain.setEnabled(false);
                mBinding.scrimMain.animate().alpha(0).start();
            }

            @Override
            public void onTransitionEnd(@NonNull Transition transition) {
                mBinding.scrimMain.setEnabled(true);
                mBinding.tFabMain.setEnabled(true);
                mBinding.scrimMain.setVisibility(View.GONE);
            }

            @Override
            public void onTransitionCancel(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionPause(@NonNull Transition transition) {

            }

            @Override
            public void onTransitionResume(@NonNull Transition transition) {

            }
        });

        mBinding.tFabMain.setOnClickListener(v -> {
            TransitionManager.beginDelayedTransition(mBinding.getRoot(), transformExpand);
            mBinding.tFabMain.setVisibility(View.GONE);
            mBinding.cardView.setVisibility(View.VISIBLE);
        });
        mBinding.scrimMain.setOnClickListener(v -> {
            TransitionManager.beginDelayedTransition(mBinding.getRoot(), transformClose);
            mBinding.tFabMain.setVisibility(View.VISIBLE);
            mBinding.cardView.setVisibility(View.GONE);
        });


        // Nav back listener
        mBinding.navIconMain.setOnClickListener((v) -> getNavController().popBackStack());

        // Destination change listener
        onDestinationChangedListener = (controller, destination, arguments) -> {
            String title = "";
            // 标题改变
            if (destination.getId() == R.id.descriptionFragment) {
                if (arguments != null) {
                    DemoInfo demoInfo = (DemoInfo) arguments.getSerializable(DemoInfo.key);
                    title = demoInfo.getTitle();
                }
                mBinding.tFabMain.hide();
            } else {
                if (destination.getLabel() != null)
                    title = destination.getLabel().toString();


                if(destination.getId() != R.id.listFragment) {
                    if(arguments != null) {
                        String channelName = arguments.getString(DescriptionFragment.channelName);
                        if(channelName!=null)
                        title += "【"+ channelName+"】";
                    }
                }
                mBinding.tFabMain.show();
            }
            mBinding.textTitleMain.setText(title);

            // 图标改变
            if (destination.getId() == R.id.listFragment) {
                mBinding.navIconMain.setVisibility(View.INVISIBLE);
            } else {
                mBinding.navIconMain.setVisibility(View.VISIBLE);
            }
        };

        // For the ime & navigationBar
        ViewCompat.setOnApplyWindowInsetsListener(getWindow().getDecorView(), (v, insets) -> {
            final int statusBarT = insets.getInsets(WindowInsetsCompat.Type.statusBars()).top;
            Insets navInset = insets.getInsets(WindowInsetsCompat.Type.navigationBars());
            final int navBottom = Math.max(navInset.bottom, insets.getInsets(WindowInsetsCompat.Type.ime()).bottom);
            final int navLeft = navInset.left;
            final int navRight = navInset.right;

            mBinding.getRoot().setPadding(navLeft, statusBarT, navRight, navBottom);
            // TODO find a better way to achieve this.
            ((ConstraintLayout.LayoutParams)mBinding.scrimMain.getLayoutParams()).setMargins(-navLeft, -statusBarT, -navRight, -navBottom);
            // status bar height
//            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
//                mBinding.toolBarMain.setPaddingRelative(0, statusBarT, 0, 0);
//                mBinding.containerNpApi.setPaddingRelative(0, 0, 0, bot);
//            } else {
//                mBinding.toolBarMain.setPadding(0, statusBarT, 0, 0);
//                mBinding.containerNpApi.setPadding(0, 0, 0, bot);
//            }
//            // ime height
//            int spaceMedium = (int) getResources().getDimension(R.dimen.space_medium);
//            ((ConstraintLayout.LayoutParams) mBinding.cardView.getLayoutParams()).bottomMargin = bot + spaceMedium;
//            ((ConstraintLayout.LayoutParams) mBinding.tFabMain.getLayoutParams()).bottomMargin = bot + spaceMedium;
            return insets;
        });
    }

    private void initControlView() {

        // "静音"按钮 点击事件
        mBinding.sectionAudioMain.muteBtnSectionAudio.addOnCheckedChangeListener((v, isChecked) -> {
            if (!v.isPressed()) return;
            BaseDemoFragment<?> f = checkDemoAvailable();
            if (f != null && f.mScene != null && f.mLocalAudioTrack != null) {
                if (isChecked) f.mScene.unpublishLocalAudioTrack(f.mLocalAudioTrack);
                else f.mScene.publishLocalAudioTrack(f.sid, f.mLocalAudioTrack);
            }
        });

        // profile change
        // 音频 profile 改变
        mBinding.sectionAudioMain.typeSwitchGroupSectionAudio.addOnButtonCheckedListener((group, checkedId, isChecked) -> {
            if (!isChecked) return;
            BaseDemoFragment<?> f = checkDemoAvailable();

            if (checkedId == R.id.default_btn_section_audio) {
                ExampleUtil.utilLog("0");
            } else if (checkedId == R.id.gs_btn_section_audio) {
                ExampleUtil.utilLog("1");
            } else {
                ExampleUtil.utilLog("2");
            }
        });

        // adjust publish audio volume
        // 调整发布音频的音量
        mBinding.sectionAudioMain.volumeDownBtnSectionAudio.setOnClickListener(v -> {
            float currentVolume = mBinding.sectionAudioMain.sliderVolumeSectionAudio.getValue();
            float desireValue = Math.max(0f, currentVolume - 1);
            mBinding.sectionAudioMain.sliderVolumeSectionAudio.setValue(desireValue);
        });
        mBinding.sectionAudioMain.volumeUpBtnSectionAudio.setOnClickListener(v -> {
            float currentVolume = mBinding.sectionAudioMain.sliderVolumeSectionAudio.getValue();
            float desireValue = Math.min(100f, currentVolume + 1);
            mBinding.sectionAudioMain.sliderVolumeSectionAudio.setValue(desireValue);
        });
        mBinding.sectionAudioMain.sliderVolumeSectionAudio.addOnChangeListener((slider, value, fromUser) -> {
            BaseDemoFragment<?> f = checkDemoAvailable();
            if (f != null && f.mLocalAudioTrack != null) f.mLocalAudioTrack.adjustPublishVolume((int) value);
        });
    }

    public NavController getNavController() {
        return Navigation.findNavController(mBinding.containerNpApi);
    }

    public void configData() {
        if (!demoInfoList.isEmpty()) return;

//        for (int i = 0; i < 20; i++) {
//            DemoInfo demoInfo = new DemoInfo(i, i / 10, "Title" + i, 1, DemoInfo.STRING_ADVANCED);
//            demoInfoList.add(demoInfo);
//        }
//        if (!demoInfoList.isEmpty()) return;

        NavDestination destination;
        DemoInfo demoInfo;

        String argsType = getString(R.string.args_nav_type);
        String argsDesc = getString(R.string.args_nav_desc);
        String argsPerm = getString(R.string.args_nav_permission);

        for (NavDestination navDestination : getNavController().getGraph()) {
            destination = navDestination;

            if (destination.getArguments().isEmpty()) continue;

            // handle type( must have)
            // 0 , 1
            // {@link DemoInfo.TYPE_BASIC},{@link DemoInfo.TYPE_ADVANCED}
            NavArgument typeArgument = destination.getArguments().get(argsType);
            // No Type, not a demo.
            if (typeArgument == null) continue;

            Integer type;
            type = (Integer) typeArgument.getDefaultValue();
            if (null == type || type < DemoInfo.TYPE_BASIC || type > DemoInfo.TYPE_ADVANCED)
                throw new IllegalArgumentException("type value out of range.");

            // title
            assert destination.getLabel() != null;
            String title = destination.getLabel().toString();

            // destination id
            Integer descInt;
            NavArgument descArgument = destination.getArguments().get(argsDesc);
            if (descArgument == null)
                throw new IllegalArgumentException("fragment argument " + argsDesc + " required.");
            descInt = (Integer) descArgument.getDefaultValue();
            if (null == descInt || descInt <= 0)
                throw new IllegalArgumentException("fragment argument " + argsDesc + " illegal.");

            // permission
            // 0x01 音频
            // 0x02 视频
            Integer permissionFlag;
            NavArgument permissionArgument = destination.getArguments().get(argsPerm);
            if (permissionArgument == null)
                throw new IllegalArgumentException("fragment argument " + argsPerm + " required.");
            else permissionFlag = (Integer) permissionArgument.getDefaultValue();
            if (null == permissionFlag || permissionFlag < 0)
                throw new IllegalArgumentException("fragment argument " + argsPerm + " illegal.");

            // create new DemoInfo
            demoInfo = new DemoInfo(destination.getId(), type, title, permissionFlag, descInt);
            demoInfoList.add(demoInfo);
        }
    }

    public List<DemoInfo> getDemoInfoList() {
        return demoInfoList;
    }

    private @Nullable
    BaseDemoFragment<?> checkDemoAvailable() {
        Fragment fragment = getSupportFragmentManager().getPrimaryNavigationFragment();
        if (fragment != null)
            fragment = fragment.getChildFragmentManager().getPrimaryNavigationFragment();
        if (fragment instanceof BaseDemoFragment) return (BaseDemoFragment<?>) fragment;
        else return null;
    }

    /**
     * 当 FAB 是展开的《==》返回键收起
     */
    @Override
    public void onBackPressed() {
        if (mBinding.cardView.getVisibility() == View.VISIBLE) {
            // 动画结束，调用点击事件；动画过程中，直接忽略这次操作。
            if (mBinding.scrimMain.isEnabled())
                mBinding.scrimMain.performClick();
        } else
            super.onBackPressed();
    }

    @Override
    public void onUIModeChanges() {
        int colorBgd = ExampleUtil.getColorInt(this, android.R.attr.colorBackground);
        int colorScrim = ExampleUtil.getColorInt(this, R.attr.scrimBackground);
        int colorPrimary = ExampleUtil.getColorInt(this, R.attr.colorPrimary);
        int colorOnPrimary = ExampleUtil.getColorInt(this, R.attr.colorOnPrimary);
        int colorSurface = ExampleUtil.getColorInt(this, R.attr.colorSurface);
        int colorOnSurface = ExampleUtil.getColorInt(this, R.attr.colorOnSurface);

        int colorTextPrimary = ExampleUtil.getColorInt(this, android.R.attr.textColorPrimary);
        int colorTextSecondary = ExampleUtil.getColorInt(this, android.R.attr.textColorSecondary);
        // Default disabledTextColor is colorOnSurface with alpha*0.38
        int colorTextDisabled = ColorUtils.setAlphaComponent(colorOnSurface, (int) (255 * 0.38));
        // default StrokeColor is colorOnSurface with alpha*0.12
        int colorStroke = ColorUtils.setAlphaComponent(colorOnSurface, (int) (255 * 0.12));

        // Config Window color
        getWindow().getDecorView().setBackgroundColor(colorBgd);

        // Config main views' color
        ImageViewCompat.setImageTintList(mBinding.navIconMain, ColorStateList.valueOf(colorTextPrimary));
        mBinding.textTitleMain.setTextColor(colorTextPrimary);
        mBinding.scrimMain.setBackgroundColor(colorScrim);
        mBinding.tFabMain.setSupportImageTintList(ColorStateList.valueOf(colorOnPrimary));

        // Config ControlView's color
        mBinding.cardView.setCardBackgroundColor(colorSurface);
        ColorStateList primaryStateList = new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked, android.R.attr.state_enabled}, new int[]{android.R.attr.state_enabled}, new int[]{}}, new int[]{colorPrimary, colorTextSecondary, colorTextDisabled});
        ColorStateList surfaceStateList = new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked}, new int[]{}}, new int[]{colorPrimary, colorStroke});

        ExampleUtil.updateMaterialButtonTint(mBinding.sectionAudioMain.muteBtnSectionAudio, primaryStateList, surfaceStateList);

        for (int i = 0; i < mBinding.sectionAudioMain.typeSwitchGroupSectionAudio.getChildCount(); i++)
            ExampleUtil.updateMaterialButtonTint((MaterialButton) mBinding.sectionAudioMain.typeSwitchGroupSectionAudio.getChildAt(i), primaryStateList, surfaceStateList);
        for (int i = 0; i < mBinding.sectionAudioMain.voiceSwitchGroupSectionAudio.getChildCount(); i++)
            ExampleUtil.updateMaterialButtonTint((MaterialButton) mBinding.sectionAudioMain.voiceSwitchGroupSectionAudio.getChildAt(i), primaryStateList, surfaceStateList);
    }
}
