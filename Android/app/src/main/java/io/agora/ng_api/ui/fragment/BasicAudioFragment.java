package io.agora.ng_api.ui.fragment;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseDemoFragment;
import io.agora.ng_api.databinding.FragmentBasicAudioBinding;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.ng_api.view.ScrollableLinearLayout;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.media.stream.AgoraRtcStreamOptions;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.scene.AgoraRteSceneConnState;
import io.agora.rte.scene.AgoraRteSceneEventHandler;
import io.agora.rte.statistics.AgoraRteLocalAudioStats;
import io.agora.rte.statistics.AgoraRteRemoteAudioStats;

/**
 * This demo demonstrates how to make a Basic Audio Scene
 */
public class BasicAudioFragment extends BaseDemoFragment<FragmentBasicAudioBinding> {

    private final Map<String, MutableLiveData<String>> liveStat = new HashMap<>();

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView();
        initListener();
        if (!MyApp.justDebugUIPart) {
            initAgoraRteSDK();
            joinScene();
        }
    }

    private void initView() {
    }

    private void initListener() {

        mAgoraHandler = new AgoraRteSceneEventHandler() {

            @Override
            public void onConnectionStateChanged(AgoraRteSceneConnState state, AgoraRteSceneConnState state1, io.agora.rte.scene.AgoraRteConnectionChangedReason reason) {
                ExampleUtil.utilLog("onConnectionStateChanged: " + state.getValue() + ", " + state1.getValue() + ",reason: " + reason.getValue() + "，\nThread:" + Thread.currentThread().getName());
                if (mBinding == null) return;
                // 连接建立完成
                /*
                    1. createOrUpdateRTCStream
                    2. addVoiceView
                    3. initBasicLocalAudioTrack
                 */
                if (state1 == AgoraRteSceneConnState.CONN_STATE_CONNECTED && mLocalAudioTrack == null) {
                    // Step 1
                    mScene.createOrUpdateRTCStream(mLocalUserId, new AgoraRtcStreamOptions());
                    // Step 2
                    addVoiceView(null);
                    // Step 3
                    initBasicLocalAudioTrack();
                }
            }

            @Override
            public void onRemoteStreamAdded(List<AgoraRteMediaStreamInfo> list) {
                if (mBinding == null) return;
                for (AgoraRteMediaStreamInfo info : list)
                    addVoiceView(info);
            }

            @Override
            public void onRemoteStreamRemoved(List<AgoraRteMediaStreamInfo> list) {
                ExampleUtil.utilLog("onRemoteStreamRemoved: " + Thread.currentThread().getName());
                if (mBinding == null) return;

                for (AgoraRteMediaStreamInfo info : list) {
                    // Remove view
                    mBinding.containerBasicAudio.dynamicRemoveViewWithTag(info.getStreamId());

                    /*
                        Stop update stat
                        停止信息展示监听
                     */
                    LiveData<String> currentLiveData = liveStat.get(info.getStreamId());
                    if (currentLiveData != null) {
                        currentLiveData.removeObservers(getViewLifecycleOwner());
                        liveStat.remove(info.getStreamId());
                    }
                }
            }

            @Override
            public void onLocalStreamAudioStats(String streamId, AgoraRteLocalAudioStats stats) {
                MutableLiveData<String> test = liveStat.get(mLocalUserId);
                if (test != null) {
                    String localStatInfo = "ChannelCount:" + stats.getNumChannels() + "\n" +
                            stats.getSendBitrateInKbps() + "Kbps " +
                            stats.getSentSampleRate() + "Hz";
                    test.setValue(localStatInfo);
                }
            }

            @Override
            public void onRemoteStreamAudioStats(String streamId, AgoraRteRemoteAudioStats stats) {
                MutableLiveData<String> test = liveStat.get(streamId);
                if (test != null) {
                    String remoteStatInfo = "ChannelCount:" + stats.getNumChannels() + "\n" +
                            stats.getReceivedBitrate() + "Kbps " +
                            stats.getReceivedSampleRate() + "Hz " + stats.getAudioLossRate() + "%";
                    test.setValue(remoteStatInfo);
                }
            }
        };

    }

    private void initBasicLocalAudioTrack() {
        mLocalAudioTrack = AgoraRteSDK.getRteMediaFactory().createMicrophoneAudioTrack();
        mLocalAudioTrack.startRecording();
        mScene.publishLocalAudioTrack(mLocalStreamId, mLocalAudioTrack);
    }

    /**
     * Every time a user joined scene, we add a view
     * 每当有用户加入场景，在界面上添加一个View
     *
     * @param info 用户信息
     *             若为 null 则表示为本机用户，不会订阅音频流.
     *             if info is null, just add a view
     *             indicates that we have successfully
     *             joined this scene and will not subscribe
     *             a audio stream.
     */
    private void addVoiceView(@Nullable AgoraRteMediaStreamInfo info) {
        // 用 streamId 作为 tag 标记每个用户
        String tag = info == null ? null : info.getStreamId();
        // 本地视图 title 为 local_{userId}，远端为 {userId}
        String title = info == null ? getString(R.string.local_user_id_format, mLocalUserId) : info.getUserId();
        // 工具方法，直接返回一个 CardView, 内部包含一个 TextView
        CardView cardView = ScrollableLinearLayout.getChildAudioCardView(requireContext(), tag, title);
        // 工具方法，简化属性设置流程, 直接添加View
        mBinding.containerBasicAudio.dynamicAddView(cardView);

        /*
            Start listen data changes
            监听音频信息变化并展示在界面上
         */

        MutableLiveData<String> mutableLiveData = new MutableLiveData<>();
        mutableLiveData.observe(getViewLifecycleOwner(), s -> ((TextView) cardView.getChildAt(0)).setText(s));

        if (tag != null) {
            liveStat.put(tag, mutableLiveData);
            // Start receive audio data
            mScene.subscribeRemoteAudio(tag);
        } else {
            liveStat.put(mLocalUserId, mutableLiveData);
        }

    }

    private void joinScene() {
        doJoinScene(sceneName, mLocalUserId, "");
    }

    @Override
    public void doChangeView() {

    }
}
