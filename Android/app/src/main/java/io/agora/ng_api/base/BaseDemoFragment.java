package io.agora.ng_api.base;

import android.media.AudioManager;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.viewbinding.ViewBinding;
import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.ui.fragment.DescriptionFragment;
import io.agora.ng_api.util.ExampleUtil;
import io.agora.rte.AgoraRteSDK;
import io.agora.rte.AgoraRteSdkConfig;
import io.agora.rte.base.AgoraRteLogConfig;
import io.agora.rte.media.stream.AgoraRteMediaStreamInfo;
import io.agora.rte.media.track.AgoraRteCameraVideoTrack;
import io.agora.rte.media.track.AgoraRteMicrophoneAudioTrack;
import io.agora.rte.scene.AgoraRteScene;
import io.agora.rte.scene.AgoraRteSceneConfig;
import io.agora.rte.scene.AgoraRteSceneEventHandler;
import io.agora.rte.scene.AgoraRteSceneJoinOptions;
import io.agora.rte.user.AgoraRteUserInfo;

import java.util.ArrayList;
import java.util.List;

/**
 * On Jetpack navigation
 * Fragments enter/exit represent onCreateView/onDestroyView
 * Thus we should detach all reference to the VIEW on onDestroyView
 */
public abstract class BaseDemoFragment<B extends ViewBinding> extends BaseFragment<B> {

    // COMMON FILED
    public String sid;
    public String channelName;
    public AudioManager audioManager;
    public AgoraRteScene mScene;
    public AgoraRteSceneEventHandler mAgoraHandler;
    public AgoraRteCameraVideoTrack mLocalVideoTrack;
    public AgoraRteMicrophoneAudioTrack mLocalAudioTrack;


    public List<AgoraRteMediaStreamInfo> streamInfoList = new ArrayList<>();
    public List<AgoraRteUserInfo> userInfoList = new ArrayList<>();

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // ensure channelName
        if (getArguments() == null || getArguments().get(DescriptionFragment.channelName) == null) {
            MyApp.getInstance().shortToast("channelName required");
            getNavController().popBackStack();
            return;
        }
        channelName = getArguments().getString(DescriptionFragment.channelName);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        // view is gone, these data represent nothing.
        streamInfoList.clear();
        userInfoList.clear();
        if (!MyApp.debugMine) {
            doExitChannel();
        }
    }

    /**
     * app id 优先级
     * 用户主动输入 > strings 内置
     */
    public void initAgoraRteSDK() {
        AgoraRteSdkConfig profile = new AgoraRteSdkConfig();
        // check SP
        String appId = ExampleUtil.getSp(requireContext()).getString(ExampleUtil.APPID, "");
        // check strings
        if (appId.isEmpty()) appId = getString(R.string.agora_app_id);
        profile.appId = appId;
        profile.context = requireContext();
        profile.logConfig = new AgoraRteLogConfig(requireContext().getFilesDir().getAbsolutePath());
        AgoraRteSDK.init(profile);
//        if (res != 0) Toast.makeText(requireContext(), "AGORA SDK init error, code: " + res, Toast.LENGTH_SHORT).show();
    }

    /**
     * @param channelName channel name
     * @param userId      userId
     * @param token       access token
     */
    public void doJoinChannel(String channelName, String userId, String token) {
        AgoraRteSceneConfig config = new AgoraRteSceneConfig();
        mScene = AgoraRteSDK.createRteScene(channelName, config);
        mScene.registerSceneEventHandler(mAgoraHandler);
        mScene.join(userId, token, new AgoraRteSceneJoinOptions());
    }

    public void doExitChannel() {
        if (mScene != null) {
            mScene.leave();
            mScene.destroy();
        }
        AgoraRteSDK.deInit();
    }
}
