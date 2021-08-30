package io.agora.ng_api.view;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.os.Build;
import android.util.Property;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import androidx.annotation.RequiresApi;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.constraintlayout.widget.Constraints;
import androidx.transition.AutoTransition;
import androidx.transition.Transition;
import androidx.transition.TransitionManager;

class DynamicViewAnimationHelper {
    private DynamicView dynamicView;
    private long durationStage1 = 3000;
    private long durationStage2 = 300;

    private final boolean amIStillWorkingOnIt = true;

    DynamicViewAnimationHelper(DynamicView view) {
        this.dynamicView = view;
    }

    public final Transition flexTransition = new AutoTransition();

    /**
     * remove child for {@link DynamicView#STYLE_LAYOUT_FLEX_GRID}
     *
     * @param index the index of the view to be removed
     */
    public void removeChildInFlexLayout(int index) {
        TransitionManager.beginDelayedTransition(dynamicView, flexTransition);
        dynamicView.removeViewAt(index);
        dynamicView.regroupChildren();
    }

    /**
     * remove child for {@link DynamicView#STYLE_LAYOUT_FLEX_GRID}
     *
     * @param view the view to be removed
     */
    public void removeChildInFlexLayout(View view) {
        TransitionManager.beginDelayedTransition(dynamicView, flexTransition);
        dynamicView.removeView(view);
        dynamicView.regroupChildren();
    }

    /**
     * add child for {@link DynamicView#STYLE_LAYOUT_FLEX_GRID}
     *
     * @param view the view to be added
     */
    public void addChildInFlexLayout(View view) {
        addChildInFlexLayout(view, dynamicView.getChildCount());
    }

    /**
     * add child for {@link DynamicView#STYLE_LAYOUT_FLEX_GRID}
     *
     * @param view  the view to be added
     * @param index the index of the view
     */
    public void addChildInFlexLayout(View view, int index) {
        TransitionManager.beginDelayedTransition(dynamicView, flexTransition);
        dynamicView.addView(view);
        dynamicView.regroupChildren();
    }

