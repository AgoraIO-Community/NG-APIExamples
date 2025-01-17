package io.agora.ng_api.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.RippleDrawable;
import android.os.Build;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;

import androidx.annotation.ColorInt;
import androidx.annotation.IntDef;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import io.agora.ng_api.R;
import io.agora.ng_api.util.ExampleUtil;

/**
 * @author lq
 * Just a simple arrow to indicator current status.
 *
 * eg: when {@link this#gravity == Gravity.BOTTOM}
 * Show ⬆ when {@link this#currentFraction == -1f}
 * Show ⬇ when {@link this#currentFraction == 1f}
 */
public class IndicatorView extends View {

    public void setCurrentFraction(float currentFraction) {
        this.currentFraction = currentFraction;
        invalidate();
    }

    private float currentFraction = 0f;

    @GravityFlag
    public int gravity = Gravity.BOTTOM;

    public boolean showIndicator = true;

    private final Paint indicatorPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private Point[] indicatorKeyPoint;
    private final Path indicatorPath = new Path();

    public IndicatorView(Context context) {
        this(context, null);
    }

    public IndicatorView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public IndicatorView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public IndicatorView(Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        setMinimumWidth((int)dp2px(24));
        setMinimumHeight(getMinimumWidth());

        indicatorPaint.setColor(ContextCompat.getColor(getContext(),R.color.agora_blue));
        indicatorPaint.setStyle(Paint.Style.STROKE);
        indicatorPaint.setStrokeCap(Paint.Cap.ROUND);
        indicatorPaint.setStrokeJoin(Paint.Join.ROUND);

//        int bgdId = ExampleUtil.getAttrResId(getContext(), R.attr.selectableItemBackgroundBorderless);
//        Drawable bgd = ContextCompat.getDrawable(getContext(),bgdId);
//        setBackground(bgd);
    }


    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        setIndicatorSize();
    }

    @Override
    protected int getSuggestedMinimumWidth() {
        return getMinimumWidth();
    }

    @Override
    protected int getSuggestedMinimumHeight() {
        return getMinimumHeight();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (showIndicator) {
            indicatorPath.reset();
            updatePath();
            canvas.drawPath(indicatorPath, indicatorPaint);
        }
    }


    /**
     * Step 1: We take MeasuredHeight/MeasuredWidth as indicatorSize according to {@link this#gravity}.
     * Step 2: Update {@link this#indicatorKeyPoint} based on the size.
     * Step 3: Offset the point to viewport center.
     */
    private void setIndicatorSize() {
        int indicatorSize;
        if (gravity == Gravity.TOP || gravity == Gravity.BOTTOM) {
            indicatorSize = getMeasuredHeight();
            indicatorKeyPoint = new Point[]{new Point(indicatorSize / 12, indicatorSize / 2),
                    new Point(indicatorSize / 2, indicatorSize / 2), new Point(indicatorSize - indicatorSize / 12, indicatorSize / 2)};
            for (Point point : indicatorKeyPoint) {
                point.offset((getMeasuredWidth() - indicatorSize)/2,0);
            }
        } else {
            indicatorSize = getMeasuredWidth();
            indicatorKeyPoint = new Point[]{new Point(indicatorSize / 2, indicatorSize / 12),
                    new Point(indicatorSize / 2, indicatorSize / 2), new Point(indicatorSize / 2, indicatorSize - indicatorSize / 12)};
            for (Point point : indicatorKeyPoint) {
                point.offset(0,(getMeasuredHeight() - indicatorSize)/2);
            }
        }
        indicatorPaint.setStrokeWidth(indicatorSize/4f);
    }

    /**
     * Update {@link this#indicatorPath} according to {@link this#currentFraction}
     */
    private void updatePath() {
        float offset = indicatorPaint.getStrokeWidth()/2f * currentFraction;
        if (gravity == Gravity.TOP || gravity == Gravity.BOTTOM) {
            if (gravity == Gravity.TOP) offset = -offset;

            indicatorPath.moveTo(indicatorKeyPoint[0].x, indicatorKeyPoint[0].y - offset);
            indicatorPath.lineTo(indicatorKeyPoint[1].x, indicatorKeyPoint[1].y + offset);
            indicatorPath.lineTo(indicatorKeyPoint[2].x, indicatorKeyPoint[2].y - offset);

        } else {
            if (gravity == Gravity.START)
                offset = -offset;

            indicatorPath.moveTo(indicatorKeyPoint[0].x - offset, indicatorKeyPoint[0].y);
            indicatorPath.lineTo(indicatorKeyPoint[1].x + offset, indicatorKeyPoint[1].y);
            indicatorPath.lineTo(indicatorKeyPoint[2].x - offset, indicatorKeyPoint[2].y);
        }
    }


    private void setIndicatorColor(@ColorInt int color){
        indicatorPaint.setColor(color);
        invalidate();
    }

    private float dp2px(int dp) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics());
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef(value = {Gravity.START, Gravity.TOP, Gravity.END, Gravity.BOTTOM})
    public @interface GravityFlag {
    }
}
