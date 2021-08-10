//
//  MediaInfo.swift
//  OpenVideoCall
//
//  Created by GongYuhua on 4/11/16.
//  Copyright © 2016 Agora. All rights reserved.
//

import Foundation
import AgoraRTE

struct StatisticsInfo {
    struct LocalInfo {
        var sceneStats = AgoraRteSceneStats()
        var videoStats = AgoraRteLocalStreamStats()
        var audioStats = AgoraRteLocalStreamStats()
    }
    
    struct RemoteInfo {
        var videoStats = AgoraRteRemoteStreamStats()
        var audioStats = AgoraRteRemoteStreamStats()
    }
    
    enum StatisticsType {
        case local(LocalInfo), remote(RemoteInfo)
        
        var isLocal: Bool {
            switch self {
            case .local:  return true
            case .remote: return false
            }
        }
    }
    
    var dimension = CGSize.zero
    var fps:UInt = 0
    
    var type: StatisticsType
    
    init(type: StatisticsType) {
        self.type = type
    }
    
    mutating func updateChannelStats(_ stats: AgoraRteSceneStats) {
        guard self.type.isLocal else {
            return
        }
        switch type {
        case .local(let info):
            var new = info
            new.sceneStats = stats
            self.type = .local(new)
        default:
            break
        }
    }
    
    mutating func updateLocalVideoStats(_ stats: AgoraRteLocalStreamStats) {
        guard self.type.isLocal, let videoStats = stats.videoStats else {
            return
        }
        switch type {
        case .local(let info):
            var new = info
            new.videoStats = stats
            self.type = .local(new)
        default:
            break
        }
        dimension = CGSize(width: Int(videoStats.encodedFrameWidth), height: Int(videoStats.encodedFrameHeight))
        fps = UInt(videoStats.encoderOutputFrameRate)
    }
    
    mutating func updateLocalAudioStats(_ stats: AgoraRteLocalStreamStats) {
        guard self.type.isLocal else {
            return
        }
        switch type {
        case .local(let info):
            var new = info
            new.audioStats = stats
            self.type = .local(new)
        default:
            break
        }
    }
    
    mutating func updateVideoStats(_ stats: AgoraRteRemoteStreamStats) {
        switch type {
        case .remote(let info):
            var new = info
            new.videoStats = stats
            dimension = CGSize(width: Int(stats.videoStats!.width), height: Int(stats.videoStats!.height))
            fps = UInt(stats.videoStats!.rendererOutputFrameRate)
            self.type = .remote(new)
        default:
            break
        }
    }
    
    mutating func updateAudioStats(_ stats: AgoraRteRemoteStreamStats) {
        switch type {
        case .remote(let info):
            var new = info
            new.audioStats = stats
            self.type = .remote(new)
        default:
            break
        }
    }
    
    func description(audioOnly:Bool) -> String {
        var full: String
        switch type {
        case .local(let info):  full = localDescription(info: info, audioOnly: audioOnly)
        case .remote(let info): full = remoteDescription(info: info, audioOnly: audioOnly)
        }
        return full
    }
    
    func localDescription(info: LocalInfo, audioOnly: Bool) -> String {
        
        let dimensionFps = "\(Int(dimension.width))×\(Int(dimension.height)),\(fps)fps"
        
//        let lastmile = "LM Delay: \(info.sceneStats.del)ms"
//        let videoSend = "VSend: \(info.videoStats.videoStats.)kbps"
        let audioSend = "ASend: \(info.audioStats.audioStats?.sentBitrate ?? 0)kbps"
        let cpu = "CPU: \(info.sceneStats.cpuAppUsage)%/\(info.sceneStats.cpuTotalUsage)%"
        //TODO
//        let vSendLoss = "VSend Loss: \(info.videoStats.txPacketLossRate)%"
//        let aSendLoss = "ASend Loss: \(info.audioStats.txPacketLossRate)%"
        let vSendLoss = "VSend Loss: MISSING%"
        let aSendLoss = "ASend Loss: MISSING%"
        
        if(audioOnly) {
            return [audioSend,cpu,aSendLoss].joined(separator: "\n")
        }
        return [dimensionFps,audioSend,cpu,vSendLoss,aSendLoss].joined(separator: "\n")
    }
    
    func remoteDescription(info: RemoteInfo, audioOnly: Bool) -> String {
        guard info.videoStats.videoStats != nil && info.audioStats.audioStats != nil else {
            return ""
        }
        let dimensionFpsBit = "\(Int(dimension.width))×\(Int(dimension.height)), \(fps)fps"
        
        var audioQuality: AgoraNetworkQuality
        if let quality = AgoraNetworkQuality(rawValue: UInt(info.audioStats.audioStats!.quality)) {
            audioQuality = quality
        } else {
            audioQuality = AgoraNetworkQuality.unknown
        }
        
        let videoRecv = "VRecv: \(info.videoStats.videoStats!.receivedBitrate)kbps"
        let audioRecv = "ARecv: \(info.audioStats.audioStats!.receivedBitrate)kbps"
        
        let videoLoss = "VLoss: \(info.videoStats.videoStats!.packetLossRate)%"
        let audioLoss = "ALoss: \(info.audioStats.audioStats!.audioLossRate)%"
        let aquality = "AQuality: \(audioQuality.description())"
        if(audioOnly) {
            return [audioRecv,audioLoss,aquality].joined(separator: "\n")
        }
        return [dimensionFpsBit,videoRecv,audioRecv,videoLoss,audioLoss,aquality].joined(separator: "\n")
    }
}
