//
//  JoinChannelAudioMain.swift
//  APIExample
//
//  Created by ADMIN on 2020/5/18.
//  Copyright © 2020 Agora Corp. All rights reserved.
//

import UIKit
import AgoraRTE
import AGEVideoLayout
class JoinChannelAudioEntry : UIViewController
{
    @IBOutlet weak var joinButton: AGButton!
    @IBOutlet weak var channelTextField: AGTextField!
    @IBOutlet weak var profileBtn: UIButton!
    var profile : AgoraAudioProfile = .default
    let identifier = "JoinChannelAudio"
    
    override func viewDidLoad() {
        super.viewDidLoad()
        profileBtn.setTitle("\(profile.description())", for: .normal)
    }
    
    @IBAction func doJoinPressed(sender: AGButton) {
        guard let channelName = channelTextField.text else {return}
        //resign channel text field
        channelTextField.resignFirstResponder()
        
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        // create new view controller every time to ensure we get a clean vc
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName":channelName,"profile":profile]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
    
    func getAudioProfileAction(_ profile:AgoraAudioProfile) -> UIAlertAction{
        return UIAlertAction(title: "\(profile.description())", style: .default, handler: {[unowned self] action in
            self.profile = profile
            self.profileBtn.setTitle("\(profile.description())", for: .normal)
        })
    }
    
    @IBAction func setAudioProfile(){
        let alert = UIAlertController(title: "Set Audio Profile".localized, message: nil, preferredStyle: .actionSheet)
        for profile in AgoraAudioProfile.allValues(){
            alert.addAction(getAudioProfileAction(profile))
        }
        alert.addCancelAction()
        present(alert, animated: true, completion: nil)
    }
}

class JoinChannelAudioMain: BaseViewController {
    var agoraKit: AgoraRteSdk!
    var scene: AgoraRteSceneProtocol!
    var microphoneTrack: AgoraRteMicrophoneAudioTrackProtocol!
    @IBOutlet weak var container: AGEVideoContainer!
    @IBOutlet weak var recordingVolumeSlider: UISlider!
    @IBOutlet weak var playbackVolumeSlider: UISlider!
    @IBOutlet weak var inEarMonitoringSwitch: UISwitch!
    @IBOutlet weak var inEarMonitoringVolumeSlider: UISlider!
    var audioViews: [UInt:VideoView] = [:]
    let LOCAL_STREAM_ID = String(UInt.random(in: 1000...2000))
    
    
    // indicate if current instance has joined channel
    var isJoined: Bool = false
    
    override func viewDidLoad(){
        super.viewDidLoad()
        
        guard let channelName = configs["channelName"] as? String,
            let audioProfile = configs["profile"] as? AgoraAudioProfile
            else { return }
        
        let profile = AgoraRteSdkProfile()
        profile.appid = KeyCenter.AppId
        agoraKit = AgoraRteSdk.sharedEngine(with: profile)
        let config = AgoraRteSceneConfg()
        config.enableAudioRecordingOrPlayout = true
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: config)
        
        scene?.setSceneDelegate(self)
        
        let audioConfig = AgoraRteAudioEncoderConfiguration()
        audioConfig.profile = audioProfile
        scene?.setAudioEncoderConfiguration(LOCAL_STREAM_ID, audioEncoderConfiguration: audioConfig)
        
        let mediaControl = agoraKit.getRteMediaFactory()
        microphoneTrack = mediaControl?.createMicrophoneAudioTrack()
        
        let joinOption = AgoraRteJoinOptions()
        joinOption.isUserVisibleToRemote = true
        
        scene.joinScene(withUserId: LOCAL_STREAM_ID, token: "", joinOptions: joinOption)
        microphoneTrack?.startRecording()
        let streamOption = AgoraRteRtcStreamOptions()
        streamOption.token = ""
        scene?.createOrUpdateRTCStream(LOCAL_STREAM_ID, rtcStreamOptions: streamOption)
        scene?.publishLocalAudioTrack(LOCAL_STREAM_ID, rteAudioTrack: microphoneTrack!)
        
//        AgoraRteObjcSceneProtocol.
        
            
//        // set up agora instance when view loaded
//        let config = AgoraRtcEngineConfig()
//        config.appId = KeyCenter.AppId
//        config.areaCode = GlobalSettings.shared.area
//        config.channelProfile = .liveBroadcasting
//        // set audio scenario
//        config.audioScenario = audioScenario
//        agoraKit = AgoraRtcEngineKit.sharedEngine(with: config, delegate: self)
//        agoraKit.setLogFile(LogUtils.sdkLogPath())
//
//        // make myself a broadcaster
//        agoraKit.setClientRole(.broadcaster)
//
//        // disable video module
//        agoraKit.disableVideo()
//
//        // set audio profile
//        agoraKit.setAudioProfile(audioProfile)
//
//        // Set audio route to speaker
//        agoraKit.setDefaultAudioRouteToSpeakerphone(true)
//
//        // enable volume indicator
//        agoraKit.enableAudioVolumeIndication(200, smooth: 3)
        