    public void switchView(View mainView, View thumbView) {
        if (mainView.getParent() != dynamicView)
            throw new IllegalStateException("mainView should be the one which is directly inside this DynamicView.");
        if (thumbView.getParent() != dynamicView.innerContainer)
            throw new IllegalStateException("thumbView should be a child of innerContainer.");

        if(amIStillWorkingOnIt){
            TransitionManager.beginDelayedTransition(dynamicView, flexTransition);
            dynamicView.doSwitchView(mainView, thumbView);
            return;
        }

        if (dynamicView.getLayoutStyle() == DynamicView.STYLE_LAYOUT_COLLABORATE) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                // config params
                int mW = mainView.getMeasuredWidth();
                int mH = mainView.getMeasuredHeight();
                int index = dynamicView.innerContainer.indexOfChild(thumbView);
                int endTransX = thumbView.getLeft() + dynamicView.scrollContainer.getLeft() - dynamicView.scrollContainer.getScrollX();
                int endTransY = thumbView.getTop() + dynamicView.scrollContainer.getTop();
                endTransX -= ((mW - dynamicView.defaultCardSize)>>1);
                endTransY -= ((mH - dynamicView.defaultCardSize)>>1);

                animateToMainForLayoutC(thumbView, mW, mH, index);
                animateToThumbForLayoutC(mainView, endTransX, endTransY, index);
            } else {
                // 4.3以下
                TransitionManager.beginDelayedTransition(dynamicView, flexTransition);
                dynamicView.doSwitchView(mainView, thumbView);
            }
        } else if (dynamicView.getLayoutStyle() == DynamicView.STYLE_LAYOUT_FIXED_GRID) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                // config params
                int mW = mainView.getMeasuredWidth();
                int mH = mainView.getMeasuredHeight();
                int index = dynamicView.innerContainer.indexOfChild(thumbView);
                int endTransX = thumbView.getLeft() + dynamicView.scrollContainer.getLeft();
                int endTransY = thumbView.getTop() + dynamicView.scrollContainer.getTop() - dynamicView.scrollContainer.getScrollY();
                endTransX -= ((mW - dynamicView.defaultCardSize)>>1);
                endTransY -= ((mH - dynamicView.defaultCardSize)>>1);

                animateToMainForLayoutFixed(thumbView, mW, mH,-endTransX, -endTransY, index);
                animateToThumbForLayoutFixed(mainView, endTransX, endTransY, index);
            } else {
                // 4.3以下
                TransitionManager.beginDelayedTransition(dynamicView, flexTransition);
                dynamicView.doSwitchView(mainView, thumbView);
            }
        }
    }

    /**
     * Retain from innerContainer
     * To dynamicView.overlay
     * Add to dynamicView
     */
    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void animateToMainForLayoutFixed(View thumbView, int mW, int mH, int endTransX, int endTransY, int index) {
        // 添加到 overlay
        dynamicView.getOverlay().add(thumbView);
        // 原布局占位
        View v = dynamicView.innerContainer.getChildAt(index);
        if (v != null) ((LinearLayout.LayoutParams) v.getLayoutParams()).topMargin = dynamicView.defaultCardSize;

        ObjectAnimator oaW = ObjectAnimator.ofInt(thumbView, DynamicViewAnimationHelper.WIDTH, mW);
        ObjectAnimator oaH = ObjectAnimator.ofInt(thumbView, DynamicViewAnimationHelper.HEIGHT, mH);

        ObjectAnimator oaTransX = ObjectAnimator.ofFloat(thumbView, View.TRANSLATION_X, endTransX);
        ObjectAnimator oaTransY = ObjectAnimator.ofFloat(thumbView, View.TRANSLATION_Y, endTransY);

        AnimatorSet finalSet = new AnimatorSet();
        finalSet.setDuration(durationStage1);
        finalSet.playTogether(oaW, oaH, oaTransX, oaTransY);
        finalSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {

            }

            @Override
            public void onAnimationEnd(Animator animator) {
                thumbView.setTranslationX(0);
                thumbView.setTranslationY(0);
                thumbView.setLayoutParams(dynamicView.getLpForMainView());
                dynamicView.getOverlay().remove(thumbView);
                dynamicView.addView(thumbView,0);
            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        });
