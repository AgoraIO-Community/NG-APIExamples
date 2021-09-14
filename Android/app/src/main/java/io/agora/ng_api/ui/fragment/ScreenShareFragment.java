package io.agora.ng_api.ui.fragment;

import static android.app.Activity.RESULT_OK;

import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Bundle;
import android.view.TextureView;
import android.view.View;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.util.List;
import java.util.Random;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentScreenShareBinding;
import io.agora.ng_api.service.MediaProjectFgService;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.track.AgoraRteScreenVideoTrack;
import io.agora.rte.media.video.AgoraRteVideoCanvas;
import io.agora.rte.media.video.AgoraRteVideoEncoderConfiguration;
import io.agora.rte.media.video.AgoraRteVideoSubscribeOptions;
import io.agora.rte.scene.AgoraRteConnectionChangedReason;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;

public class ScreenShareFragment extends BaseDemoFragment<FragmentScreenShareBinding> {

    private Intent mediaProjectionIntent;

    @Nullable
    private AgoraRteScreenVideoTrack screenVideoTrack;
    private ActivityResultLauncher<Intent> activityResultLauncher;

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            initView();
            initListener();
            if (!MyApp.debugMine) {
                initAgoraRteSDK();
                joinChannel();
            }
        } else {
            MyApp.getInstance().shortToast(R.string.screen_share_version_unsupported);
            getNavController().popBackStack();
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        requireActivity().stopService(mediaProjectionIntent);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private void initView() {
        mediaProjectionIntent = new Intent(requireActivity(), MediaProjectFgService.class);
        mBinding.containerFgScreenShare.enableDefaultClickListener = false;

        // checked the button to record screen
        mBinding.btnOpenFgScreenShare.addOnCheckedChangeListener((button, isChecked) -> {
            if (!button.isPressed()) return;
            if (isChecked) {
                MediaProjectionManager mpm = (MediaProjectionManager) requireContext().getSystemService(Context.MEDIA_PROJECTION_SERVICE);
                Intent intent = mpm.createScreenCaptureIntent();
                activityResultLauncher.launch(intent);
            } else screenCaptureOperation(false);
        });
    }


    private void initListener() {

        // handle request screen record callback
        // since onActivityResult() is deprecated
        activityResultLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
            if (result.getResultCode() == RESULT_OK) {
                createOrUpdateScreenVideoTrack(result.getData());
                screenCaptureOperation(true);
            } else {
                mBinding.btnOpenFgScreenShare.toggle();
                MyApp.getInstance().shortToast(R.string.screen_share_request_denied);
            }
        });
        mAgoraHandler = new AgoraRteSceneEventHandler() {
            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState oldState, AgoraRteSceneConnState newState, AgoraRteConnectionChangedReason reason) {
                super.onConnectionStateChanged(oldState, newState, reason);
                if (newState == AgoraRteSceneConnState.CONN_STATE_CONNECTED) {
                    mBinding.btnOpenFgScreenShare.setEnabled(true);

                    // RTC stream prepare
                    AgoraRtcStreamOptions option = new AgoraRtcStreamOptions();
                    mScene.createOrUpdateRTCStream(mLocalUserId, option);
                    // 准备视频采集
                    mLocalVideoTrack = AgoraRteSDK.getRteMediaFactory().createCameraVideoTrack();
                    // 必须先添加setPreviewCanvas，然后才能 startCapture
                    addLocalView();
                    if (mLocalVideoTrack != null) {
                        mLocalVideoTrack.startCapture(null);
                    }
                    mScene.publishLocalVideoTrack(mLocalUserId, mLocalVideoTrack);
                    // 准备音频采集
                    mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
                    mLocalAudioTrack.startRecording();
                    mScene.publishLocalAudioTrack(mLocalUserId, mLocalAudioTrack);
                }
            }

            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo stream : streams) {
                    addRemoteView(stream.getStreamId());
                }
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> streams) {
                for (AgoraRteMediaStreamInfo stream : streams) {
                    mBinding.containerFgScreenShare.dynamicRemoveViewWithTag(stream.getStreamId());
                }
            }
        };
    }


    private void screenCaptureOperation(boolean turnOn) {
        if (turnOn) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                requireContext().startForegroundService(mediaProjectionIntent);
            } else {
                requireContext().startService(mediaProjectionIntent);
            }
        } else {
            // stop screen capture and update options
            if (screenVideoTrack != null) {
                mScene.unpublishLocalVideoTrack(screenVideoTrack);
                screenVideoTrack.stopCapture();
                requireActivity().stopService(mediaProjectionIntent);
            }
        }
    }

    /**
     * Total 4 Steps
     * 1: createScreenVideoTrack
     * 2: setPreviewCanvas
     * 3: startCaptureScreen
     * 4: publishLocalVideoTrack
     */
    private TextureView textureView;
    private AgoraRteVideoCanvas canvas;
    private void createOrUpdateScreenVideoTrack(Intent intent) {
        if (screenVideoTrack == null) {
            // Add View
            textureView = mBinding.containerFgScreenShare.createDemoLayout(TextureView.class);
            mBinding.containerFgScreenShare.demoAddView(textureView);
            // Create preview canvas
            canvas = new AgoraRteVideoCanvas(textureView);
            // Create screenVideoTrack
            screenVideoTrack = AgoraRteSDK.getRteMediaFactory().createScreenVideoTrack();
            // Set preview
        }
        screenVideoTrack.setPreviewCanvas(canvas);
        mScene.createOrUpdateRTCStream(mLocalMediaStreamId, new AgoraRtcStreamOptions());
        screenVideoTrack.startCaptureScreen(intent, new AgoraRteVideoEncoderConfiguration.VideoDimensions());
        // Publish screenVideoTrack
        mScene.publishLocalVideoTrack(mLocalMediaStreamId, screenVideoTrack);
    }

    private void addLocalView() {
        TextureView view = mBinding.containerFgScreenShare.createDemoLayout(TextureView.class);
        mBinding.containerFgScreenShare.demoAddView(view);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        if (mLocalVideoTrack != null) {
            mLocalVideoTrack.setPreviewCanvas(canvas);
        }
    }

    /**
     * Add view to show remote stream data
     *
     * @param streamId related remote streamID
     */
    private void addRemoteView(String streamId) {
        TextureView view = mBinding.containerFgScreenShare.createDemoLayout(TextureView.class);
        view.setTag(streamId);
        mBinding.containerFgScreenShare.demoAddView(view);
        AgoraRteVideoCanvas canvas = new AgoraRteVideoCanvas(view);
        mScene.setRemoteVideoCanvas(streamId, canvas);

        mScene.subscribeRemoteVideo(streamId, new AgoraRteVideoSubscribeOptions());
        mScene.subscribeRemoteAudio(streamId);
    }

    private void joinChannel() {
        doJoinChannel(channelName, String.valueOf(new Random().nextInt(1024)), "");
    }

    @Override
    public void doChangeView() {

    }
}
