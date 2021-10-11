package io.agora.ng_api.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.text.TextPaint;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import io.agora.ng_api.R;
import io.agora.ng_api.bean.DemoInfo;
import io.agora.ng_api.util.ExampleUtil;

import java.util.List;

public class PinnedTitleDecoration extends RecyclerView.ItemDecoration {
    private final List<DemoInfo> demoInfoList;
    private final Paint textPaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private final Paint bgdPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint dividerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final int pinnedViewHeight;
    private final int offsetText;

    public PinnedTitleDecoration(Context context, List<DemoInfo> demoInfoList) {
        updatePaintColor(context);
        pinnedViewHeight = (int) ExampleUtil.dp2px(48);
        this.demoInfoList = demoInfoList;
        textPaint.setFakeBoldText(true);
        textPaint.setTextSize(ExampleUtil.sp2px(20));
        offsetText = (int) ((-textPaint.descent() - textPaint.ascent())/2);
    }

    @Override
    public void getItemOffsets(@NonNull Rect outRect, @NonNull View view, @NonNull RecyclerView parent, @NonNull RecyclerView.State state) {
        super.getItemOffsets(outRect, view, parent, state);
        int pos = parent.getChildAdapterPosition(view);
        // 该类别第一个
        if (pos == 0 || demoInfoList.get(pos).getType() != demoInfoList.get(pos - 1).getType()) {
            outRect.top = pinnedViewHeight;
        } else {
            outRect.top = 1;
        }
    }

    @Override
    public void onDrawOver(@NonNull Canvas c, @NonNull RecyclerView parent, @NonNull RecyclerView.State state) {
        super.onDrawOver(c, parent, state);
        for (int i = 0; i < parent.getChildCount(); i++) {
            View view = parent.getChildAt(i);
            int pos = parent.getChildAdapterPosition(view);
            int type = demoInfoList.get(pos).getType();
            // 一般情况（）
            if (view.getTop() >= pinnedViewHeight) {
                // 第一个
                if (pos == 0 || type != demoInfoList.get(pos - 1).getType()) {
                    drawTitle(parent.getContext(), c, view, view.getTop() - pinnedViewHeight, type,false, false);
                } else { // 分割线
                    c.drawRect(view.getPaddingLeft(), view.getTop() - 1, view.getRight(), view.getTop(), dividerPaint);
                }
            } else {
                // 最后一个
                if (pos + 1 < demoInfoList.size() && type != demoInfoList.get(pos + 1).getType()) {
                    int stickHeadBottom = Math.min(view.getBottom(), pinnedViewHeight);
                    boolean showTopLine = stickHeadBottom < pinnedViewHeight;
                    drawTitle(parent.getContext(), c, view, stickHeadBottom - pinnedViewHeight, type,showTopLine,false);
                } else {    // 置顶
                    drawTitle(parent.getContext(), c, view, 0, type,false, true);
                }

            }
        }
    }

    private void drawTitle(Context context, Canvas c, View view, int top, int type, boolean showTopLine, boolean showBottomLine) {
        String title;
        if (type == DemoInfo.TYPE_BASIC) title = context.getString(DemoInfo.STRING_BASIC);
        else title = context.getString(DemoInfo.STRING_ADVANCED);

        c.drawRect(view.getLeft(), top, view.getRight(), top + pinnedViewHeight, bgdPaint);
        if(showTopLine)
            c.drawRect(view.getLeft(), 0, view.getRight(), 2, dividerPaint);
        if(showBottomLine)
            c.drawRect(view.getLeft(), top+pinnedViewHeight, view.getRight(), 2+pinnedViewHeight+top, dividerPaint);

        c.drawText(title, view.getPaddingLeft(), top + pinnedViewHeight / 2f + offsetText, textPaint);
    }

    public void updatePaintColor(Context context){
        textPaint.setColor(ExampleUtil.getColorInt(context, R.attr.colorOnBackground));
        bgdPaint.setColor(ExampleUtil.getColorInt(context, android.R.attr.colorBackground));
        dividerPaint.setColor(ExampleUtil.getColorInt(context, R.attr.colorSurface));
    }
}
