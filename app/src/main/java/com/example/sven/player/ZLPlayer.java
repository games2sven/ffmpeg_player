package com.example.sven.player;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ZLPlayer implements SurfaceHolder.Callback {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private SurfaceHolder holder;
    private String dataSource;
    private OnPrepareListener listener;


    /**
     * 让使用 设置播放的文件 或者 直播地址
     */
    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 设置播放显示的画布
     *
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    public void onError(int errorCode){
        System.out.println("Java接到回调:"+errorCode);
    }


    public void onPrepare(){
        if (null != listener){
            listener.onPrepare();
        }
    }

    public void setOnPrepareListener(OnPrepareListener listener){
        this.listener = listener;
    }
    public interface OnPrepareListener{
        void onPrepare();
    }

    /**
     * 准备好 要播放的视频
     */
    public void prepare() {
        native_prepare(dataSource);
    }

    /**
     * 开始播放
     */
    public void start() {
        native_start();
    }

    /**
     * 停止播放
     */
    public void stop() {
        native_stop();
    }

    public void release() {
        holder.removeCallback(this);
        native_release();
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        native_setSurface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    native void native_prepare(String dataSource);
    native void native_start();
    native void native_stop();
    native void native_release();
    native void native_setSurface(Surface surface);
}