//        finalSet.start();

    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void animateToThumbForLayoutFixed(View main, int endTransX, int endTransY, int index) {
        int defaultCardSize = dynamicView.defaultCardSize;
        // 添加到 overlay
        dynamicView.getOverlay().add(main);

        ObjectAnimator oaW = ObjectAnimator.ofInt(main, DynamicViewAnimationHelper.WIDTH, defaultCardSize);
        ObjectAnimator oaH = ObjectAnimator.ofInt(main, DynamicViewAnimationHelper.HEIGHT, defaultCardSize);
        ObjectAnimator oaTransX = ObjectAnimator.ofFloat(main, View.TRANSLATION_X, endTransX);
        ObjectAnimator oaTransY = ObjectAnimator.ofFloat(main, View.TRANSLATION_Y, endTransY);

        AnimatorSet finalSet = new AnimatorSet();
        finalSet.setDuration(durationStage1);
        finalSet.playTogether(oaW, oaH, oaTransX, oaTransY);
        finalSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {

            }

            @Override
            public void onAnimationEnd(Animator animator) {
                main.setTranslationX(0);
                main.setTranslationY(0);
                main.setLayoutParams(dynamicView.getLpForThumbView());

                View v = dynamicView.innerContainer.getChildAt(index);
                if(v!=null) ((LinearLayout.LayoutParams)v.getLayoutParams()).topMargin = 0;
                dynamicView.getOverlay().remove(main);
                dynamicView.innerContainer.addView(main,index);
            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        });
        finalSet.start();

    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void animateToMainForLayoutC(View thumbView, int mW, int mH, int index) {
        int defaultCardSize = dynamicView.defaultCardSize;
        int imgStart = (mW - defaultCardSize) >> 1;
        int imgTop = (mH - defaultCardSize) >> 1;

        // scrollView 声明
        FrameLayout s = dynamicView.scrollContainer;
        // rawX
        int diffX = thumbView.getLeft() - s.getScrollX();
        // 需要移动的translationX
        diffX = (s.getMeasuredWidth() - defaultCardSize) / 2 - diffX;
        final int finalDiffX = diffX;

        // 从原布局移除
        ((ViewGroup) thumbView.getParent()).removeView(thumbView);
        // 原布局占位
        View v = dynamicView.innerContainer.getChildAt(index);
        if (v != null) ((LinearLayout.LayoutParams) v.getLayoutParams()).leftMargin = defaultCardSize;

        // 添加到 overlay
        s.getOverlay().add(thumbView);

        thumbView.setPivotY(0f);
        ObjectAnimator oa1ScaleX = ObjectAnimator.ofFloat(thumbView, View.SCALE_X, 0.5f);
        ObjectAnimator oa1ScaleY = ObjectAnimator.ofFloat(thumbView, View.SCALE_Y, 0.5f);

        ObjectAnimator oa2TransX = ObjectAnimator.ofFloat(thumbView, View.TRANSLATION_X, diffX);

        // stage1
        AnimatorSet stage1 = new AnimatorSet();
        stage1.playTogether(oa1ScaleX, oa1ScaleY, oa2TransX);
        stage1.setInterpolator(new AccelerateInterpolator());


        ObjectAnimator oa3ScaleX = ObjectAnimator.ofFloat(thumbView, View.SCALE_X, 0.3f);
        ObjectAnimator oa3ScaleY = ObjectAnimator.ofFloat(thumbView, View.SCALE_Y, 0.3f);
        ObjectAnimator oa4TransY = ObjectAnimator.ofFloat(thumbView, View.TRANSLATION_Y, s.getTop() - defaultCardSize / 2f);

        // stage2
        AnimatorSet stage2 = new AnimatorSet();
        stage2.playTogether(oa3ScaleX, oa3ScaleY, oa4TransY);
        stage2.setInterpolator(new DecelerateInterpolator());


        ObjectAnimator oa5ScaleX = ObjectAnimator.ofFloat(thumbView, View.SCALE_X, 1f);
        ObjectAnimator oa5ScaleY = ObjectAnimator.ofFloat(thumbView, View.SCALE_Y, 1f);
        ObjectAnimator oa6TransY = ObjectAnimator.ofFloat(thumbView, View.TRANSLATION_Y, imgTop);
        ObjectAnimator oa7W = ObjectAnimator.ofInt(thumbView, DynamicViewAnimationHelper.WIDTH, mW);
        ObjectAnimator oa7H = ObjectAnimator.ofInt(thumbView, DynamicViewAnimationHelper.HEIGHT, mH);

        // stage3
        AnimatorSet stage3 = new AnimatorSet();
        stage3.playTogether(oa5ScaleX, oa5ScaleY, oa6TransY, oa7W, oa7H);
        stage2.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {
            }

            @Override
            public void onAnimationEnd(Animator animator) {
                // 从overlay移除
                thumbView.setTranslationX((s.getMeasuredWidth() - defaultCardSize) / 2f + s.getLeft());
                dynamicView.getOverlay().remove(thumbView);
                dynamicView.addView(thumbView,0);

            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        });
        stage1.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {

            }

            @Override
            public void onAnimationEnd(Animator animator) {
                thumbView.setTranslationX(finalDiffX + s.getLeft());
                thumbView.setTranslationY(s.getTop());
                // 从overlay移除
                dynamicView.getOverlay().add(thumbView);
//                s.getOverlay().remove(minor);
//                minor.setTranslationX(s.getLeft());
                // 添加到 this
            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        });

        AnimatorSet finalSet = new AnimatorSet();
        finalSet.setDuration(durationStage1);
        finalSet.playSequentially(stage1, stage2, stage3);
        finalSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {

            }

            @Override
            public void onAnimationEnd(Animator animator) {
                thumbView.setTranslationX(0);
                thumbView.setTranslationY(0);
                thumbView.setLayoutParams(new ConstraintLayout.LayoutParams(Constraints.LayoutParams.MATCH_PARENT, Constraints.LayoutParams.MATCH_PARENT));
            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        });
        finalSet.start();

    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void animateToThumbForLayoutC(View main, int endTransX, int endTransY, int index) {
        int defaultCardSize = dynamicView.defaultCardSize;

/////////////////////////////// STAGE  1 //////////////////////////////////////////////////
        ObjectAnimator oa1W = ObjectAnimator.ofInt(main, DynamicViewAnimationHelper.WIDTH, (int) (defaultCardSize * 1.5));
        ObjectAnimator oa1H = ObjectAnimator.ofInt(main, DynamicViewAnimationHelper.HEIGHT, (int) (defaultCardSize * 1.5));
        ObjectAnimator oa2TransY = ObjectAnimator.ofFloat(main, View.TRANSLATION_Y, defaultCardSize);

        AnimatorSet stage1 = new AnimatorSet();
        stage1.playTogether(oa1W, oa1H, oa2TransY);
        stage1.setInterpolator(new AccelerateInterpolator());
        stage1.setDuration(durationStage1);
        stage1.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                dynamicView.getOverlay().add(main);
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });

