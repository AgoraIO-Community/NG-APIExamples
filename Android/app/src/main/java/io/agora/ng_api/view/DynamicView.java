package io.agora.ng_api.view;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Build;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.*;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.constraintlayout.widget.ConstraintSet;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import com.google.android.material.card.MaterialCardView;
import io.agora.ng_api.R;
import io.agora.ng_api.util.ExampleUtil;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @author liuqiang
 * 可自定义一行多少个 View
 * 默认Flex模式
 * 所有子View宽高1：1
 * 需要定制可重写 {@link this#getStepByChildCount(int)}
 */
public class DynamicView extends ConstraintLayout {
    public static final int STYLE_LAYOUT_FLEX_GRID = 0;
    public static final int STYLE_LAYOUT_COLLABORATE = 1;
    public static final int STYLE_LAYOUT_FIXED_GRID = 2;


    public FrameLayout scrollContainer;
    public LinearLayout innerContainer;

    public int defaultCardSize;
    private int layoutStyle;

    ///////////////////////////////////// ANIMATION /////////////////////////////////////////////////////////////////
    // TODO try my best to finish the animation part
    private boolean enableInsideAnimation = false;
    public boolean enableDefaultClickListener = true;
    private DynamicViewAnimationHelper helper;

    private final int defStyleAttr;
    private final int defStyleRes;

    public DynamicView(@NonNull Context context) {
        this(context, null);
    }

    public DynamicView(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DynamicView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public DynamicView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        this.defStyleAttr = defStyleAttr;
        this.defStyleRes = defStyleRes;


        TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.DynamicView);
        layoutStyle = ta.getInt(R.styleable.DynamicView_layoutStyle_dynamic, DynamicView.STYLE_LAYOUT_FLEX_GRID);
        ta.recycle();

        defaultCardSize = (int) dp2px(120);
        // init childView
        setupViewWithStyle();


        if (isInEditMode()) enableInsideAnimation = false;
        // default LayoutTransition do not match our needs
        // addView/removeView will cause bound change before view start animate
        if (enableInsideAnimation) {
            helper = new DynamicViewAnimationHelper(this);
            setLayoutTransition(null);
        }

        // for the preview
        if (this.isInEditMode()) {
            int[] colors = new int[]{Color.RED, Color.GREEN, Color.BLUE};
            View view;
            for (int i = 0; i < 3; i++) {
                view = createDemoLayout(FrameLayout.class);
                view.setBackgroundColor(colors[i]);
                demoAddView(view);
            }
        }
    }


    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if(this.scrollContainer != null)
            this.scrollContainer.setBackgroundColor(ExampleUtil.getColorInt(getContext(),R.attr.colorSurface));
    }

    /**
     * Handle stuff each time layoutStyle changed.
     * 1. Gather all childView added to this.
     * 2. Recreate all container based on current layoutStyle
     * 3. Restore all view.
     */
    private void setupViewWithStyle() {
        // Step 1
        List<View> children = resetView();

        // Step 2
        innerContainer = initInnerContainer();
        scrollContainer = initScrollContainer();

        // FLEX situation ==> Step 3
        if (layoutStyle == DynamicView.STYLE_LAYOUT_FLEX_GRID) {
            for (View child : children) {
                configViewIfIsSurfaceView(child, false);
                if (enableDefaultClickListener)
                    child.setOnClickListener(null);
                this.addView(child, getLpForMainView());
            }
            regroupChildren();
            children.clear();
            return;
        }
        // other situation ==> Step 3

        // For the first View we add it to 'this'
        if (children.size() > 0) {
            View v = children.get(0);
            configViewIfIsSurfaceView(v, false);
            if (enableDefaultClickListener)
                v.setOnClickListener(null);
            this.addView(v, getLpForMainView());
            children.remove(v);
        }

        // For the rest View we try to add it to innerContainer
        if (innerContainer != null) {
            if (scrollContainer != null) {
                scrollContainer.addView(innerContainer);
                addView(scrollContainer);
                for (View child : children) {
                    configViewIfIsSurfaceView(child, true);
                    if (enableDefaultClickListener)
                        child.setOnClickListener(this::switchView);
                    innerContainer.addView(child, getLpForThumbView());
                }
            }
        }
        children.clear();
    }

    /**
     * if there is a SurfaceView inside a child
     * setZOrderMediaOverlay to it
     *
     * @param child the view you want to config
     */
    private void configViewIfIsSurfaceView(View child, boolean zOrderMediaOverlay) {
        // for SurfaceView
        SurfaceView surfaceView = null;
        if (child instanceof ViewGroup) {
            ViewGroup viewGroup = ((ViewGroup) child);
            for (int i = 0; i < viewGroup.getChildCount(); i++) {
                if (viewGroup.getChildAt(i) instanceof SurfaceView) {
                    surfaceView = (SurfaceView) viewGroup.getChildAt(i);
                    break;
                }
            }
        } else if (child instanceof SurfaceView) {
            surfaceView = (SurfaceView) child;
        }
        if (surfaceView != null)
            surfaceView.setZOrderMediaOverlay(zOrderMediaOverlay);
    }

    private List<View> resetView() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
            getOverlay().clear();
        List<View> children = new ArrayList<>();
        if (innerContainer == null) {
            for (int i = 0; i < getChildCount(); i++)
                children.add(this.getChildAt(i));
        } else {
            children.add(this.getChildAt(0));
            for (int i = 0; i < innerContainer.getChildCount(); i++)
                children.add(innerContainer.getChildAt(i));
            innerContainer.removeAllViews();
        }
        removeAllViews();
        return children;
    }

    private LinearLayout initInnerContainer() {
        if (layoutStyle == STYLE_LAYOUT_FLEX_GRID) return null;
        LinearLayout linearLayout;
        // setup linearLayout
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
            linearLayout = new LinearLayout(getContext(), null, defStyleAttr, defStyleRes);
        else linearLayout = new LinearLayout(getContext(), null, defStyleAttr);

        FrameLayout.LayoutParams lp;
        if (layoutStyle == STYLE_LAYOUT_COLLABORATE) {
            linearLayout.setOrientation(LinearLayout.HORIZONTAL);
            lp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT);
        } else if (layoutStyle == STYLE_LAYOUT_FIXED_GRID) {
            linearLayout.setOrientation(LinearLayout.VERTICAL);
            lp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        } else {
            lp = null;
        }

        linearLayout.setLayoutParams(lp);
        return linearLayout;
    }


    private FrameLayout initScrollContainer() {
        if (layoutStyle == STYLE_LAYOUT_FLEX_GRID) return null;
        FrameLayout scrollView;
        LayoutParams lp;
        if (layoutStyle == DynamicView.STYLE_LAYOUT_COLLABORATE) {
            // config lp
            lp = new LayoutParams(LayoutParams.MATCH_PARENT, defaultCardSize);
            lp.topToTop = ConstraintSet.PARENT_ID;
            // new scrollView
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                scrollView = new HorizontalScrollView(getContext(), null, defStyleAttr, defStyleRes);
            } else {
                scrollView = new HorizontalScrollView(getContext(), null, defStyleAttr);
            }
            lp.setMargins((int) dp2px(16), (int) dp2px(12), (int) dp2px(16), 0);
            ((HorizontalScrollView) scrollView).setFillViewport(true);
        } else if (layoutStyle == DynamicView.STYLE_LAYOUT_FIXED_GRID) {
            // config lp
            lp = new LayoutParams(defaultCardSize, LayoutParams.MATCH_PARENT);
            lp.topToTop = ConstraintSet.PARENT_ID;
            lp.endToEnd = ConstraintSet.PARENT_ID;
            lp.rightToRight = ConstraintSet.PARENT_ID;

            // new scrollView
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                scrollView = new ScrollView(getContext(), null, defStyleAttr, defStyleRes);
            } else {
                scrollView = new ScrollView(getContext(), null, defStyleAttr);
            }
            ((ScrollView) scrollView).setFillViewport(true);
        } else return null;

        scrollView.setLayoutParams(lp);
        scrollView.setId(ViewCompat.generateViewId());

        TypedValue typedValue = new TypedValue();
        getContext().getTheme().resolveAttribute(R.attr.colorSurface, typedValue, true);
        int surface = ContextCompat.getColor(getContext(), typedValue.resourceId);

        scrollView.setBackgroundColor(surface);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) scrollView.setElevation(dp2px(2));


        return scrollView;
    }

    public void demoAddView(View child) {
        demoAddView(child, false);
    }

    public void demoAddView(View child, boolean insideInnerContainer) {
        if (layoutStyle == DynamicView.STYLE_LAYOUT_FLEX_GRID) {
            if (insideInnerContainer)
                throw new IllegalStateException("layoutStyle STYLE_LAYOUT_FLEX_GRID do not allow the ScrollContainer exist.");
            if (enableInsideAnimation)
                helper.addChildInFlexLayout(child);
            else {
                addView(child);
                regroupChildren();
            }
        } else if (insideInnerContainer) {
            if (enableDefaultClickListener)
                throw new IllegalStateException("DynamicView cannot handle this situation.");
            else innerContainer.addView(child);
        } else if (getChildCount() == 1)
            addView(child, 0);
        else innerContainer.addView(child);
    }

    public void demoAddView() {
        if (layoutStyle == DynamicView.STYLE_LAYOUT_FLEX_GRID) {
            if (enableInsideAnimation)
                helper.addChildInFlexLayout(createDefaultVideoView());
            else {
                addView(createDefaultVideoView());
                regroupChildren();
            }
        } else if (getChildCount() == 1)
            addView(createDefaultVideoView(), 0);
        else innerContainer.addView(createDefaultVideoView());
    }

    public void demoRemoveView(View view) {
        if (layoutStyle == DynamicView.STYLE_LAYOUT_FLEX_GRID) {
            if (enableInsideAnimation)
                helper.removeChildInFlexLayout(view);
            else {
                removeView(view);
                regroupChildren();
            }
        } else removeView(view);
    }

    public void switchView(View thumbView) {
        switchView(getChildAt(0), thumbView);
    }

    public void switchView(View mainView, View thumbView) {
        if (mainView.getParent() != this)
            throw new IllegalStateException("mainView should be the one which is directly inside this DynamicView.");
        if (thumbView.getParent() != innerContainer)
            throw new IllegalStateException("thumbView should be a child of innerContainer.");
        if (this.enableDefaultClickListener) {
            thumbView.setOnClickListener(null);
            mainView.setOnClickListener(this::switchView);
        }
        configViewIfIsSurfaceView(mainView, true);
        configViewIfIsSurfaceView(thumbView, false);

        // enable custom Animation
        if (enableInsideAnimation) {
            helper.switchView(mainView, thumbView);
        } else {
            doSwitchView(mainView, thumbView);
        }
    }

    protected void doSwitchView(View mainView, View thumbView) {
        int indexOfMain = this.indexOfChild(mainView);
        this.removeView(mainView);

        int indexOfThumb = innerContainer.indexOfChild(thumbView);
        innerContainer.removeView(thumbView);

        this.addView(thumbView, indexOfMain, getLpForMainView());
        innerContainer.addView(mainView, indexOfThumb, getLpForThumbView());
    }

    public View createDefaultVideoView() {
        ImageView iv = createDemoLayout(ImageView.class);
        iv.setBackgroundColor(Color.rgb(new Random().nextInt(255), new Random().nextInt(255), new Random().nextInt(255)));
        return iv;
    }

    public View createDefaultAudioView() {
        ImageView iv = createDemoLayout(ImageView.class);
        iv.setImageResource(R.mipmap.ic_launcher);
        return iv;
    }


    public <T extends View> T createDemoLayout(Class<T> tClass) {
        boolean forContainer = getChildCount() > 1 && layoutStyle != DynamicView.STYLE_LAYOUT_FLEX_GRID;
        return createDemoLayout(tClass, forContainer);
    }

    /**
     * base lp config
     * <p>
     *
     * @param tClass class of view
     * @return the view going to be ADDED
     */
    public <T extends View> T createDemoLayout(Class<T> tClass, boolean forContainer) {
        T view;
        try {
            view = tClass.getConstructor(Context.class).newInstance(getContext());
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        view.setId(ViewCompat.generateViewId());
        view.setSaveEnabled(true);

        if (this.enableDefaultClickListener) {
            if (forContainer) view.setOnClickListener(this::switchView);
        }

        if (forContainer) {
            view.setLayoutParams(getLpForThumbView());
        } else {
            view.setLayoutParams(getLpForMainView());
        }

        return view;
    }

    public LinearLayout.LayoutParams getLpForThumbView() {
        LinearLayout.LayoutParams lp;
        switch (this.layoutStyle) {
            case DynamicView.STYLE_LAYOUT_COLLABORATE:
                lp = new LinearLayout.LayoutParams(defaultCardSize, LinearLayout.LayoutParams.MATCH_PARENT);
                break;
            case DynamicView.STYLE_LAYOUT_FIXED_GRID:
                lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, defaultCardSize);
                break;
            default:
                lp = null;
        }
        return lp;
    }

    public LayoutParams getLpForMainView() {
        ConstraintLayout.LayoutParams lp = new ConstraintLayout.LayoutParams(0, 0);
        switch (this.layoutStyle) {
            case DynamicView.STYLE_LAYOUT_FLEX_GRID:
                lp.dimensionRatio = "1:1";
                break;
            case DynamicView.STYLE_LAYOUT_COLLABORATE:
                lp.leftToLeft = ConstraintSet.PARENT_ID;
                lp.startToStart = ConstraintSet.PARENT_ID;

                lp.topToTop = ConstraintSet.PARENT_ID;

                lp.rightToRight = ConstraintSet.PARENT_ID;
                lp.endToEnd = ConstraintSet.PARENT_ID;

                lp.bottomToBottom = ConstraintSet.PARENT_ID;
                break;
            default:
                lp.leftToLeft = ConstraintSet.PARENT_ID;
                lp.startToStart = ConstraintSet.PARENT_ID;

                lp.topToTop = ConstraintSet.PARENT_ID;

                lp.rightToLeft = scrollContainer.getId();
                lp.endToStart = scrollContainer.getId();

                lp.bottomToBottom = ConstraintSet.PARENT_ID;
        }
        return lp;
    }

    /**
     * 重新组织子 View 的关联
     */
    protected void regroupChildren() {
        int childCount = this.getChildCount();
        if (childCount == 0) return;

        View currentView;
        int step = getStepByChildCount(childCount);
        for (int i = 0; i < childCount; i++) {
            currentView = this.getChildAt(i);
            configViewForFlexStyle(i, step, currentView);
        }
    }

    /**
     * step = 2
     * 0 == 1
     * 2 == 3
     * <p>
     * step = 3
     * 0 == 1 == 2
     * 3 == 4 == 5
     *
     * @param index view 在父View中的下标
     * @param step  步长
     */
    private void configViewForFlexStyle(int index, int step, View v) {
        ConstraintLayout.LayoutParams lp = (ConstraintLayout.LayoutParams) v.getLayoutParams();
        // 第一个
        if (index % step == 0) {
            lp.leftToLeft = ConstraintSet.PARENT_ID;
            lp.leftToRight = ConstraintSet.UNSET;
            if (step == 1) {
                lp.rightToLeft = ConstraintSet.UNSET;
                lp.rightToRight = ConstraintSet.PARENT_ID;
            } else {
                lp.rightToRight = ConstraintSet.UNSET;
//                其他情况有可能此View为最后一个，需要判断
                if (index + 1 == this.getChildCount()) {
                    lp.rightToLeft = this.getChildAt(index + 1 - step).getId();
                } else {
                    lp.rightToLeft = this.getChildAt(index + 1).getId();
                }
            }
        } else if (index % step == step - 1) { // 最后一个
            lp.leftToLeft = ConstraintSet.UNSET;
            lp.leftToRight = this.getChildAt(index - 1).getId();
            lp.rightToLeft = ConstraintSet.UNSET;
            lp.rightToRight = ConstraintSet.PARENT_ID;
        } else { // 中间
            lp.leftToLeft = ConstraintSet.UNSET;
            lp.leftToRight = this.getChildAt(index - 1).getId();
            lp.rightToRight = ConstraintSet.UNSET;

//          其他情况有可能此View为最后一个，需要判断
            if (index + 1 == this.getChildCount()) {
                lp.rightToLeft = this.getChildAt(index + 1 - step).getId();
            } else {
                lp.rightToLeft = this.getChildAt(index + 1).getId();
            }
        }
        // TOP 修正
        if (index - step >= 0) {
            lp.topToBottom = this.getChildAt(index - step).getId();
            lp.topToTop = ConstraintSet.UNSET;
        } else {
            lp.topToTop = ConstraintSet.PARENT_ID;
            lp.topToBottom = ConstraintSet.UNSET;
        }
        // Android 版本兼容
        lp.startToEnd = lp.leftToRight;
        lp.startToStart = lp.leftToLeft;
        lp.endToStart = lp.rightToLeft;
        lp.endToEnd = lp.rightToRight;
        // 赋值
        v.setLayoutParams(lp);
    }

    public int getLayoutStyle() {
        return layoutStyle;
    }

    public void setLayoutStyle(@IntRange(from = DynamicView.STYLE_LAYOUT_FLEX_GRID, to = DynamicView.STYLE_LAYOUT_FIXED_GRID) int newLayoutStyle) {
        this.layoutStyle = newLayoutStyle;
        setupViewWithStyle();
    }

    /**
     * override this method for custom requirement
     *
     * @param childCount 子 view 个数
     * @return step，隔多少个View换行
     */
    public int getStepByChildCount(int childCount) {
        if (childCount < 2)
            return 1;
        else if (childCount < 5)
            return 2;
        else return 3;
    }

    float dp2px(int dp) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics());
    }

}