        recordingVolumeSlider.maximumValue = 400
        recordingVolumeSlider.minimumValue = 0
        recordingVolumeSlider.integerValue = 100
        
        playbackVolumeSlider.maximumValue = 400
        playbackVolumeSlider.minimumValue = 0
        playbackVolumeSlider.integerValue = 100
        
        inEarMonitoringVolumeSlider.maximumValue = 100
        inEarMonitoringVolumeSlider.minimumValue = 0
        inEarMonitoringVolumeSlider.integerValue = 100
        
        // start joining channel
        // 1. Users can only see each other after they join the
        // same channel successfully using the same app id.
        // 2. If app certificate is turned on at dashboard, token is needed
        // when joining channel. The channel name and uid used to calculate
        // the token has to match the ones used for channel join
//        let option = AgoraRtcChannelMediaOptions()
//        option.publishCameraTrack = .of(false)
//        option.clientRoleType = .of((Int32)(AgoraClientRole.broadcaster.rawValue))
//
//        let result = agoraKit.joinChannel(byToken: KeyCenter.Token, channelId: channelName, uid: 0, mediaOptions: option)
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
//                agoraKit.leaveChannel { (stats) -> Void in
//                    LogUtils.log(message: "left channel, duration: \(stats.duration)", level: .info)
//                }
            }
        }
    }
    
    func sortedViews() -> [VideoView] {
        return Array(audioViews.values).sorted(by: { $0.uid < $1.uid })
    }
    
    @IBAction func onChangeRecordingVolume(_ sender:UISlider){
        let value:Int = Int(sender.value)
        print("adjustRecordingSignalVolume \(value)")
//        agoraKit.adjustRecordingSignalVolume(value)
        microphoneTrack.adjustPublishVolume(value)
    }
    
    @IBAction func onChangePlaybackVolume(_ sender:UISlider){
        let value:Int = Int(sender.value)
        print("adjustPlaybackSignalVolume \(value)")
//        agoraKit.adjustPlaybackSignalVolume(value)
        microphoneTrack.adjustPlayoutVolume(value)
    }
    
    @IBAction func toggleInEarMonitoring(_ sender:UISwitch){
        inEarMonitoringVolumeSlider.isEnabled = sender.isOn
//        agoraKit.enable(inEarMonitoring: sender.isOn)
        microphoneTrack.enableEarMonitor(sender.isOn, audioFilters: 1)
    }
    
    @IBAction func onChangeInEarMonitoringVolume(_ sender:UISlider){
        let value:Int = Int(sender.value)
        print("setInEarMonitoringVolume \(value)")
//        microphoneTrack
    }
}

extension JoinChannelAudioMain: AgoraRteSceneDelegate {
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, didRemoteUserJoined userInfos: [AgoraRteStreamInfo]?) {
        guard let infos = userInfos else { return }
        for info in infos {
            rteScene.subscribeRemoteAudio(info.streamId!)
            
            DispatchQueue.main.async { [weak self] in
                guard let strongSelf = self else {
                    return
                }
                //set up remote audio view, this view will not show video but just a placeholder
                let view = Bundle.loadVideoView(type: .remote, audioOnly: true)
                view.uid = UInt(info.streamId!)!
                strongSelf.audioViews[UInt(info.streamId!)!] = view
                view.setPlaceholder(text: strongSelf.getAudioLabel(uid: UInt(info.streamId!)!, isLocal: false))
                strongSelf.container.layoutStream3x2(views: strongSelf.sortedViews())
                strongSelf.container.reload(level: 0, animated: true)
            }
        }
    }
    
    func agoraRteObjcScene(_ rteScene: AgoraRteSceneProtocol, didRemoteStreamAdded streams: [Any]?) {
        print("didRemoteStreamAdded")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, connectionStateDidChange oldState: AgoraConnectionState, newState state: AgoraConnectionState, changedReason reason: AgoraConnectionChangedReason) {
        print("didConnectionStateChanged state:\(state.rawValue) reason:\(reason.rawValue)")
        if state == .connected{
            self.isJoined = true
            DispatchQueue.main.async { [weak self] in
                guard let strongSelf = self else {
                    return
                }
                //set up local audio view, this view will not show video but just a placeholder
                let view = Bundle.loadVideoView(type: .local, audioOnly: true)
                strongSelf.audioViews[0] = view
                view.setPlaceholder(text: strongSelf.getAudioLabel(uid: UInt(strongSelf.LOCAL_STREAM_ID) ?? 0, isLocal: true))
                strongSelf.container.layoutStream3x2(views: strongSelf.sortedViews())
            }
        }
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, localStreamStateDidChange streams: AgoraRteStreamInfo?, mediaType: AgoraRteMediaType, steamMediaState oldState: AgoraRteStreamState, newState: AgoraRteStreamState, stateChangedReason reason: AgoraRteStreamStateChangedReason) {
        print("didLocalStreamStateChanged \(String(describing: streams?.streamId)), audio sentBitrate: \(String(describing: newState))")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, localStreamDidStats streamId: String?, stats: AgoraRteLocalStreamStats?) {
        print("didLocalStreamStats \(String(describing: streamId)), audio sentBitrate: \(String(describing: stats?.audioStats?.sentBitrate))")
        audioViews[0]?.statsInfo?.updateLocalAudioStats(stats!)
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, sceneStats stats: AgoraRteSceneStats?) {
        print("didSceneStats")
        guard stats != nil else {
            return
        }
        audioViews[0]?.statsInfo?.updateChannelStats(stats!)
    }
}

