package io.agora.ng_api.ui.fragment;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.view.SurfaceView;
import android.view.View;
import android.webkit.URLUtil;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentMediaPlayerBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.ng_api.view.VideoView;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.data.AgoraRteMediaPlayerObserver;
import io.agora.rte.media.data.AgoraRteVideoFrame;
import io.agora.rte.media.media_player.*;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.media.video.AgoraRteVideoSubscribeOptions;
import io.agora.rte.scene.AgoraRteConnectionChangedReason;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

import java.util.List;
import java.util.Random;

public class MediaPlayerFragment extends BaseDemoFragment<FragmentMediaPlayerBinding> {
    private AgoraRteMediaPlayer mPlayer;
    private AgoraRteMediaPlayerObserver mPlayerObserver;
    private VideoView mVideoView;

    boolean initVideoView = false;
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        initView();
        initListener();

        if (!MyApp.debugMine) {
            initAgoraRteSDK();
            joinChannel();
        }
    }

    @Override
    public void onDestroyView() {
        if (mPlayer != null) {
            mPlayer.unregisterMediaPlayerObserver(mPlayerObserver);
            mPlayer.destroy();
        }
        super.onDestroyView();
    }

    private void initView() {
        mBinding.containerFgPlayer.enableDefaultClickListener = false;
        mBinding.btnOpenFgPlayer.setOnClickListener(v -> openURL());
        mVideoView = mBinding.containerFgPlayer.createDemoLayout(VideoView.class, false);
        mVideoView.mPlayBtn.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (!buttonView.isPressed()) return;
            mVideoView.showOverlay();
            if (isChecked) mPlayer.play();
            else mPlayer.pause();
        });

        mVideoView.mProgressSlider.addOnChangeListener((slider, value, fromUser) -> {
            if (fromUser && mPlayer != null) {
                mVideoView.showOverlay();
                mPlayer.seek((long) value);
            }
        });
    }

    private void openURL() {
        boolean valid = false;
        String url = "";
        Editable editable = mBinding.inputUrlFgPlayer.getText();
        if (editable != null) {
            url = editable.toString();
            valid = URLUtil.isValidUrl(url);
        }

        if (valid) {
            doOpenURL(url);
            mBinding.btnOpenFgPlayer.setEnabled(false);
        }
        else ExampleUtil.shakeViewAndVibrateToAlert(mBinding.layoutInputUrlFgPlayer);
    }

    private void initListener() {
        mPlayerObserver = new AgoraRteMediaPlayerObserver() {
            @Override
            public void onVideoFrame(AgoraRteFileInfo fileInfo, AgoraRteVideoFrame videoFrame) {
                super.onVideoFrame(fileInfo, videoFrame);
                if(!initVideoView){
                    initVideoView = true;
                    new Handler(Looper.getMainLooper()).postAtFrontOfQueue(() -> {
                        mVideoView.mTextureView.getLayoutParams().height = mVideoView.mTextureView.getMeasuredWidth() * videoFrame.getHeight() / videoFrame.getWidth();
                        mVideoView.mTextureView.requestLayout();
                    });
                }
            }

            @Override
            public void onPlayerStateChanged(AgoraRteFileInfo fileInfo, AgoraRteMediaPlayerState state, AgoraRteMediaPlayerError error) {
                super.onPlayerStateChanged(fileInfo, state, error);
                ExampleUtil.utilLog(fileInfo.beginTime + "/" + fileInfo.duration + "state: " + state + ", error: " + error);
                if (state == AgoraRteMediaPlayerState.PLAYER_STATE_OPEN_COMPLETED) {
                    mPlayer.play();
                    mVideoView.mLoadingView.setVisibility(View.GONE);
                    mVideoView.mProgressSlider.setValueTo(mPlayer.getDuration());
                    mVideoView.performClick();
                } else if (state == AgoraRteMediaPlayerState.PLAYER_STATE_PLAYBACK_COMPLETED) {
                    mVideoView.mPlayBtn.setChecked(false);
                }
            }

            @Override
            public void onPositionChanged(AgoraRteFileInfo fileInfo, long position) {
                super.onPositionChanged(fileInfo, position);
                if(!mVideoView.mProgressSlider.isHovered())
                    mVideoView.mProgressSlider.setValue(position);
            }

        };
        mAgoraHandler = new AgoraRteSceneEventHandler() {
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState oldState, AgoraRteSceneConnState newState, AgoraRteConnectionChangedReason reason) {
                if (newState == AgoraRteSceneConnState.CONN_STATE_CONNECTED) {
                    ExampleUtil.utilLog("onConnectionStateChanged,Thread:"+Thread.currentThread().getName());
                    // RTC stream prepare
                    AgoraRtcStreamOptions option = new AgoraRtcStreamOptions();
                    mScene.createOrUpdateRTCStream(mLocalUserId, option);
                    // 准备视频采集
                    mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
                    // 必须先添加setPreviewCanvas，然后才能 startCapture
                    addCameraView();
                    if (mLocalVideoTrack != null) {
                        mLocalVideoTrack.startCapture(null);
                    }
                    mScene.publishLocalVideoTrack(mLocalUserId, mLocalVideoTrack);
                    // 准备音频采集
                    mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
                    mLocalAudioTrack.startRecording();
                    mScene.publishLocalAudioTrack(mLocalUserId, mLocalAudioTrack);

                    // MediaPlayer initiate
                    mPlayer = AgoraRteSDK.getRteMediaFactory().createMediaPlayer();
                    mPlayer.registerMediaPlayerObserver(mPlayerObserver);
                    mBinding.btnOpenFgPlayer.setEnabled(true);
                }
            }

            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo stream : streams) {
                    ExampleUtil.utilLog("sId:" + stream.getStreamId() + ",userId:" + stream.getUserId());
                    addRemoteView(stream.getStreamId());
                }
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo stream : streams) {
                    mBinding.containerFgPlayer.dynamicRemoveViewWithTag(stream.getStreamId());
                }
            }
        };
    }


    private void doOpenURL(String url) {
        addMediaView();
        mPlayer.open(url, 0);

        mScene.createOrUpdateRTCStream(mLocalMediaStreamId, new AgoraRtcStreamOptions());
        mScene.publishMediaPlayer(mLocalMediaStreamId, mPlayer);
    }

    public void joinChannel() {
        doJoinChannel(channelName, String.valueOf(new Random().nextInt(1024)), "");
    }

    private void addMediaView() {
        mBinding.containerFgPlayer.demoAddView(mVideoView, false);
        mPlayer.setView(mVideoView.mTextureView);
    }

    private void addCameraView() {
        SurfaceView view = mBinding.containerFgPlayer.createDemoLayout(SurfaceView.class, true);
        view.setZOrderMediaOverlay(true);
        mBinding.containerFgPlayer.demoAddView(view, true);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        if (mLocalVideoTrack != null) {
            mLocalVideoTrack.setPreviewCanvas(canvas);
        }
    }

    private void addRemoteView(String streamId) {
        SurfaceView view = mBinding.containerFgPlayer.createDemoLayout(SurfaceView.class, true);
        view.setZOrderMediaOverlay(true);
        view.setTag(streamId);
        mBinding.containerFgPlayer.demoAddView(view, true);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        mScene.setRemoteVideoCanvas(streamId, canvas);

        mScene.subscribeRemoteVideo(streamId, new AgoraRteVideoSubscribeOptions());
        mScene.subscribeRemoteAudio(streamId);
    }

    @Override
    public void doChangeView() {

    }
}
