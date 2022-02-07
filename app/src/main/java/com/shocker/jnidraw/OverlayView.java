package com.shocker.jnidraw;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.view.View;

class OverlayView extends View {
    public static native void Draw(OverlayView overlayView, Canvas canvas);
    private native void NativeInit();
    private native void SetWidthScale(int width);
    private native void SetHeightScale(int height);

    Paint mStrokePaint;


    public OverlayView(Context context, int widthPixels, int heightPixels) {
        super(context);
        //初始化Paint
        Init();
        //jni层初始化
        NativeInit();
        //传入当前宽高,在jni里设置出缩放比例
        SetWidthScale(widthPixels);
        SetHeightScale(heightPixels);
    }


    @Override
    protected void onDraw(Canvas canvas) {
//        super.onDraw(canvas);
//        DrawRect(canvas,255, 255, 255, 255,1,80,80,550,600);
        Draw(this,canvas);
        invalidate();
    }

    public void DrawRect(Canvas cvs, int a, int r, int g, int b, float stroke, float x, float y, float width, float height) {
        mStrokePaint.setStrokeWidth(stroke);
        mStrokePaint.setColor(Color.rgb(r, g, b));
        mStrokePaint.setAlpha(a);
        cvs.drawRect(x, y, width, height, mStrokePaint);
    }


    public void Init() {
        mStrokePaint = new Paint();
        mStrokePaint.setStyle(Paint.Style.STROKE);
        mStrokePaint.setAntiAlias(true);
        mStrokePaint.setColor(Color.rgb(0, 0, 0));
    }
}