/////////////////////////////// STAGE  2 //////////////////////////////////////////////////
        ObjectAnimator oa3W = ObjectAnimator.ofInt(main, DynamicViewAnimationHelper.WIDTH, defaultCardSize);
        ObjectAnimator oa3H = ObjectAnimator.ofInt(main, DynamicViewAnimationHelper.HEIGHT, defaultCardSize);
        ObjectAnimator oa4TransX = ObjectAnimator.ofFloat(main, View.TRANSLATION_X, endTransX);
        ObjectAnimator oa4TransY = ObjectAnimator.ofFloat(main, View.TRANSLATION_Y, endTransY);

        AnimatorSet stage2 = new AnimatorSet();
        stage2.playTogether(oa3W, oa3H, oa4TransX, oa4TransY);
        stage2.setInterpolator(new DecelerateInterpolator());


        AnimatorSet finalSet = new AnimatorSet();
        finalSet.playSequentially(stage1, stage2);
        finalSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animator) {

            }

            @Override
            public void onAnimationEnd(Animator animator) {
                dynamicView.getOverlay().remove(main);
                main.setTranslationX(0);
                main.setTranslationY(0);
                main.setLayoutParams(new LinearLayout.LayoutParams(defaultCardSize, defaultCardSize));
                View view = dynamicView.innerContainer.getChildAt(index);
                if (view != null) ((LinearLayout.LayoutParams) view.getLayoutParams()).leftMargin = 0;
                dynamicView.innerContainer.addView(main, index);
            }

            @Override
            public void onAnimationCancel(Animator animator) {

            }

            @Override
            public void onAnimationRepeat(Animator animator) {

            }
        });
        finalSet.start();

    }

    public static Property<View, Integer> WIDTH = new Property<View, Integer>(Integer.class, "width") {
        @Override
        public Integer get(View view) {
            return view.getWidth();
        }

        @Override
        public void set(View v, Integer value) {
//            v.getLayoutParams().width = value;
//            v.requestLayout();
            int mid = v.getMeasuredWidth() / 2;
            v.setLeft(mid - value / 2);
            v.setRight(v.getLeft() + value);
        }
    };
    public static Property<View, Integer> HEIGHT = new Property<View, Integer>(Integer.class, "height") {
        @Override
        public Integer get(View view) {
            return view.getHeight();
        }

        @Override
        public void set(View v, Integer value) {
//            v.getLayoutParams().height = value;
//            v.requestLayout();
            int mid = v.getMeasuredHeight() / 2;
            v.setTop(mid - value / 2);
            v.setBottom(v.getTop() + value);
        }
    };
}
