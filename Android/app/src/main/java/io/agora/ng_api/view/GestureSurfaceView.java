package io.agora.ng_api.view;

import android.content.Context;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SurfaceView;
import android.view.ViewGroup;
import io.agora.ng_api.util.ExampleUtil;

public class GestureSurfaceView extends SurfaceView implements ScaleGestureDetector.OnScaleGestureListener {
    private final ScaleGestureDetector detector;

    public GestureSurfaceView(Context context) {
        super(context);
        detector = new ScaleGestureDetector(context,this);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        detector.onTouchEvent(event);
        return true;
    }

    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        ExampleUtil.utilLog("scale:"+detector.getScaleFactor());
        ViewGroup.LayoutParams lp = getLayoutParams();
        lp.width *= detector.getScaleFactor();
        lp.height *= detector.getScaleFactor();
        setLayoutParams(lp);
        return true;
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        return true;
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {

    }
}
