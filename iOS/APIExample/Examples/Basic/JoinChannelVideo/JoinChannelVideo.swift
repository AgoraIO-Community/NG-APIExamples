//
//  JoinChannelVC.swift
//  APIExample
//
//  Created by 张乾泽 on 2020/4/17.
//  Copyright © 2020 Agora Corp. All rights reserved.
//
import UIKit
import AGEVideoLayout
import AgoraRTE

class JoinChannelVideoEntry : UIViewController
{
    @IBOutlet weak var joinButton: UIButton!
    @IBOutlet weak var channelTextField: UITextField!
    let identifier = "JoinChannelVideo"
    @IBOutlet var resolutionBtn: UIButton!
    @IBOutlet var fpsBtn: UIButton!
    @IBOutlet var orientationBtn: UIButton!
    var width:Int = 640, height:Int = 360, orientation:AgoraVideoOutputOrientationMode = .adaptative, fps = 30
    
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    
    func getResolutionAction(width:Int, height:Int) -> UIAlertAction{
        return UIAlertAction(title: "\(width)x\(height)", style: .default, handler: {[unowned self] action in
            self.width = width
            self.height = height
            self.resolutionBtn.setTitle("\(width)x\(height)", for: .normal)
        })
    }
    
    func getFpsAction(_ fps:Int) -> UIAlertAction{
        return UIAlertAction(title: "\(fps)fps", style: .default, handler: {[unowned self] action in
            self.fps = fps
            self.fpsBtn.setTitle("\(fps)fps", for: .normal)
        })
    }
    
    func getOrientationAction(_ orientation:AgoraVideoOutputOrientationMode) -> UIAlertAction{
        return UIAlertAction(title: "\(orientation.description())", style: .default, handler: {[unowned self] action in
            self.orientation = orientation
            self.orientationBtn.setTitle("\(orientation.description())", for: .normal)
        })
    }
    