/// agora rtc engine delegate events
//extension JoinChannelAudioMain: AgoraRtcEngineDelegate {
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
//
//        //set up local audio view, this view will not show video but just a placeholder
//        let view = Bundle.loadVideoView(type: .local, audioOnly: true)
//        self.audioViews[0] = view
//        view.setPlaceholder(text: self.getAudioLabel(uid: uid, isLocal: true))
//        self.container.layoutStream3x2(views: self.sortedViews())
//    }
//
//    /// callback when a remote user is joinning the channel, note audience in live broadcast mode will NOT trigger this event
//    /// @param uid uid of remote joined user
//    /// @param elapsed time elapse since current sdk instance join the channel in ms
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didJoinedOfUid uid: UInt, elapsed: Int) {
//        LogUtils.log(message: "remote user join: \(uid) \(elapsed)ms", level: .info)
//
//        //set up remote audio view, this view will not show video but just a placeholder
//        let view = Bundle.loadVideoView(type: .remote, audioOnly: true)
//        view.uid = uid
//        self.audioViews[uid] = view
//        view.setPlaceholder(text: self.getAudioLabel(uid: uid, isLocal: false))
//        self.container.layoutStream3x2(views: sortedViews())
//        self.container.reload(level: 0, animated: true)
//    }
//
//    /// callback when a remote user is leaving the channel, note audience in live broadcast mode will NOT trigger this event
//    /// @param uid uid of remote joined user
//    /// @param reason reason why this user left, note this event may be triggered when the remote user
//    /// become an audience in live broadcasting profile
//    func rtcEngine(_ engine: AgoraRtcEngineKit, didOfflineOfUid uid: UInt, reason: AgoraUserOfflineReason) {
//        LogUtils.log(message: "remote user left: \(uid) reason \(reason)", level: .info)
//
//        //remove remote audio view
//        self.audioViews.removeValue(forKey: uid)
//        self.container.layoutStream3x2(views: sortedViews())
//        self.container.reload(level: 0, animated: true)
//    }
//
//    /// Reports which users are speaking, the speakers' volumes, and whether the local user is speaking.
//    /// @params speakers volume info for all speakers
//    /// @params totalVolume Total volume after audio mixing. The value range is [0,255].
//    func rtcEngine(_ engine: AgoraRtcEngineKit, reportAudioVolumeIndicationOfSpeakers speakers: [AgoraRtcAudioVolumeInfo], totalVolume: Int) {
////        for speaker in speakers {
////            if let audioView = audioViews[speaker.uid] {
////                audioView.setInfo(text: "Volume:\(speaker.volume)")
////            }
////        }
//    }
//
//    /// Reports the statistics of the current call. The SDK triggers this callback once every two seconds after the user joins the channel.
//    /// @param stats stats struct
//    func rtcEngine(_ engine: AgoraRtcEngineKit, reportRtcStats stats: AgoraChannelStats) {
//        audioViews[0]?.statsInfo?.updateChannelStats(stats)
//    }
//
//    /// Reports the statistics of the uploading local audio streams once every two seconds.
//    /// @param stats stats struct
//    func rtcEngine(_ engine: AgoraRtcEngineKit, localAudioStats stats: AgoraRtcLocalAudioStats) {
//        audioViews[0]?.statsInfo?.updateLocalAudioStats(stats)
//    }
//
//    /// Reports the statistics of the audio stream from each remote user/host.
//    /// @param stats stats struct for current call statistics
//    func rtcEngine(_ engine: AgoraRtcEngineKit, remoteAudioStats stats: AgoraRtcRemoteAudioStats) {
//        audioViews[stats.uid]?.statsInfo?.updateAudioStats(stats)
//    }
//}
