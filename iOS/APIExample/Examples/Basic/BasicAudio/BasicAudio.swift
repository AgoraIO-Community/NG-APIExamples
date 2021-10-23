//
//  BasicAudioMain.swift
//  APIExample
//
//  Created by ADMIN on 2020/5/18.
//  Copyright © 2020 Agora Corp. All rights reserved.
//

import UIKit
import AgoraRTE
import AGEVideoLayout
class BasicAudioEntry : UIViewController
{
    @IBOutlet weak var joinButton: AGButton!
    @IBOutlet weak var channelTextField: AGTextField!
    let identifier = "BasicAudio"
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    @IBAction func doJoinPressed(sender: AGButton) {
        guard let channelName = channelTextField.text else {return}
        //resign channel text field
        channelTextField.resignFirstResponder()
        
        let storyBoard: UIStoryboard = UIStoryboard(name: identifier, bundle: nil)
        // create new view controller every time to ensure we get a clean vc
        guard let newViewController = storyBoard.instantiateViewController(withIdentifier: identifier) as? BaseViewController else {return}
        newViewController.title = channelName
        newViewController.configs = ["channelName":channelName]
        self.navigationController?.pushViewController(newViewController, animated: true)
    }
}

class BasicAudioMain: BaseViewController {
    var agoraKit: AgoraRteSdk!
    var scene: AgoraRteSceneProtocol!
    var microphoneTrack: AgoraRteMicrophoneAudioTrackProtocol!
    @IBOutlet weak var container: AGEVideoContainer!
    @IBOutlet weak var recordingVolumeSlider: UISlider!
    @IBOutlet weak var playbackVolumeSlider: UISlider!
    @IBOutlet weak var inEarMonitoringSwitch: UISwitch!
    var audioViews: [UInt:VideoView] = [:]
    let LOCAL_STREAM_ID = String(UInt.random(in: 1000...2000))
    
    
    // indicate if current instance has joined channel
    var isJoined: Bool = false
    
    override func viewDidLoad(){
        super.viewDidLoad()
        
        // get channel name from configs
        guard let channelName = configs["channelName"] as? String
            else { return }
        
        // layout render view
        recordingVolumeSlider.maximumValue = 400
        recordingVolumeSlider.minimumValue = 0
        recordingVolumeSlider.integerValue = 100
        
        playbackVolumeSlider.maximumValue = 400
        playbackVolumeSlider.minimumValue = 0
        playbackVolumeSlider.integerValue = 100
        
        let view = Bundle.loadVideoView(type: .local, audioOnly: true)
        view.uid = 0
        view.setPlaceholder(text: getAudioLabel(uid: 0, isLocal: true))
        audioViews[0] = view
        
        // initialize sdk
        let profile = AgoraRteSdkProfile()
        profile.appid = KeyCenter.AppId
        agoraKit = AgoraRteSdk.sharedEngine(with: profile)
        
        // initialize media control
        let mediaControl = agoraKit.rteMediaFactory()
        
        // audio
        microphoneTrack = mediaControl?.createMicrophoneAudioTrack()
        microphoneTrack?.startRecording()
        
        //initilize streaming control
        scene = agoraKit.createRteScene(withSceneId: channelName, sceneConfig: AgoraRteSceneConfg())
        scene?.setSceneDelegate(self)
        scene?.setAudioEncoderConfiguration(LOCAL_STREAM_ID, audioEncoderConfiguration: AgoraRteAudioEncoderConfiguration())
        scene.joinScene(withUserId: LOCAL_STREAM_ID, token: "", joinOptions: AgoraRteJoinOptions())
        let streamOption = AgoraRteRtcStreamOptions()
        streamOption.token = ""
        scene?.createOrUpdateRTCStream(LOCAL_STREAM_ID, rtcStreamOptions: streamOption)
        scene?.publishLocalAudioTrack(LOCAL_STREAM_ID, rteAudioTrack: microphoneTrack!)        

    }
    
    override func willMove(toParent parent: UIViewController?) {
        if parent == nil {
            // leave channel when exiting the view
            if isJoined {
                scene.leave()
            }
        }
    }
    
    func sortedViews() -> [VideoView] {
        return Array(audioViews.values).sorted(by: { $0.uid < $1.uid })
    }
    
    @IBAction func onChangeRecordingVolume(_ sender:UISlider){
        let value:Int = Int(sender.value)
        print("adjustRecordingSignalVolume \(value)")
        microphoneTrack.adjustPublishVolume(value)
    }
    
    @IBAction func onChangePlaybackVolume(_ sender:UISlider){
        let value:Int = Int(sender.value)
        print("adjustPlaybackSignalVolume \(value)")
        microphoneTrack.adjustPlayoutVolume(value)
    }
    
    @IBAction func toggleInEarMonitoring(_ sender:UISwitch){
//        enum EAR_MONITORING_FILTER_TYPE {
//          /**
//           * 1: Do not add an audio filter to the in-ear monitor.
//           */
//          EAR_MONITORING_FILTER_NONE = (1<<0),
//          /**
//           * 2: Enable audio filters to the in-ear monitor.
//           */
//          EAR_MONITORING_FILTER_BUILT_IN_AUDIO_FILTERS = (1<<1),
//          /**
//           * 4: Enable noise suppression to the in-ear monitor.
//           */
//          EAR_MONITORING_FILTER_NOISE_SUPPRESSION = (1<<2)
//        };
        microphoneTrack.enableEarMonitor(sender.isOn, audioFilters: 1)
    }
}

extension BasicAudioMain: AgoraRteSceneDelegate {
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, remoteUserDidJoinWithUserInfos userInfos: [AgoraRteStreamInfo]?) {
        print("on remoteUserDidJoin")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, remoteStreamesDidAddWith streamInfos: [AgoraRteStreamInfo]?) {
        guard let infos = streamInfos else { return }
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
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, connectionStateDidChange oldState: AgoraConnectionState, newState state: AgoraConnectionState, changedReason reason: AgoraConnectionChangedReason) {
        print("didConnectionStateChanged state:\(state.rawValue) reason:\(reason.rawValue)")
    }
    
    func agoraRteScene(_ rteScene: AgoraRteSceneProtocol, localStreamStateDidChange streams: AgoraRteStreamInfo?, mediaType: AgoraRteMediaType, steamMediaState oldState: AgoraRteStreamState, newState: AgoraRteStreamState, stateChangedReason reason: AgoraRteStreamStateChangedReason) {
        print("didLocalStreamStateChanged \(String(describing: streams?.streamId)), audio sentBitrate: \(String(describing: newState))")
    }
}