    @IBAction func setResolution(){
        let alert = UIAlertController(title: "Set Resolution".localized, message: nil, preferredStyle: .actionSheet)
        alert.addAction(getResolutionAction(width: 90, height: 90))
        alert.addAction(getResolutionAction(width: 160, height: 120))
        alert.addAction(getResolutionAction(width: 320, height: 240))
        alert.addAction(getResolutionAction(width: 640, height: 360))
        alert.addAction(getResolutionAction(width: 1280, height: 720))
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
    
    @IBAction func setFps(){
        let alert = UIAlertController(title: "Set Fps".localized, message: nil, preferredStyle: .actionSheet)
        alert.addAction(getFpsAction(10))
        alert.addAction(getFpsAction(15))
        alert.addAction(getFpsAction(24))
        alert.addAction(getFpsAction(30))
        alert.addAction(getFpsAction(60))
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
    
    @IBAction func setOrientation(){
        let alert = UIAlertController(title: "Set Orientation".localized, message: nil, preferredStyle: .actionSheet)
        alert.addAction(getOrientationAction(.adaptative))
        alert.addAction(getOrientationAction(.fixedLandscape))
        alert.addAction(getOrientationAction(.fixedPortrait))
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
    
    @IBAction func doJoinPressed(sender: UIButton) {
        guard let channelName = channelTextField.text else {return}
        //resign channel text field
        channelTextField.resignFirstResponder()
        
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        // create new view controller every time to ensure we get a clean vc
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName":channelName, "resolution":CGSize(width: width, height: height), "fps": fps, "orientation": orientation]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
}

class JoinChannelVideoMain: BaseViewController {
    var localVideo = Bundle.loadVideoView(type: .local, audioOnly: false)
    var remoteVideo = Bundle.loadVideoView(type: .remote, audioOnly: false)
    
    @IBOutlet weak var container: AGEVideoContainer!
    
    var agoraKit: AgoraRteSdk!
    var scene: AgoraRteSceneProtocol!
    var microphoneTrack: AgoraRteMicrophoneAudioTrackProtocol!
    var cameraTrack: AgoraRteCameraVideoTrackProtocol!
    let LOCAL_STREAM_ID = String(UInt.random(in: 1000...2000))
    
    // indicate if current instance has joined channel
    var isJoined: Bool = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // get channel name from configs
        guard let channelName = configs["channelName"] as? String,
            let resolution = configs["resolution"] as? CGSize,
            let fps = configs["fps"] as? Int,
            let orientation = configs["orientation"] as? AgoraVideoOutputOrientationMode else {return}
        
        // layout render view
        localVideo.setPlaceholder(text: "Local Host".localized)
        remoteVideo.setPlaceholder(text: "Remote Host".localized)
        container.layoutStream(views: [localVideo, remoteVideo])
        
        // set up agora instance when view loaded
//        let config = AgoraRtcEngineConfig()
//        config.appId = KeyCenter.AppId
//        config.areaCode = GlobalSettings.shared.area
//        config.channelProfile = .liveBroadcasting
//        agoraKit = AgoraRtcEngineKit.sharedEngine(with: config, delegate: self)
//        agoraKit.setLogFile(LogUtils.sdkLogPath())
        
        let profile = AgoraRteSdkProfile()
        profile.appid = KeyCenter.AppId
        agoraKit = AgoraRteSdk.sharedEngine(with: profile)
        let config = AgoraRteSceneConfg()
        config.enableAudioRecordingOrPlayout = true
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: config)
        
        scene?.setSceneDelegate(self)
        
        let videoConfig = AgoraVideoEncoderConfiguration(size: resolution, frameRate: AgoraVideoFrameRate(rawValue: fps) ?? .fps15, bitrate: AgoraVideoBitrateStandard, orientationMode: orientation, mirrorMode: .auto)
        
        let mediaControl = agoraKit.getRteMediaFactory()
        cameraTrack = mediaControl?.createCameraVideoTrack()
        microphoneTrack = mediaControl?.createMicrophoneAudioTrack()
        
        let joinOption = AgoraRteJoinOptions()
        joinOption.isUserVisibleToRemote = true
        
        scene.joinScene(withUserId: LOCAL_STREAM_ID, token: "", joinOptions: joinOption)
        microphoneTrack?.startRecording()
        
        let videoCanvas = AgoraRtcVideoCanvas()
        videoCanvas.uid = 0
        videoCanvas.view = localVideo.videoView
        videoCanvas.renderMode = .hidden
        cameraTrack?.setPreviewCanvas(videoCanvas)
        
        let streamOption = AgoraRteRtcStreamOptions()
        streamOption.token = ""
        scene?.createOrUpdateRTCStream(LOCAL_STREAM_ID, rtcStreamOptions: streamOption)
        scene?.publishLocalAudioTrack(LOCAL_STREAM_ID, rteAudioTrack: microphoneTrack!)
        scene.publishLocalVideoTrack(LOCAL_STREAM_ID, rteVideoTrack: cameraTrack!)
        
        
//
//        // set up local video to render your local camera preview
//        let videoCanvas = AgoraRtcVideoCanvas()
//        videoCanvas.uid = 0
//        // the view to be binded
//        videoCanvas.view = localVideo.videoView
//        videoCanvas.renderMode = .hidden
//        agoraKit.setupLocalVideo(videoCanvas)
//        // you have to call startPreview to see local video
//        agoraKit.startPreview()
//
//        // Set audio route to speaker
//        agoraKit.setDefaultAudioRouteToSpeakerphone(true)
//
//        // start joining channel
//        // 1. Users can only see each other after they join the
//        // same channel successfully using the same app id.
//        // 2. If app certificate is turned on at dashboard, token is needed
//        // when joining channel. The channel name and uid used to calculate
//        // the token has to match the ones used for channel join
//        let option = AgoraRtcChannelMediaOptions()
//        option.publishCameraTrack = .of(true)
//        option.clientRoleType = .of((Int32)(AgoraClientRole.broadcaster.rawValue))
//
//        let result = agoraKit.joinChannel(byToken: KeyCenter.Token, channelId: channelName, uid: 0, mediaOptions: option)
//        //let result = agoraKit.joinChannel(byToken: KeyCenter.Token, channelId: channelName, info: nil, uid: 0)
//        if result != 0 {
//            // Usually happens with invalid parameters
//            // Error code description can be found at:
//            // en: https://docs.agora.io/en/Voice/API%20Reference/oc/Constants/AgoraErrorCode.html
//            // cn: https://docs.agora.io/cn/Voice/API%20Reference/oc/Constants/AgoraErrorCode.html
//            self.showAlert(title: "Error", message: "joinChannel call failed: \(result), please check your params")
//        }
    }
    
    override func willMove(toParent parent: UIViewController?) {
        if parent == nil {
            // leave channel when exiting the view
            if isJoined {
//                agoraKit.stopPreview()
//                agoraKit.leaveChannel { (stats) -> Void in
//                    LogUtils.log(message: "left channel, duration: \(stats.duration)", level: .info)
//                }
            }
        }
    }
}

/// agora rtc engine delegate events
//extension JoinChannelVideoMain: AgoraRtcEngineDelegate {
//    /// callback when warning occured for agora sdk, warning can usually be ignored, still it's nice to check out
//    /// what is happening
//    /// Warning code description can be found at:
//    /// en: https://docs.agora.io/en/Voice/API%20Reference/oc/Constants/AgoraWarningCode.html
//    /// cn: https://docs.agora.io/cn/Voice/API%20Reference/oc/Constants/AgoraWarningCode.html
//    /// @param warningCode warning code of the problem
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didOccurWarning warningCode: AgoraWarningCode) {
//        LogUtils.log(message: "warning: \(warningCode.description)", level: .warning)
//    }
//
//    /// callback when error occured for agora sdk, you are recommended to display the error descriptions on demand
//    /// to let user know something wrong is happening
//    /// Error code description can be found at:
//    /// en: https://docs.agora.io/en/Voice/API%20Reference/oc/Constants/AgoraErrorCode.html
//    /// cn: https://docs.agora.io/cn/Voice/API%20Reference/oc/Constants/AgoraErrorCode.html
//    /// @param errorCode error code of the problem
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didOccurError errorCode: AgoraErrorCode) {
//        LogUtils.log(message: "error: \(errorCode)", level: .error)
//        self.showAlert(title: "Error", message: "Error \(errorCode.description) occur")
//    }
//
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinChannel channel: String, withUid uid: UInt, elapsed: Int) {
//        self.isJoined = true
//        LogUtils.log(message: "Join \(channel) with uid \(uid) elapsed \(elapsed)ms", level: .info)
//    }
//
//    /// callback when a remote user is joinning the channel, note audience in live broadcast mode will NOT trigger this event
//    /// @param uid uid of remote joined user
//    /// @param elapsed time elapse since current sdk instance join the channel in ms
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinedOfUid uid: UInt, elapsed: Int) {
//        LogUtils.log(message: "remote user join: \(uid) \(elapsed)ms", level: .info)
//
//        agoraKit.setupRemoteVideo(videoCanvas)
//    }
//
//    /// callback when a remote user is leaving the channel, note audience in live broadcast mode will NOT trigger this event
//    /// @param uid uid of remote joined user
//    /// @param reason reason why this user left, note this event may be triggered when the remote user
//    /// become an audience in live broadcasting profile
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didOfflineOfUid uid: UInt, reason: AgoraUserOfflineReason) {
//        LogUtils.log(message: "remote user left: \(uid) reason \(reason)", level: .info)
//
//        // to unlink your view from sdk, so that your view reference will be released
//        // note the video will stay at its last frame, to completely remove it
//        // you will need to remove the EAGL sublayer from your binded view
//        let videoCanvas = AgoraRtcVideoCanvas()
//        videoCanvas.uid = uid
//        // the view to be binded
//        videoCanvas.view = nil
//        videoCanvas.renderMode = .hidden
//        agoraKit.setupRemoteVideo(videoCanvas)
//    }
//
//    /// Reports the statistics of the current call. The SDK triggers this callback once every two seconds after the user joins the channel.
//    /// @param stats stats struct
//    func rtcEngine(_ engine: AgoraRtcEngineKit, reportRtcStats stats: AgoraChannelStats) {
//        localVideo.statsInfo?.updateChannelStats(stats)
//    }
//
//    /// Reports the statistics of the uploading local video streams once every two seconds.
//    /// @param stats stats struct
//    func rtcEngine(_ engine: AgoraRtcEngineKit, localVideoStats stats: AgoraRtcLocalVideoStats) {
//        localVideo.statsInfo?.updateLocalVideoStats(stats)
//    }
//
//    /// Reports the statistics of the uploading local audio streams once every two seconds.
//    /// @param stats stats struct
//    func rtcEngine(_ engine: AgoraRtcEngineKit, localAudioStats stats: AgoraRtcLocalAudioStats) {
//        localVideo.statsInfo?.updateLocalAudioStats(stats)
//    }
//
//    /// Reports the statistics of the video stream from each remote user/host.
//    /// @param stats stats struct
//    func rtcEngine(_ engine: AgoraRtcEngineKit, remoteVideoStats stats: AgoraRtcRemoteVideoStats) {
//        remoteVideo.statsInfo?.updateVideoStats(stats)
//    }
//
//    /// Reports the statistics of the audio stream from each remote user/host.
//    /// @param stats stats struct for current call statistics
//    func rtcEngine(_ engine: AgoraRtcEngineKit, remoteAudioStats stats: AgoraRtcRemoteAudioStats) {
//        remoteVideo.statsInfo?.updateAudioStats(stats)
//    }
//}

extension JoinChannelVideoMain: AgoraRteSceneDelegate {
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didRemoteUserJoined userInfos: [AgoraRteStreamInfo]?) {
        guard let infos = userInfos else { return }
        for info in infos {
            rteScene.subscribeRemoteAudio(info.streamId!)
            let option = AgoraRteVideoSubscribeOptions()
            rteScene.subscribeRemoteVideo(info.streamId!, videoSubscribeOptions: option)
            
            DispatchQueue.main.async { [weak self] in
                guard let strongSelf = self else {
                    return
                }
                // Only one remote video view is available for this
                // tutorial. Here we check if there exists a surface
                // view tagged as this uid.
                let videoCanvas = AgoraRtcVideoCanvas()
                videoCanvas.uid = UInt(info.streamId!)!
                // the view to be binded
                videoCanvas.view = strongSelf.remoteVideo.videoView
                videoCanvas.renderMode = .hidden
                rteScene.setRemoteVideoCanvas(info.streamId!, videoCanvas: videoCanvas)
            }
        }
    }
    
    func agoraRteObjcScene(_ rteScene: AgoraRteSceneProtocol, didRemoteStreamAdded streams: [Any]?) {
        print("didRemoteStreamAdded")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didConnectionStateChanged oldState: AgoraConnectionState, newState state: AgoraConnectionState, changedReason reason: AgoraConnectionChangedReason) {
        print("didConnectionStateChanged state:\(state.rawValue) reason:\(reason.rawValue)")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didLocalStreamStateChanged streams: AgoraRteStreamInfo?, mediaType: AgoraRteMediaType, steamMediaState oldState: AgoraRteStreamState, newState: AgoraRteStreamState, stateChangedReason reason: AgoraRteStreamStateChangedReason) {
        print("didLocalStreamStateChanged \(String(describing: streams?.streamId)), audio sentBitrate: \(String(describing: newState))")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didLocalStreamStats streamId: String?, stats: AgoraRteLocalStreamStats?) {
        print("didLocalStreamStats \(String(describing: streamId)), audio sentBitrate: \(String(describing: stats?.audioStats?.sentBitrate))")
        remoteVideo.statsInfo?.updateLocalVideoStats(stats!)
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didSceneStats stats: AgoraRteSceneStats?) {
        print("didSceneStats")
        guard stats != nil else {
            return
        }
        localVideo.statsInfo?.updateChannelStats(stats!)
    }
}
