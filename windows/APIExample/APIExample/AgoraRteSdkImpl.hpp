// ======================= AgoraRteVideoDeviceManager.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteDeviceManager.h"

namespace agora {
namespace rte {
#if RTE_WIN || RTE_MAC
class AgoraRteVideoDeviceManager : public IAgoraRteVideoDeviceManager {
 public:
  AgoraRteVideoDeviceManager(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  virtual std::vector<VideoDeviceInfo> EnumerateVideoDevices() override;

 private:
  agora::agora_refptr<rtc::ICameraCapturer> camera_capturer_;
};

#endif
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteUtils.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteMediaTrack.h"

namespace agora {
namespace rte {

class AgoraRteRtcVideoTrackBase;
class AgoraRteRtcAudioTrackBase;
class IAgoraRteVideoTrack;
class IAgoraRteAudioTrack;
struct RteRtcStreamInfo;

class AgoraRteUtils {
 public:
  template <typename EventHandlerT, typename MutexT>
  static void NotifyEventHandlers(
      MutexT& mutex, std::vector<std::weak_ptr<EventHandlerT>>& handlers,
      std::function<void(const std::shared_ptr<EventHandlerT>&)> task) {
    std::vector<std::shared_ptr<EventHandlerT>> shared_handles;

    {
      std::lock_guard<MutexT> _(mutex);

      handlers.erase(
          std::remove_if(
              handlers.begin(), handlers.end(),
              [&shared_handles](auto& hanlder) {
                bool should_remove = true;
                auto handler_shared = hanlder.lock();
                if (handler_shared) {
                  should_remove = false;
                  shared_handles.push_back(std::move(
                      std::static_pointer_cast<EventHandlerT>(handler_shared)));
                }

                return should_remove;
              }),
          handlers.end());
    }

    std::for_each(shared_handles.begin(), shared_handles.end(),
                  std::move(task));
  }

  template <typename EventHandlerT, typename MutexT>
  static void UnregisterSharedPtrFromContainer(
      MutexT& mutex, std::vector<std::weak_ptr<EventHandlerT>>& handlers,
      const std::shared_ptr<EventHandlerT>& shared_handles) {
    {
      std::lock_guard<MutexT> _(mutex);

      handlers.erase(
          std::remove_if(handlers.begin(), handlers.end(),
                         [&shared_handles](auto& hanlder) {
                           bool should_remove = true;
                           auto handler_shared = hanlder.lock();
                           if (handler_shared &&
                               handler_shared.get() != shared_handles.get()) {
                             should_remove = false;
                           }

                           return should_remove;
                         }),
          handlers.end());
    }
  }
  
  template <typename ObjectType>
  static std::shared_ptr<ObjectType> AgoraRefObjectToSharedObject(
      agora_refptr<ObjectType>& agora_object) {
    agora_object->AddRef();
    std::shared_ptr<ObjectType> result(agora_object.get(), [](ObjectType* obj) {
      if (obj) obj->Release();
    });
    return result;
  }

  static std::string GenerateRtcStreamId(bool is_major_stream,
                                         const std::string& user_id,
                                         const std::string& stream_id);

  template <typename... Args>
  static std::string GeneratorJsonUserId(const std::string& format,
                                         Args&&... args) {
    auto size =
        std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args)...);
    std::string output(size + 1, '\0');
    std::sprintf(&output[0], format.c_str(), std::forward<Args>(args)...);
    return output;
  }

  static bool ExtractRtcStreamInfo(const std::string& rtc_stream_id,
                                   RteRtcStreamInfo& info);

  static ConnectionState GetConnStateFromRtcState(
      rtc::CONNECTION_STATE_TYPE state);

  static std::shared_ptr<AgoraRteRtcVideoTrackBase> CastToImpl(
      std::shared_ptr<IAgoraRteVideoTrack> track);

  static std::shared_ptr<AgoraRteRtcAudioTrackBase> CastToImpl(
      std::shared_ptr<IAgoraRteAudioTrack> track);

  private:
  static bool SplitKeyPairInRtcStream(
       const std::string& key_pair, std::map<std::string, std::string>& store);

};  // namespace rte

template <typename T>
class RtcCallbackWrapper : public T {
 public:
  using Type = std::shared_ptr<RtcCallbackWrapper<T>>;

  template <typename... Args>
  static Type Create(Args&&... args) {
    std::shared_ptr<RtcCallbackWrapper<T>> result(
        new RtcCallbackWrapper<T>(std::forward<Args>(args)...));
    // circular reference on purpose, function "DeletMe" should be passed to RTC
    // as observer's safe deleter
    result->shared_me_ = result;
    return result;
  }

  virtual ~RtcCallbackWrapper() = default;

  // Note: This interface can only be invoked from inner callback deleter.
  void DeleteMe() { shared_me_.reset(); }

 private:
  template <typename... Args>
  explicit RtcCallbackWrapper(Args&&... args)
      : T(std::forward<Args>(args)...) {}

 private:
  // shared_me_ can only be invoked form inner callback thread.
  std::shared_ptr<RtcCallbackWrapper<T>> shared_me_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteStream.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//
#pragma once
#include <mutex>
#include <string>

#include "AgoraRteBase.h"
#include "IAgoraRteMediaTrack.h"

namespace agora {
namespace rte {

class AgoraRteRtcAudioTrackBase;
class AgoraRteRtcVideoTrackBase;

class AgoraRteStream {
 public:
  AgoraRteStream(const std::string& user_id, const std::string& stream_id,
                 StreamType stream_type)
      : stream_id_(stream_id), user_id_(user_id), stream_type_(stream_type) {}

  const std::string& GetStreamId() { return stream_id_; }

  const std::string& GetUserId() { return user_id_; }

  bool ContainsAudio() { return contains_audio_; }

  bool ContainsVideo() { return contains_video_; }

  void OnAudioConnected() {
    RTE_LOG_VERBOSE << " stream id: " << stream_id_;
    contains_audio_ = true;
  }

  void OnVideoConnected() {
    RTE_LOG_VERBOSE << " stream id: " << stream_id_;
    contains_video_ = true;
  }

  void OnAudioDisconnected() {
    RTE_LOG_VERBOSE << " stream id: " << stream_id_ ;
    contains_audio_ = false;
  }

  void OnVideoDisconnected() {
    RTE_LOG_VERBOSE << " stream id: " << stream_id_;
    contains_video_ = false;
  }

  StreamType GetStreamType() { return stream_type_; }

 protected:
  std::atomic<bool> contains_audio_ = {false};
  std::atomic<bool> contains_video_ = {false};
  std::string stream_id_;
  std::string user_id_;
  StreamType stream_type_;
};

class AgoraRteMajorStream : public std::enable_shared_from_this<AgoraRteMajorStream>,
                            public AgoraRteStream {
 public:
  AgoraRteMajorStream(const std::string& user_id,
                      StreamType stream_type)
      : AgoraRteStream(user_id, "", stream_type) {}
  virtual ~AgoraRteMajorStream() = default;
  virtual int Connect() = 0;
  virtual void Disconnect() = 0;

  virtual int PushUserInfo(const UserInfo& info, InstanceState state) = 0;
  virtual int PushStreamInfo(const StreamInfo& info, InstanceState state) = 0;
  virtual int PushMediaInfo(const StreamInfo& info, MediaType media_type, InstanceState state) = 0;

  // TODO: Push in batch
  //
};

class AgoraRteLocalStream : public std::enable_shared_from_this<AgoraRteLocalStream>,
                            public AgoraRteStream {
 public:
  AgoraRteLocalStream(const std::string& user_id, const std::string& stream_id,
                      StreamType stream_type)
      : AgoraRteStream(user_id, stream_id, stream_type) {}

  virtual ~AgoraRteLocalStream() = default;

  // Expose Connect/Disconnect here , for rtc, if we failed to connect, no callback will
  // be trigged, so we expose this to scene to handle error there, meanwhile, exposing
  // these functions hurts nothing, streams could check their internal status and do
  // whatever they want.
  //
  virtual int Connect() = 0;

  virtual void Disconnect() = 0;

  // Expose internal track state to scene, customer might want to query such status to tell
  // whether stream contains aduio or video tracks
  //
  virtual size_t GetAudioTrackCount() = 0;

  virtual size_t GetVideoTrackCount() = 0;

  // Update stream options, e.g. token , url
  //
  virtual int UpdateOption(const StreamOption& option) = 0;

  // Publish/Unpublish tracks
  //
  virtual int PublishLocalAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track) = 0;

  virtual int PublishLocalVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track) = 0;

  virtual int UnpublishLocalAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track) = 0;

  virtual int UnpublishLocalVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track) = 0;

  // Set audio/video encoder configuration
  //
  virtual int SetAudioEncoderConfiguration(const AudioEncoderConfiguration& config) = 0;

  virtual int SetVideoEncoderConfiguration(const VideoEncoderConfiguration& config) = 0;

  // Enable or disable audio observer, scene will call this when customer registed related
  // observer to scene, or disable observer when none observer is registed in scene
  //
  virtual int EnableLocalAudioObserver(const LocalAudioObserverOptions& option) = 0;
  
  virtual int DisableLocalAudioObserver() = 0;

  virtual int SetBypassCdnTranscoding(const agora::rtc::LiveTranscoding& transcoding) = 0;

  virtual int AddBypassCdnUrl(const std::string& target_cdn_url, bool transcoding_enabled) = 0;

  virtual int RemoveBypassCdnUrl(const std::string& target_cdn_url) = 0;
};

class AgoraRteRemoteStream : public std::enable_shared_from_this<AgoraRteRemoteStream>,
                             public AgoraRteStream {
 public:
  AgoraRteRemoteStream(const std::string& user_id, const std::string& stream_id,
                       StreamType stream_type)
      : AgoraRteStream(user_id, stream_id, stream_type) {}

  virtual ~AgoraRteRemoteStream() = default;

  // Subscribe/Unsubscribe tracks
  //
  virtual int SubscribeRemoteAudio() = 0;

  virtual int UnsubscribeRemoteAudio() = 0;

  virtual int SubscribeRemoteVideo(const VideoSubscribeOptions& options) = 0;

  virtual int UnsubscribeRemoteVideo() = 0;

  // Set remote video canvas
  //
  virtual int SetRemoteVideoCanvas(const VideoCanvas& canvas) = 0;

  virtual int AdjustRemoteVolume(int volume) = 0;

  virtual int GetRemoteVolume(int* volume) = 0;

  // Enable or disable audio/video observer, scene will call this when customer registed
  // related observer to scene, or disable observer when none observer is registed in scene
  //
  virtual int EnableRemoteVideoObserver() = 0;

  virtual int DisableRemoveVideoObserver() = 0;

  virtual int EnableRemoteAudioObserver(const RemoteAudioObserverOptions& option) = 0;

  virtual int DisableRemoteAudioObserver() = 0;
};

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMediaFactory.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "IAgoraRteMediaFactory.h"

namespace agora {
namespace rte {

class IAgoraRteAudioTrack;
class IAgoraRteVideoTrack;

class AgoraRteMediaFactory final
    : public std::enable_shared_from_this<AgoraRteMediaFactory>,
      public IAgoraRteMediaFactory {
 public:
  AgoraRteMediaFactory(std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteMediaFactory() = default;

  std::shared_ptr<IAgoraRteCameraVideoTrack> CreateCameraVideoTrack() override;

  std::shared_ptr<IAgoraRteScreenVideoTrack> CreateScreenVideoTrack() override;

  std::shared_ptr<IAgoraRteMixedVideoTrack> CreateMixedVideoTrack() override;

  std::shared_ptr<IAgoraRteCustomVideoTrack> CreateCustomVideoTrack() override;

  std::shared_ptr<IAgoraRteMicrophoneAudioTrack> CreateMicrophoneAudioTrack()
      override;

  std::shared_ptr<IAgoraRteCustomAudioTrack> CreateCustomAudioTrack() override;

  std::shared_ptr<IAgoraRteMediaPlayer> CreateMediaPlayer() override;

  std::shared_ptr<IAgoraRteStreamingSource> CreateStreamingSource() override;

 private:
  std::shared_ptr<agora::base::IAgoraService> rtc_services_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteTrackImplBase.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"

namespace agora {
namespace rte {
class AgoraRteTrackImplBase {
 public:
  AgoraRteTrackImplBase() : stream_id_("") {}

  std::shared_ptr<rtc::IMediaNodeFactory> GetMediaNodeFactory() {
    return media_node_factory_;
  }

  void SetStreamId(const std::string& stream_id) { stream_id_ = stream_id; }

  const std::string& GetStreamId() const { return stream_id_; }

  virtual ~AgoraRteTrackImplBase() = default;

  virtual int Start() = 0;
  virtual void Stop() = 0;

 protected:
  std::mutex track_lock_;
  std::shared_ptr<agora::base::IAgoraService> rtc_service_;
  std::shared_ptr<rtc::IMediaNodeFactory> media_node_factory_;
  bool is_started_ = false;
  std::string stream_id_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRtePlayList.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//
#pragma once

#include <list>
#include <memory>

#include "AgoraRefPtr.h"
#include "AgoraRteBase.h"
#include "IAgoraMediaStreamingSource.h"
#include "IAgoraRtePlayList.h"

namespace agora {
namespace rte {

class AgoraRtePlayList : public IAgoraRtePlayList {
 public:
  AgoraRtePlayList();
  virtual ~AgoraRtePlayList();

  //////////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of IAgoraRtePlayList //////////////////
  ///////////////////////////////////////////////////////////////////////////
  int ClearFileList() override;
  int32_t GetFileCount() override;
  int64_t GetTotalDuration() override;
  int GetFileInfoById(int32_t file_id, RteFileInfo& out_file_info) override;
  int GetFileInfoByIndex(int32_t file_index,
                         RteFileInfo& out_file_info) override;
  int GetFirstFileInfo(RteFileInfo& first_file_info) override;
  int GetLastFileInfo(RteFileInfo& last_file_info) override;
  int GetFileList(std::vector<RteFileInfo>& out_file_list) override;

  int InsertFile(const char* file_url, int32_t insert_index,
                 RteFileInfo& out_file_info) override;
  int AppendFile(const char* file_url, RteFileInfo& out_file_info) override;

  int RemoveFileById(int32_t remove_file_id) override;
  int RemoveFileByIndex(int32_t remove_file_index) override;
  int RemoveFileByUrl(const char* remove_file_url) override;

  ///////////////////////////////////////////////////////////////
  ///////////////// Only public for RTE Internal ///////////////
  ///////////////////////////////////////////////////////////////
  int SetCurrFileById(int32_t file_id);
  int SetCurrFileByListTime(int64_t time_in_list, int64_t& out_time_in_file);
  bool CurrentIsFirstFile();
  bool CurrentIsLastFile();
  int GetCurrentFileInfo(RteFileInfo& out_curr_file_info);
  int MoveCurrentToPrev(bool list_loop);
  int MoveCurrentToNext(bool list_loop);
  RteFileInfoSharePtr FindPrevFile(bool list_loop);
  RteFileInfoSharePtr FindNextFile(bool list_loop);

 protected:
  int ParseMediaInfo(const char* file_url,
                     media::base::PlayerStreamInfo& video_info,
                     media::base::PlayerStreamInfo& audio_info);
  RteFileInfoSharePtr FindFileById(int32_t find_file_id);
  RteFileInfoSharePtr FindFileByIndex(int32_t find_file_index);
  void FindFileByUrl(const char* find_file_url,
                     std::vector<RteFileInfoSharePtr>& file_info_list);
  void UpdateFileList(int64_t& out_total_duration);

  static int32_t GetNextFileId() {
    static std::atomic<int32_t> global_file_id_ = {0};
    return global_file_id_++;
  }

 protected:
  static std::atomic<int32_t>
      global_file_id_;  ///< Global increase-self file Id
  agora_refptr<agora::rtc::IMediaStreamingSource> streaming_source_ = nullptr;
  std::recursive_mutex list_lock_;              ///< locker for AgoraRtePlayList
  std::list<RteFileInfoSharePtr> file_list_;    ///< file list
  RteFileInfoSharePtr current_file_ = nullptr;  ///< current file
  int64_t total_duration_ = 0;                  ///< total duration
};

}  // namespace rte
}  // namespace agora
// ======================= AgoraRteMediaPlayer.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraMediaPlayer.h"
#include "IAgoraMediaPlayerSource.h"
#include "IAgoraRteMediaPlayer.h"

namespace agora {
namespace rte {

class AgoraRteMediaPlayer;
class AgoraRteScene;
class AgoraRtePlayList;

class AgoraRteMediaPlayerSrcObserver : public rtc::IMediaPlayerSourceObserver {
 public:
  AgoraRteMediaPlayerSrcObserver(
      std::shared_ptr<AgoraRteMediaPlayer> media_player)
      : media_player_(media_player) {}

  void onPlayerSourceStateChanged(media::base::MEDIA_PLAYER_STATE state,
                                  media::base::MEDIA_PLAYER_ERROR ec) override;

  void onPositionChanged(int64_t position) override;

  void onPlayerEvent(media::base::MEDIA_PLAYER_EVENT event) override;

  void onMetaData(const void* data, int length) override;

  void onCompleted() override;

  void onPlayBufferUpdated(int64_t playCachedBuffer) override;

 private:
  std::weak_ptr<AgoraRteMediaPlayer> media_player_;
};

class AgoraRteMediaPlayerAudioObserver
    : public agora::media::base::IAudioFrameObserver {
 public:
  AgoraRteMediaPlayerAudioObserver(
      std::shared_ptr<AgoraRteMediaPlayer> media_player)
      : media_player_(media_player) {}

  void onFrame(AudioPcmFrame* frame) override;

 private:
  std::weak_ptr<AgoraRteMediaPlayer> media_player_;
};

class AgoraRteMediaPlayerVideoObserver
    : public agora::media::base::IVideoFrameObserver {
 public:
  AgoraRteMediaPlayerVideoObserver(
      std::shared_ptr<AgoraRteMediaPlayer> media_player)
      : media_player_(media_player) {}

  void onFrame(const VideoFrame* frame) override;

 private:
  std::weak_ptr<AgoraRteMediaPlayer> media_player_;
};

class AgoraRteMediaPlayer final
    : public std::enable_shared_from_this<AgoraRteMediaPlayer>,
      public IAgoraRteMediaPlayer {
  friend class AgoraRteMediaPlayerSrcObserver;
  friend class AgoraRteMediaPlayerAudioObserver;
  friend class AgoraRteMediaPlayerVideoObserver;

 public:
  AgoraRteMediaPlayer(std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteMediaPlayer();

  std::shared_ptr<IAgoraRtePlayList> CreatePlayList() override;

  int Open(const std::string& url, int64_t start_pos) override;

  int Open(std::shared_ptr<IAgoraRtePlayList> in_play_list,
           int64_t start_pos) override;

  int Play() override;

  int Pause() override;

  int Resume() override;

  int Stop() override;

  int Seek(int64_t pos) override;

  int SeekToPrev(int64_t pos) override;

  int SeekToNext(int64_t pos) override;

  int SeekToFile(int32_t file_id, int64_t pos) override;

  int ChangePlaybackSpeed(MEDIA_PLAYER_PLAYBACK_SPEED speed) override;

  int AdjustPlayoutVolume(int volume) override;

  int Mute(bool mute) override;

  int SelectAudioTrack(int index) override;

  int SetLoopCount(int loop_count) override;

  int MuteAudio(bool audio_mute) override;

  bool IsAudioMuted() override;

  int MuteVideo(bool video_mute) override;

  bool IsVideoMuted() override;

  int SetView(View view) override;

  int SetRenderMode(RENDER_MODE_TYPE mode) override;

  int SetPlayerOption(const std::string& key, int value) override;

  int SetPlayerOptionString(const std::string& key,
                            const std::string& value) override;

  int GetPlayerStatus(RtePlayerStatus& out_playing_status) override;

  MEDIA_PLAYER_STATE GetState() override;

  int GetDuration(int64_t& duration) override;

  int GetPlayPosition(int64_t& pos) override;

  int GetPlayoutVolume(int& volume) override;

  int GetStreamCount(int64_t& count) override;

  int GetStreamInfo(int64_t index, PlayerStreamInfo& info) override;

  bool IsMuted() override;

  const char* GetPlayerVersion() override;

  int SetStreamId(const std::string& stream_id) override;

  int GetStreamId(std::string& stream_id) const override;

  int RegisterMediaPlayerObserver(
      std::shared_ptr<IAgoraRteMediaPlayerObserver> observer) override;

  int UnregisterMediaPlayerObserver(
      std::shared_ptr<IAgoraRteMediaPlayerObserver> observer) override;

  std::shared_ptr<IAgoraRteAudioTrack> GetAudioTrack() override;

  std::shared_ptr<IAgoraRteVideoTrack> GetVideoTrack() override;

  void GetCurrFileInfo(RteFileInfo& out_current_file);
  int ProcessOpened();
  int ProcessOneMediaCompleted();

 protected:
  std::shared_ptr<agora::base::IAgoraService> rtc_service_;

  std::shared_ptr<IAgoraRteAudioTrack> audio_track_;
  std::shared_ptr<IAgoraRteVideoTrack> video_track_;

  std::string stream_id_;
  std::shared_ptr<rtc::IMediaPlayer> media_player_;
  std::vector<std::weak_ptr<IAgoraRteMediaPlayerObserver>> media_player_obs_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteMediaPlayerSrcObserver>>
      internal_media_play_src_ob_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteMediaPlayerAudioObserver>>
      internal_media_play_audio_ob_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteMediaPlayerVideoObserver>>
      internal_media_play_video_ob_;
  std::mutex media_player_lock_;

  std::recursive_mutex file_manager_lock_;  ///< locker for file manager
  std::shared_ptr<AgoraRtePlayList> play_list_ = nullptr;  ///< play list
  RteFileInfo current_file_;  ///< current file information
  int32_t first_file_id_ =
      INVALID_RTE_FILE_ID;           ///< The file id of start file in list
  int64_t setting_loop_count_ = 1;   ///< default play count is only once
  bool setting_mute_ = false;        ///< default play is unmute
  int64_t list_played_count_ = 0;    ///< the played count of list
  bool auto_play_for_open_ = false;  ///< whether open for play list
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteStreamFactory.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteScene.h"

namespace agora {
namespace rte {

class AgoraRteLocalStream;
class AgoraRteScene;
class AgoraRteMajorStream;

class AgoraRteStreamFactory {
 public:
  AgoraRteStreamFactory(
      const std::string& appid,
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  std::shared_ptr<AgoraRteLocalStream> CreateLocalStream(
      std::shared_ptr<AgoraRteScene> scene, const StreamOption& option,
      const std::string& stream_id);

  std::shared_ptr<AgoraRteMajorStream> CreateMajorStream(
      std::shared_ptr<AgoraRteScene> scene, const std::string& token,
      const JoinOptions& option);

 protected:
  std::string appid_;
  std::shared_ptr<agora::base::IAgoraService> rtc_service_;
};
}  // namespace rte
}  // namespace agora
// ======================= AgoraRteSdk.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteDeviceManager.h"
#include "IAgoraRteMediaFactory.h"
#include "IAgoraRteScene.h"

namespace agora {
namespace rte {

class AgoraRteStreamFactory;
using AgoraServiceCreatorFunctionPtr =
    agora::base::IAgoraService*(AGORA_CALL*)();

class AgoraRteSDK {
 public:
  static int Init(const SdkProfile& profile,
                  bool enable_rtc_compatible_mode = false);

  static void Deinit();

  static std::shared_ptr<IAgoraRteScene> CreateRteScene(
      const std::string& scene_id, const SceneConfig& config);

  static void SetAgoraServiceCreator(AgoraServiceCreatorFunctionPtr func);

  static std::shared_ptr<IAgoraRteMediaFactory> GetRteMediaFactory();

  static std::shared_ptr<agora::base::IAgoraService> GetRtcService();

  static int GetProfile(SdkProfile& out_profile);

#if RTE_WIN || RTE_MAC
  static std::shared_ptr<IAgoraRteAudioDeviceManager> GetAudioDeviceManager();

  static std::shared_ptr<IAgoraRteVideoDeviceManager> GetVideoDeviceManager();
#endif

 private:
  static std::shared_ptr<AgoraRteSDK> GetSdkInstance();
  static std::mutex& GetLock();

 private:
  AgoraRteSDK() = default;

  int SetProfileInternal(const SdkProfile& profile,
                         bool enable_rtc_compatible_mode);
  int GetProfileInternal(SdkProfile& out_profile);

#if RTE_WIN || RTE_MAC
  std::shared_ptr<IAgoraRteAudioDeviceManager> GetRteAudioDeviceManager();

  std::shared_ptr<IAgoraRteVideoDeviceManager> GetRteVideoDeviceManager();
#endif

  std::shared_ptr<IAgoraRteMediaFactory> GetRteMediaFactoryInternal();

  void SetServiceCreator(AgoraServiceCreatorFunctionPtr func);

  std::shared_ptr<agora::base::IAgoraService> GetRtcServiceInternal();

  std::shared_ptr<AgoraRteStreamFactory> GetStreamFactory();

  void DeinitInternal();

  bool IsCompatibleModeEnabled() { return enable_rtc_compatible_mode_; }

 private:
  std::shared_ptr<agora::base::IAgoraService> rtc_service_ = nullptr;
  std::shared_ptr<IAgoraRteMediaFactory> media_factory_ = nullptr;

#if RTE_WIN || RTE_MAC
  std::shared_ptr<IAgoraRteVideoDeviceManager> video_device_mgr_ = nullptr;
  std::shared_ptr<IAgoraRteAudioDeviceManager> audio_device_mgr_ = nullptr;
#endif

  std::shared_ptr<AgoraRteStreamFactory> stream_factory_ = nullptr;

  SdkProfile profile_;

  bool enable_rtc_compatible_mode_ = false;

  AgoraServiceCreatorFunctionPtr creator_ = nullptr;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteAudioDeviceManager.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteDeviceManager.h"
#include "IAudioDeviceManager.h"

namespace agora {
namespace rte {

// The implementation is only for WIN and MAC
//
#if RTE_WIN || RTE_MAC

class AgoraRteAudioDeviceCollection : public IAgoraRteAudioDeviceCollection {
 public:
  explicit AgoraRteAudioDeviceCollection(
      rtc::IAudioDeviceCollection* collection);
  int GetCount() override;
  int GetDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH],
                char deviceId[MAX_DEVICE_ID_LENGTH]) override;
  int SetDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) override;
  int SetApplicationVolume(int volume) override;
  int GetApplicationVolume(int& volume) override;
  int MuteApplication(bool mute) override;
  int IsApplicationMuted(bool& mute) override;
  void Release() override;

 private:
  rtc::IAudioDeviceCollection* collection_;
};

class AgoraRteAudioDeviceManager final : public IAgoraRteAudioDeviceManager {
 public:
  explicit AgoraRteAudioDeviceManager(
      base::IAgoraService* service, rtc::IAudioDeviceManagerObserver* observer);
  virtual ~AgoraRteAudioDeviceManager();

  IAgoraRteAudioDeviceCollection* EnumeratePlaybackDevices() override;
  IAgoraRteAudioDeviceCollection* EnumerateRecordingDevices() override;

  int SetPlaybackDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) override;
  int GetPlaybackDevice(char deviceId[MAX_DEVICE_ID_LENGTH]) override;

  int SetPlaybackDeviceVolume(int volume) override;
  int GetPlaybackDeviceVolume(int* volume) override;

  int SetRecordingDevice(const char deviceId[MAX_DEVICE_ID_LENGTH]) override;
  int GetRecordingDevice(char deviceId[MAX_DEVICE_ID_LENGTH]) override;

  int SetRecordingDeviceVolume(int volume) override;
  int GetRecordingDeviceVolume(int* volume) override;

  int MutePlaybackDevice(bool mute) override;
  int IsPlaybackDeviceMuted(bool& muted) override;

  int MuteRecordingDevice(bool mute) override;
  int IsRecordingDeviceMuted(bool& muted) override;

  int StartPlaybackDeviceTest(const char* testAudioFilePath) override;
  int StopPlaybackDeviceTest() override;

  int StartRecordingDeviceTest(int indicationInterval) override;
  int StopRecordingDeviceTest() override;

  int StartDeviceLoopbackTest(int indicationInterval) override;
  int StopDeviceLoopbackTest() override;

 private:
  agora_refptr<rtc::IAudioDeviceManager> audio_device_manager_;
  IAgoraRteSceneEventHandler* event_handler_ = nullptr;
};

#endif
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteTrackBase.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteMediaTrack.h"

namespace agora {
namespace rte {

class AgoraRteLocalStream;
class AgoraRteTrackImplBase;
class AgoraRteVideoTrackImpl;
class AgoraRteAudioTrackImpl;

// This class is to define private interface functions, we don't want to define
// these functions in IAgoraRteXXXX.h.
//
class AgoraRteTrackBase
    : public std::enable_shared_from_this<AgoraRteTrackBase> {
 public:
  AgoraRteTrackBase();

  virtual ~AgoraRteTrackBase() = default;

  virtual void Init() {}

  virtual void SetStreamId(const std::string& stream_id) = 0;

  PublishState GetTrackPublishState() { return track_pub_stat_; }

  // These functions are to change track states since we don't want to expose
  // state change to child class.
  //
  virtual int BeforePublish(const std::shared_ptr<AgoraRteLocalStream>& stream);

  virtual void AfterPublish(int result,
                            const std::shared_ptr<AgoraRteLocalStream>& stream);

  virtual int BeforeUnPublish(
      const std::shared_ptr<AgoraRteLocalStream>& stream);

  virtual void AfterUnPublish(
      int result, const std::shared_ptr<AgoraRteLocalStream>& stream);

  virtual void OnTrackPublished();

  virtual void OnTrackUnpublished();

  const std::string& GetTrackId() { return track_id_; }

  virtual int BeforeVideoEncodingChanged(
      const VideoEncoderConfiguration& config) = 0;

  static int64_t GenerateTrackTicket() {
    std::atomic<int64_t> track_ticket_ = {0};
    return ++track_ticket_;
  }

 protected:
  int CheckAndChangePublishState();

  int CheckAndChangeUnpublishState();

  std::string track_id_;
  std::mutex track_pub_state_lock_;
  PublishState track_pub_stat_ = PublishState::kUnpublished;
  PublishState track_state_before_unpub = PublishState::kUnpublished;
};

class AgoraRteRtcTrackBase : public AgoraRteTrackBase {
 public:
  AgoraRteRtcTrackBase() = default;

  virtual ~AgoraRteRtcTrackBase() = default;

  virtual std::shared_ptr<AgoraRteTrackImplBase> GetTackImpl() = 0;

  virtual int BeforeVideoEncodingChanged(
      const VideoEncoderConfiguration& config) override {
    return ERR_OK;
  }  // Do nothing by default
};

class AgoraRteRtcVideoTrackBase : public AgoraRteRtcTrackBase {
 public:
  AgoraRteRtcVideoTrackBase() = default;

  virtual ~AgoraRteRtcVideoTrackBase() = default;

  virtual std::shared_ptr<agora::rtc::ILocalVideoTrack> GetRtcVideoTrack()
      const = 0;

  std::shared_ptr<AgoraRteTrackImplBase> GetTackImpl() override;

 protected:
  std::shared_ptr<AgoraRteVideoTrackImpl> track_impl_;
};

class AgoraRteRtcAudioTrackBase : public AgoraRteRtcTrackBase {
 public:
  AgoraRteRtcAudioTrackBase() = default;

  virtual ~AgoraRteRtcAudioTrackBase() = default;

  virtual std::shared_ptr<agora::rtc::ILocalAudioTrack> GetRtcAudioTrack()
      const = 0;

  std::shared_ptr<AgoraRteTrackImplBase> GetTackImpl() override;

 protected:
  std::shared_ptr<AgoraRteAudioTrackImpl> track_impl_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteRtcStreamObserver.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "IAgoraRteMediaTrack.h"
#include "IAgoraRtmpStreamingService.h"

namespace agora {
namespace rte {

class AgoraRteScene;

class AgoraRteRtcMajorStream;

class AgoraRteRtcLocalStream;

class AgoraRteRtcStreamBase;

struct RteRtcStreamInfo {
  std::string user_id;    // the user id we expose to scene
  std::string stream_id;  // the stream id we expose to scene
  bool is_major_stream;   // whether rtc stream is used as major stream
};

class AgoraRteRtcMajorStreamObserver : public rtc::IRtcConnectionObserver {
 public:
  AgoraRteRtcMajorStreamObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcMajorStream>& stream) : scene_(scene),
                                                             stream_(stream) {}

  ~AgoraRteRtcMajorStreamObserver() override;

  void onConnected(const rtc::TConnectionInfo& connectionInfo,
                   rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onDisconnected(const rtc::TConnectionInfo& connectionInfo,
                      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onConnecting(const rtc::TConnectionInfo& connectionInfo,
                    rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onReconnecting(const rtc::TConnectionInfo& connectionInfo,
                      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onReconnected(const rtc::TConnectionInfo& connectionInfo,
                     rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onConnectionLost(const rtc::TConnectionInfo& connectionInfo) override;

  void onTokenPrivilegeWillExpire(const char* token) override;

  void onTokenPrivilegeDidExpire() override;

  void onConnectionFailure(const rtc::TConnectionInfo& connectionInfo,
                           rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onUserJoined(user_id_t userId) override;

  void onUserLeft(user_id_t userId,
                  rtc::USER_OFFLINE_REASON_TYPE reason) override;

  void onChannelMediaRelayStateChanged(int state, int code) override {}

  // Inherited via IRtcConnectionObserver and not implemented
  //
  void onTransportStats(const rtc::RtcStats& stats) override;

  void
  onUserAccountUpdated(rtc::uid_t uid, const char* user_account) override {}

  void onLastmileQuality(const rtc::QUALITY_TYPE quality) override {}

  void onLastmileProbeResult(const rtc::LastmileProbeResult& result) override {}

  void TryToFireClosedEvent() { fire_connection_closed_event_ = true; }

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcMajorStream> stream_;
  bool fire_connection_closed_event_ = false;
  bool is_close_event_failed_ = true;
};

class AgoraRteRtcLocalStreamObserver : public rtc::IRtcConnectionObserver {
 public:
  AgoraRteRtcLocalStreamObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcLocalStream>& stream) : scene_(scene),
                                                             stream_(stream) {}

  void onTokenPrivilegeWillExpire(const char* token) override;

  void onTokenPrivilegeDidExpire() override;

  // Inherited via IRtcConnectionObserver and not implemented
  //
  void onConnected(const rtc::TConnectionInfo& connectionInfo,
                   rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onDisconnected(const rtc::TConnectionInfo& connectionInfo,
                      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}

  void onConnecting(const rtc::TConnectionInfo& connectionInfo,
                    rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onReconnecting(const rtc::TConnectionInfo& connectionInfo,
                      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onReconnected(const rtc::TConnectionInfo& connectionInfo,
                     rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;

  void onConnectionLost(const rtc::TConnectionInfo& connectionInfo) override {}

  void onLastmileQuality(const rtc::QUALITY_TYPE quality) override {}

  void
  onLastmileProbeResult(const rtc::LastmileProbeResult& result) override {}

  void onConnectionFailure(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}

  void onUserJoined(user_id_t userId) override {}

  void onUserLeft(user_id_t userId,
                  rtc::USER_OFFLINE_REASON_TYPE reason) override {}

  void onTransportStats(const rtc::RtcStats& stats) override;

  void onChannelMediaRelayStateChanged(int state, int code) override {}

  void onUserAccountUpdated(rtc::uid_t uid,
                            const char* user_account) override {}

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcLocalStream> stream_;
};

class AgoraRteRtcLocalStreamUserObserver : public rtc::ILocalUserObserver {
 public:
  AgoraRteRtcLocalStreamUserObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcLocalStream>& stream) : scene_(scene),
                                                             stream_(stream) {}

  void onAudioTrackPublishSuccess(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack) override;

  void onAudioTrackPublicationFailure(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack,
    ERROR_CODE_TYPE error) override {}

  void onLocalAudioTrackStateChanged(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack,
    rtc::LOCAL_AUDIO_STREAM_STATE state,
    rtc::LOCAL_AUDIO_STREAM_ERROR errorCode) override {}

  void onLocalAudioTrackStatistics(const rtc::LocalAudioStats& stats) override;

  void onVideoTrackPublishSuccess(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack) override;

  void onVideoTrackPublicationFailure(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    ERROR_CODE_TYPE error) override {}

  void onLocalVideoTrackStateChanged(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    rtc::LOCAL_VIDEO_STREAM_STATE state,
    rtc::LOCAL_VIDEO_STREAM_ERROR errorCode) override {}

  void onLocalVideoTrackStatistics(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    const rtc::LocalVideoTrackStats& stats) override;

  void onAudioPublishStateChanged(const char* channel,
                                  rtc::STREAM_PUBLISH_STATE oldState,
                                  rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override;

  void onVideoPublishStateChanged(const char* channel,
                                  rtc::STREAM_PUBLISH_STATE oldState,
                                  rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override;

  void onUserInfoUpdated(user_id_t userId, USER_MEDIA_INFO msg,
                         bool val) override {}

  void onIntraRequestReceived() override {}

  void onStreamMessage(user_id_t userId, int streamId, const char* data,
                       size_t length) override {}

  void onPublishStateChangedCommon(const char* channel,
                                   rtc::STREAM_PUBLISH_STATE oldState,
                                   rtc::STREAM_PUBLISH_STATE newState,
                                   int elapseSinceLastState, MediaType type);

  // Inherited via ILocalUserObserver and not implemented
  //
  void onRemoteAudioTrackStatistics(
    agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
    const rtc::RemoteAudioTrackStats& stats) override {}

  void onUserAudioTrackSubscribed(
    user_id_t userId,
    agora_refptr<rtc::IRemoteAudioTrack> audioTrack) override {}

  void onUserAudioTrackStateChanged(
    user_id_t userId, agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
    rtc::REMOTE_AUDIO_STATE state, rtc::REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) override {}

  void onUserVideoTrackSubscribed(
    user_id_t userId, rtc::VideoTrackInfo trackInfo,
    agora_refptr<rtc::IRemoteVideoTrack> videoTrack) override {}

  void onUserVideoTrackStateChanged(
    user_id_t userId, agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
    rtc::REMOTE_VIDEO_STATE state, rtc::REMOTE_VIDEO_STATE_REASON reason,
    int elapsed) override {}

  void onRemoteVideoTrackStatistics(
    agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
    const rtc::RemoteVideoTrackStats& stats) override {}

  void onAudioVolumeIndication(const rtc::AudioVolumeInfo* speakers,
                               unsigned int speakerNumber,
                               int totalVolume) override {}

  void onAudioSubscribeStateChanged(
    const char* channel, user_id_t userId,
    rtc::STREAM_SUBSCRIBE_STATE oldState,
    rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) override {
  }

  void onVideoSubscribeStateChanged(
    const char* channel, user_id_t userId,
    rtc::STREAM_SUBSCRIBE_STATE oldState,
    rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) override {
  }

 private:
  class LocalStreamStatsManager {
   public:
    void SetRteLocalAudioStats(const RteLocalAudioStats& stats);
    void SetRteLocalVideoStats(const RteLocalVideoStats& stats);
    LocalStreamStats GetLocalStreamStats();
   private:
    std::mutex local_stream_stats_mutex_;
    LocalStreamStats local_stream_stats_ = {};
  };

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcLocalStream> stream_;
  LocalStreamStatsManager local_stream_stats_manager_;
};

class AgoraRteRtcLocalStreamCdnObserver : public rtc::IRtmpStreamingObserver {
 public:
  AgoraRteRtcLocalStreamCdnObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcLocalStream>& stream) : scene_(scene),
                                                             stream_(stream) {}

  void onRtmpStreamingStateChanged(
    const char* url, agora::rtc::RTMP_STREAM_PUBLISH_STATE state,
    agora::rtc::RTMP_STREAM_PUBLISH_ERROR err_code) override;
  void onStreamPublished(const char* url, int error) override;
  void onStreamUnpublished(const char* url) override;
  void onTranscodingUpdated() override;

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcLocalStream> stream_;
};

class AgoraRteRtcMajorStreamUserObserver : public rtc::ILocalUserObserver {
 public:
  AgoraRteRtcMajorStreamUserObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcMajorStream>& stream) : scene_(scene),
                                                             stream_(stream) {}

  void onRemoteAudioTrackStatistics(
    agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
    const rtc::RemoteAudioTrackStats& stats) override;

  void onUserAudioTrackSubscribed(
    user_id_t userId,
    agora_refptr<rtc::IRemoteAudioTrack> audioTrack) override;

  void onUserAudioTrackStateChanged(
    user_id_t userId, agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
    rtc::REMOTE_AUDIO_STATE state, rtc::REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) override {}

  void onUserVideoTrackSubscribed(
    user_id_t userId, rtc::VideoTrackInfo trackInfo,
    agora_refptr<rtc::IRemoteVideoTrack> videoTrack) override;

  void onUserVideoTrackStateChanged(
    user_id_t userId, agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
    rtc::REMOTE_VIDEO_STATE state, rtc::REMOTE_VIDEO_STATE_REASON reason,
    int elapsed) override {}

  void onRemoteVideoTrackStatistics(
    agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
    const rtc::RemoteVideoTrackStats& stats) override;

  void onAudioSubscribeStateChanged(const char* channel, user_id_t userId,
                                    rtc::STREAM_SUBSCRIBE_STATE oldState,
                                    rtc::STREAM_SUBSCRIBE_STATE newState,
                                    int elapseSinceLastState) override;

  void onAudioVolumeIndication(const rtc::AudioVolumeInfo* speakers,
                               unsigned int speakerNumber,
                               int totalVolume) override;

  void onVideoSubscribeStateChanged(const char* channel, user_id_t userId,
                                    rtc::STREAM_SUBSCRIBE_STATE oldState,
                                    rtc::STREAM_SUBSCRIBE_STATE newState,
                                    int elapseSinceLastState) override;

  void onUserInfoUpdated(user_id_t userId, USER_MEDIA_INFO msg,
                         bool val) override {}

  void onStreamMessage(user_id_t userId, int streamId, const char* data,
                       size_t length) override {}

  void onSubscribeStateChangedCommon(const char* channel, user_id_t userId,
                                     rtc::STREAM_SUBSCRIBE_STATE oldState,
                                     rtc::STREAM_SUBSCRIBE_STATE newState,
                                     int elapseSinceLastState, MediaType type);

  // Inherited via ILocalUserObserver and not implemented
  //
  void onAudioTrackPublishSuccess(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack) override {}

  void onAudioTrackPublicationFailure(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack,
    ERROR_CODE_TYPE error) override {}

  void onLocalAudioTrackStateChanged(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack,
    rtc::LOCAL_AUDIO_STREAM_STATE state,
    rtc::LOCAL_AUDIO_STREAM_ERROR errorCode) override {}

  void onLocalAudioTrackStatistics(
    const rtc::LocalAudioStats& stats) override {}

  void onVideoTrackPublishSuccess(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack) override {}

  void onVideoTrackPublicationFailure(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    ERROR_CODE_TYPE error) override {}

  void onLocalVideoTrackStateChanged(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    rtc::LOCAL_VIDEO_STREAM_STATE state,
    rtc::LOCAL_VIDEO_STREAM_ERROR errorCode) override {}

  void onLocalVideoTrackStatistics(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    const rtc::LocalVideoTrackStats& stats) override {}

  void onAudioPublishStateChanged(const char* channel,
                                  rtc::STREAM_PUBLISH_STATE oldState,
                                  rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override {}

  void onVideoPublishStateChanged(const char* channel,
                                  rtc::STREAM_PUBLISH_STATE oldState,
                                  rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override {}

 private:
  class RemoteStreamStatsManager {
   public:
    void SetRteRemoteAudioStats(const std::string& stream_id,
                                const RteRemoteAudioStats& stats);
    void SetRteRemoteVideoStats(const std::string& stream_id,
                                const RteRemoteVideoStats& stats);
    bool GetRemoteStreamStats(const std::string& stream_id,
                              RemoteStreamStats& stats);

   private:
    std::mutex remote_stream_stats_list_mutex_;
    std::map<std::string, RemoteStreamStats> remote_stream_stats_list_;
  };

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcMajorStream> stream_;
  RemoteStreamStatsManager remote_stream_stats_manager_;
};

class AgoraRteRtcAudioFrameObserver : public agora::media::IAudioFrameObserver {
 public:
  AgoraRteRtcAudioFrameObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcStreamBase>& stream) : scene_(scene),
                                                            stream_(stream) {}

  bool onRecordAudioFrame(AudioFrame& audioFrame) override;

  bool onPlaybackAudioFrame(AudioFrame& audioFrame) override;

  bool onMixedAudioFrame(AudioFrame& audioFrame) override;

  bool onPlaybackAudioFrameBeforeMixing(user_id_t userId,
                                        AudioFrame& audioFrame) override;

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcStreamBase> stream_;
};

class AgoraRteRtcRemoteVideoObserver : public rtc::IVideoFrameObserver2 {
 public:
  explicit AgoraRteRtcRemoteVideoObserver(
    const std::shared_ptr<AgoraRteScene>& scene) : scene_(scene) {}

  void onFrame(user_id_t uid, rtc::conn_id_t connectionId,
               const media::base::VideoFrame* frame) override;

 private:
  std::weak_ptr<AgoraRteScene> scene_;
};

class RteStatsConvertHelper {
 public:
  static void LocalAudioStats(const rtc::LocalAudioStats& stats,
                              RteLocalAudioStats& dest_stats);
  static void LocalVideoStats(const rtc::LocalVideoTrackStats& stats,
                              RteLocalVideoStats& dest_stats);
  static void RemoteAudioStats(const rtc::RemoteAudioTrackStats& stats,
                               RteRemoteAudioStats& dest_stats);
  static void RemoteVideoStats(const rtc::RemoteVideoTrackStats& stats,
                               RteRemoteVideoStats dest_stats);
};

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteScene.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "IAgoraRteScene.h"

namespace agora {
namespace rte {

enum class MediaType;
class AgoraRteMajorStream;
class AgoraRteLocalStream;
class AgoraRteRemoteStream;
class AgoraRteStreamFactory;
class AgoraRteTrackBase;

class AgoraRteScene : public std::enable_shared_from_this<AgoraRteScene>,
                      public IAgoraRteScene {
 public:
  AgoraRteScene(std::shared_ptr<AgoraRteStreamFactory> stream_factory,
                const std::string& scene_id, const SceneConfig& config);

  virtual ~AgoraRteScene() { Leave(); }

  // Note: All publish functions should be protected by lock which makes them
  // thread-safe
  //
  int Join(const std::string& user_id, const std::string& token,
           const JoinOptions& option) override;

  void Leave() override;

  SceneInfo GetSceneInfo() const override;

  UserInfo GetLocalUserInfo() const override;

  std::vector<UserInfo> GetRemoteUsers() const override;

  std::vector<StreamInfo> GetLocalStreams() const override;

  std::vector<StreamInfo> GetRemoteStreams() const override;

  std::vector<StreamInfo> GetRemoteStreamsByUserId(
      const std::string& user_id) const override;

  int CreateOrUpdateRTCStream(const std::string& local_stream_id,
                              const RtcStreamOptions& option) override;

  int CreateOrUpdateDirectCDNStream(
      const std::string& local_stream_id,
      const DirectCdnStreamOptions& option) override;

  int DestroyStream(const std::string& local_stream_id) override;

  int SetAudioEncoderConfiguration(
      const std::string& local_stream_id,
      const AudioEncoderConfiguration& config) override;

  int SetVideoEncoderConfiguration(
      const std::string& local_stream_id,
      const VideoEncoderConfiguration& config) override;

  int SetBypassCdnTranscoding(
      const std::string& local_stream_id,
      const agora::rtc::LiveTranscoding& transcoding) override;

  int AddBypassCdnUrl(const std::string& local_stream_id,
                      const std::string& target_cdn_url,
                      bool transcoding_enabled) override;

  int RemoveBypassCdnUrl(const std::string& local_stream_id,
                         const std::string& target_cdn_url) override;

  int PublishLocalAudioTrack(
      const std::string& local_stream_id,
      std::shared_ptr<IAgoraRteAudioTrack> track) override;

  int PublishLocalVideoTrack(
      const std::string& local_stream_id,
      std::shared_ptr<IAgoraRteVideoTrack> track) override;

  int UnpublishLocalAudioTrack(
      std::shared_ptr<IAgoraRteAudioTrack> track) override;

  int UnpublishLocalVideoTrack(
      std::shared_ptr<IAgoraRteVideoTrack> track) override;

  int PublishMediaPlayer(const std::string& local_stream_id,
                         std::shared_ptr<IAgoraRteMediaPlayer> player) override;

  int UnpublishMediaPlayer(
      std::shared_ptr<IAgoraRteMediaPlayer> player) override;

  int SubscribeRemoteAudio(const std::string& remote_stream_id) override;

  int UnsubscribeRemoteAudio(const std::string& remote_stream_id) override;

  int SubscribeRemoteVideo(const std::string& remote_stream_id,
                           const VideoSubscribeOptions& options) override;

  int UnsubscribeRemoteVideo(const std::string& remote_stream_id) override;

  int SetRemoteVideoCanvas(const std::string& remote_stream_id,
                           const VideoCanvas& canvas) override;

  void RegisterEventHandler(
      std::shared_ptr<IAgoraRteSceneEventHandler> event_handler) override;

  void UnregisterEventHandler(
      std::shared_ptr<IAgoraRteSceneEventHandler> event_handler) override;

  void RegisterRemoteVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  void UnregisterRemoteVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  void RegisterAudioFrameObserver(
      std::shared_ptr<IAgoraRteAudioFrameObserver> observer,
      const AudioObserverOptions& option) override;

  void UnregisterAudioFrameObserver(
      std::shared_ptr<IAgoraRteAudioFrameObserver> observer) override;

  int AdjustUserPlaybackSignalVolume(const std::string& remote_stream_id, int volume) override;

  int GetUserPlaybackSignalVolume(const std::string& remote_stream_id, int* volume) override;

  void ChangeSceneState(SceneState state, ConnectionChangedReason reason);

  void AddRemoteUser(const std::string& user_id);

  void RemoveRemoteUser(const std::string& user_id);

  void AddRemoteStream(const std::string& remote_stream_id,
                       const std::shared_ptr<AgoraRteRemoteStream>& stream);

  void RemoveRemoteStream(const std::string& stream_id);

  void OnLocalStreamStateChanged(const StreamInfo& stream, MediaType media_type,
                                 StreamMediaState old_state,
                                 StreamMediaState new_state,
                                 StreamStateChangedReason reason);

  void OnRemoteStreamStateChanged(const StreamInfo& stream,
                                  MediaType media_type,
                                  StreamMediaState old_state,
                                  StreamMediaState new_state,
                                  StreamStateChangedReason reason);

  void OnAudioVolumeIndication(const std::vector<AudioVolumeInfo>& speakers,
                               int totalVolume);

  void OnRemoteVideoFrame(const std::string stream_id,
                          const media::base::VideoFrame& frame);

  void OnRecordAudioFrame(AudioFrame& audioFrame);

  void OnPlaybackAudioFrame(AudioFrame& audioFrame);

  void OnMixedAudioFrame(AudioFrame& audioFrame);

  void OnPlaybackAudioFrameBeforeMixing(const std::string& stream_id,
                                        AudioFrame& audioFrame);

  void OnSceneTokenWillExpired(const std::string& token);

  void OnSceneTokenExpired();

  void OnStreamTokenWillExpire(const std::string& stream_id,
                               const std::string& token);

  void OnStreamTokenExpired(const std::string& stream_id);

  void OnCdnStateChanged(const std::string& stream_id, const char* url,
                         rtc::RTMP_STREAM_PUBLISH_STATE state,
                         rtc::RTMP_STREAM_PUBLISH_ERROR errCode);

  void OnCdnPublished(const std::string& stream_id, const char* url, int error);

  void OnCdnUnpublished(const std::string& stream_id, const char* url);

  void OnCdnTranscodingUpdated(const std::string& stream_id);

  void OnRtcStats(const std::string& stream_id, const rtc::RtcStats& stats);

  void OnLocalStreamStats(const std::string& stream_id, const LocalStreamStats& stats);

  void OnRemoteStreamStats(const std::string& stream_id, const RemoteStreamStats& stats);

  std::shared_ptr<AgoraRteMajorStream> GetMajorStream();

  bool IsAudioRecordingOrPlayOutEnabled() {
    return config_.enable_audio_recording_or_playout;
  }

 private:
  int CreateOrUpdateStreamCommon(const std::string& local_stream_id,
                                 const StreamOption& option);

  std::shared_ptr<AgoraRteLocalStream> CreatLocalStream(
      const std::string& local_stream_id, const StreamOption& option);

  std::shared_ptr<AgoraRteLocalStream> GetLocalStream(
      const std::string& local_stream_id);

  std::shared_ptr<AgoraRteLocalStream> RemoveLocalStream(
      const std::string& local_stream_id);

  void SetMajorStream(const std::shared_ptr<AgoraRteMajorStream>& stream);

  std::shared_ptr<AgoraRteRemoteStream> GetRemoteStream(
      const std::string& stream_id);

  void DestroyStream(const std::shared_ptr<AgoraRteLocalStream>& local_stream);

  bool IsSceneActive();

  template <typename track_type>
  int PublishCommon(std::shared_ptr<track_type> track,
                    const std::string& local_stream_id);

  template <typename track_type>
  int PublishSpecific(const std::shared_ptr<AgoraRteLocalStream>& stream,
                      const std::shared_ptr<track_type>& track);

  template <typename track_type>
  int UnpublishCommon(std::shared_ptr<track_type> track);

  template <typename track_type>
  int UnpublishSpecific(const std::shared_ptr<AgoraRteLocalStream>& stream,
                        const std::shared_ptr<track_type>& track);

 private:
  mutable std::recursive_mutex operation_lock_;  // Sync between user operations
  mutable std::recursive_mutex callback_lock_;   // Sync between callbacks

  std::string scene_id_;
  std::string user_id_;
  SceneState scene_state_ = SceneState::kDisconnected;
  SceneConfig config_;

  std::vector<std::weak_ptr<IAgoraRteSceneEventHandler>> event_handlers_;
  std::vector<std::weak_ptr<IAgoraRteVideoFrameObserver>>
      remote_video_frame_observers_;
  std::vector<std::weak_ptr<IAgoraRteAudioFrameObserver>>
      audio_frame_observers_;

  AudioObserverOptions audio_observer_option_;

  std::shared_ptr<AgoraRteMajorStream> major_stream_object_;
  std::map<std::string, std::shared_ptr<AgoraRteLocalStream>>
      local_stream_objects_;
  std::map<std::string, std::shared_ptr<AgoraRteRemoteStream>>
      remote_stream_objects_;
  std::unordered_set<std::string> remote_users_;

  std::shared_ptr<AgoraRteStreamFactory> stream_factory_;
  std::map<std::string, std::vector<rtc::RtcStats>> rtc_stats_map_;
};

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteScreenVideoTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once


namespace agora {

namespace rte {

class AgoraRteScreenVideoTrack : public IAgoraRteScreenVideoTrack,
                                 public AgoraRteRtcVideoTrackBase {
 public:
  AgoraRteScreenVideoTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteScreenVideoTrack() = default;

  // Inherited via IAgoraRteScreenVideoTrack
  virtual int SetPreviewCanvas(const VideoCanvas& canvas) override;

  virtual SourceType GetSourceType() override;

  virtual void RegisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual void UnregisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual int EnableVideoFilter(const std::string& id, bool enable) override;

  virtual int SetFilterProperty(const std::string& id, const std::string& key,
                                const std::string& json_value) override;

  virtual std::string GetFilterProperty(const std::string& id,
                                        const std::string& key) override;

  virtual const std::string& GetAttachedStreamId() override;

#if RTE_WIN
  virtual int StartCaptureScreen(const Rectangle& screenRect,
                                 const Rectangle& regionRect) override;

  virtual int StartCaptureWindow(view_t windowId,
                                 const Rectangle& regionRect) override;
#elif RTE_MAC
  virtual int StartCaptureScreen(view_t displayId,
                                 const Rectangle& regionRect) override;

  virtual int StartCaptureWindow(view_t windowId,
                                 const Rectangle& regionRect) override;
#elif RTE_ANDROID
  virtual int StartCaptureScreen(void* data,
                                 const VideoDimensions& dimensions) override;
#elif RTE_IPHONE
  virtual int StartCaptureScreen() override;
#endif

  virtual void StopCapture() override;

#if RTE_WIN || RTE_MAC
  virtual int SetContentHint(VIDEO_CONTENT_HINT contentHint) override;

  virtual int UpdateScreenCaptureRegion(const Rectangle& regionRect) override;
#endif

  // Inherited via AgoraRteRtcVideoTrackBase
  virtual void SetStreamId(const std::string& stream_id) override;
  virtual std::shared_ptr<agora::rtc::ILocalVideoTrack> GetRtcVideoTrack()
      const override;

 private:
  agora_refptr<rtc::IScreenCapturer> capture_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCdnStream.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include <map>

#include "IAgoraRteMediaTrack.h"
#include "IAgoraRteScene.h"
#include "NGIAgoraRtmpConnection.h"
#include "NGIAgoraRtmpLocalUser.h"

namespace agora {
namespace rte {

class AgoraRteScene;

class AgoraRteCdnLocalStream : public AgoraRteLocalStream,
                               public rtc::IRtmpConnectionObserver,
                               public rtc::IRtmpLocalUserObserver {
 public:
  AgoraRteCdnLocalStream(
      std::shared_ptr<AgoraRteScene> scene,
      std::shared_ptr<agora::base::IAgoraService> rtc_service,
      const std::string& stream_id, const std::string& url);

  // Inherited via AgoraRteLocalStream
  virtual int Connect() override;

  virtual void Disconnect() override;

  virtual size_t GetAudioTrackCount() override;

  virtual size_t GetVideoTrackCount() override;

  virtual int UpdateOption(const StreamOption& option) override;

  virtual int PublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track) override;

  virtual int PublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track) override;

  virtual int UnpublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track) override;

  virtual int UnpublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track) override;

  virtual int SetAudioEncoderConfiguration(
      const AudioEncoderConfiguration& config) override;

  virtual int SetVideoEncoderConfiguration(
      const VideoEncoderConfiguration& config) override;

  virtual int EnableLocalAudioObserver(
      const LocalAudioObserverOptions& option) override;

  virtual int DisableLocalAudioObserver() override;

  virtual int SetBypassCdnTranscoding(
      const agora::rtc::LiveTranscoding& transcoding) override;

  virtual int AddBypassCdnUrl(const std::string& target_cdn_url,
                              bool transcoding_enabled) override;

  virtual int RemoveBypassCdnUrl(const std::string& target_cdn_url) override;

 private:
  // IRtmpConnectionObserver
  void onConnected(const rtc::RtmpConnectionInfo& connectionInfo) override;
  void onDisconnected(const rtc::RtmpConnectionInfo& connectionInfo) override;
  void onReconnecting(const rtc::RtmpConnectionInfo& connectionInfo) override;
  void onConnectionFailure(const rtc::RtmpConnectionInfo& connectionInfo,
                           rtc::RTMP_CONNECTION_ERROR errCode) override;
  void onTransferStatistics(uint64_t video_bitrate, uint64_t audio_bitrate,
                            uint64_t video_frame_rate) override;

  // IRtmpLocalUserObserver
  void onAudioTrackPublishSuccess(
      agora_refptr<rtc::ILocalAudioTrack> audioTrack) override;
  void onAudioTrackPublicationFailure(
      agora_refptr<rtc::ILocalAudioTrack> audioTrack,
      rtc::PublishAudioError error) override;
  void onVideoTrackPublishSuccess(
      agora_refptr<rtc::ILocalVideoTrack> videoTrack) override;
  void onVideoTrackPublicationFailure(
      agora_refptr<rtc::ILocalVideoTrack> videoTrack,
      rtc::PublishVideoError error) override;

  int createRtmpConnection();
  int stopDirectCdnStreamingInternal();
  void FireStreamStateChange(MediaType media_type, StreamMediaState old_state,
                             StreamMediaState new_state,
                             StreamStateChangedReason reason);

 private:
  enum streaming_state_internal {
    STATE_IDLE = 0,
    STATE_PUBLISHING,
    STATE_PUBLISHED,
    STATE_PUBLISH_STOPPED,
    STATE_PUBLISH_FAILED
  };
  streaming_state_internal streaming_state_ = STATE_IDLE;

  rtc::RtmpStreamingAudioConfiguration audio_encoder_config_;
  rtc::RtmpStreamingVideoConfiguration video_encoder_config_;

  VideoEncoderConfiguration track_video_encoder_config_;
  bool is_video_encoder_set_ = false;

  agora_refptr<rtc::ILocalVideoTrack> published_video_track_;
  StreamMediaState published_video_stream_state_ = StreamMediaState::kIdle;
  std::map<agora_refptr<rtc::ILocalAudioTrack>, StreamMediaState>
      published_audio_tracks_;

  std::weak_ptr<AgoraRteScene> scene_;
  std::string url_;
  std::string stream_id_;
  std::shared_ptr<agora::base::IAgoraService> rtc_service_;
  agora_refptr<rtc::IRtmpConnection> rtmp_conn_;
  rtc::IRtmpLocalUser* local_user_ = nullptr;
};

}  // namespace rte
}  // namespace agora
// ======================= AgoraRteCameraVideoTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once


namespace agora {
namespace rte {

class AgoraRteScene;
class AgoraRteCameraVideoTrack;

class AgoraRteCameraObserver final : public rtc::ICameraCaptureObserver {
 public:
  AgoraRteCameraObserver(
      const std::shared_ptr<AgoraRteCameraVideoTrack>& camera_track);

  void onCameraStateChanged(ICameraCapturer::CAMERA_STATE state,
                            ICameraCapturer::CAMERA_SOURCE source) override;

 protected:
  std::weak_ptr<AgoraRteCameraVideoTrack> camera_track_;
};

class AgoraRteCameraVideoTrack final : public IAgoraRteCameraVideoTrack,
                                       public AgoraRteRtcVideoTrackBase {
 public:
  AgoraRteCameraVideoTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteCameraVideoTrack();

#if RTE_IPHONE || RTE_ANDROID
  virtual int SetCameraSource(CameraSource source) override;

  virtual void SwitchCamera() override;

  virtual int SetCameraZoom(float zoomValue) override;

  virtual int SetCameraFocus(float x, float y) override;

  virtual int SetCameraAutoFaceFocus(bool enable) override;

  virtual int SetCameraFaceDetection(bool enable) override;
#elif RTE_DESKTOP
  virtual int SetCameraDevice(const std::string& device_id) override;

  virtual void SetDeviceOrientation(VIDEO_ORIENTATION orientation) override;

#endif

  virtual int SetPreviewCanvas(const VideoCanvas& canvas) override;

  virtual SourceType GetSourceType() override;

  virtual const std::string& GetAttachedStreamId() override;

  virtual void RegisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual void UnregisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual int EnableVideoFilter(const std::string& id, bool enable) override;

  virtual int SetFilterProperty(const std::string& id, const std::string& key,
                                const std::string& json_value) override;

  virtual std::string GetFilterProperty(const std::string& id,
                                        const std::string& key) override;

  virtual int StartCapture(CameraCallbackFun callback = nullptr) override;

  virtual void StopCapture() override;

  virtual void Init() override;

  virtual void SetStreamId(const std::string& stream_id) override;

  virtual std::shared_ptr<agora::rtc::ILocalVideoTrack> GetRtcVideoTrack()
      const override;

  CameraCallbackFun& GetCallback() { return callback_; }

 private:
  rtc::ICameraCapturer::CAMERA_SOURCE GetRtcCameraSource(CameraSource src);

 protected:
  agora_refptr<rtc::ICameraCapturer> camera_capturer_;
  CameraSource source_ = CameraSource::kFront;
  std::mutex camera_track_lock_;
  bool is_camera_source_assigned_ = false;
  std::shared_ptr<AgoraRteCameraObserver> camera_ob_;
  CameraCallbackFun callback_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMixedVideoTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once


namespace agora {
namespace rte {
class AgoraRteMixedVideoTrack final : public IAgoraRteMixedVideoTrack,
                                      public AgoraRteRtcVideoTrackBase {
 public:
  AgoraRteMixedVideoTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteMixedVideoTrack();

  virtual int SetPreviewCanvas(const VideoCanvas& canvas) override;

  virtual SourceType GetSourceType() override;

  virtual void RegisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual void UnregisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual int EnableVideoFilter(const std::string& id, bool enable) override;

  virtual int SetFilterProperty(const std::string& id, const std::string& key,
                                const std::string& json_value) override;

  virtual std::string GetFilterProperty(const std::string& id,
                                        const std::string& key) override;

  virtual const std::string& GetAttachedStreamId() override;

  virtual int SetLayout(const LayoutConfigs layoutConfigs) override;

  virtual int GetLayout(LayoutConfigs& layoutConfigs) override;

  virtual int AddTrack(std::shared_ptr<IAgoraRteVideoTrack> track) override;

  virtual int RemoveTrack(std::shared_ptr<IAgoraRteVideoTrack> track) override;

  virtual int AddMediaPlayer(
      std::shared_ptr<IAgoraRteMediaPlayer> mediaPlayer) override;

  virtual int RemoveMediaPlayer(
      std::shared_ptr<IAgoraRteMediaPlayer> mediaPlayer) override;

  virtual void SetStreamId(const std::string& stream_id) override;

  virtual std::shared_ptr<agora::rtc::ILocalVideoTrack> GetRtcVideoTrack()
      const override;

  virtual int BeforeVideoEncodingChanged(
      const VideoEncoderConfiguration& config) override;

 private:
  int AddImage(const std::string& id, const rtc::MixerLayoutConfig& layout);

  int RemoveImage(const std::string& id);

 protected:
  agora_refptr<rtc::IVideoMixerSource> video_mixer_;
  Optional<LayoutConfigs> layout_configs_;
  int fps_from_encoder_ = 0;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteVideoTrackImpl.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "IAgoraRteMediaObserver.h"

namespace agora {
namespace rte {

class AgoraRteVideoTrackImpl;

class AgoraRteRawVideoFrameRender : public rtc::IVideoRenderer {
 public:
  AgoraRteRawVideoFrameRender(
      std::shared_ptr<AgoraRteVideoTrackImpl> video_track) {
    video_track_ = video_track;
  }

  virtual ~AgoraRteRawVideoFrameRender() = default;

  int onFrame(const media::base::VideoFrame& videoFrame) override;

 private:
  int setRenderMode(media::base::RENDER_MODE_TYPE renderMode) override {
    return ERR_OK;
  }

  int setMirror(bool mirror) override { return ERR_OK; }

  int setView(void* view) override { return ERR_OK; }

  int unsetView() override { return ERR_OK; }

  std::weak_ptr<AgoraRteVideoTrackImpl> video_track_;
};

class AgoraRteVideoTrackImpl
    : public std::enable_shared_from_this<AgoraRteVideoTrackImpl>,
      public AgoraRteTrackImplBase {
  friend class AgoraRteRawVideoFrameRender;

 public:
  std::shared_ptr<rtc::IVideoRenderer> GetVideoRender(
      bool create_if_not_exist = true);

  AgoraRteVideoTrackImpl(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteVideoTrackImpl();

  int SetPreviewCanvas(const VideoCanvas& canvas);

  void DisableAnyRender() { is_render_enabled_ = false; }

  void SetTrack(std::shared_ptr<rtc::ILocalVideoTrack> track);

  int SetView(View view);

  int SetVideoEncoderConfiguration(const VideoEncoderConfiguration& config);

  int RegisterLocalVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer);

  void UnregisterLocalVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer);

  std::shared_ptr<rtc::ILocalVideoTrack> GetVideoTrack() const;

  int Start() override;

  void Stop() override;

  void Reset();

 protected:
  std::mutex render_lock_;
  bool is_render_enabled_ = true;
  std::shared_ptr<rtc::ILocalVideoTrack> video_track_;
  std::shared_ptr<rtc::IVideoRenderer> video_render_;
  std::vector<std::weak_ptr<IAgoraRteVideoFrameObserver>>
      local_video_frame_observers_;
  agora_refptr<AgoraRteRawVideoFrameRender> raw_video_frame_renders_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCustomAudioTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once


namespace agora {
namespace rte {

class AgoraRteScene;

class AgoraRteCustomAudioTrack final : public IAgoraRteCustomAudioTrack,
                                       public AgoraRteRtcAudioTrackBase {
 public:
  AgoraRteCustomAudioTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteCustomAudioTrack() = default;

  virtual int EnableLocalPlayback() override;

  virtual SourceType GetSourceType() override;

  virtual int AdjustPublishVolume(int volume) override;

  virtual int AdjustPlayoutVolume(int volume) override;

  virtual const std::string& GetAttachedStreamId() override;

  virtual int PushAudioFrame(AudioFrame& frame) override;

  virtual void SetStreamId(const std::string& stream_id) override;

  virtual std::shared_ptr<agora::rtc::ILocalAudioTrack> GetRtcAudioTrack()
      const override;

 private:
  agora_refptr<rtc::IAudioPcmDataSender> audio_frame_sender_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteWrapperVideoTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//
#pragma once

#include "AgoraRefPtr.h"
#include "AgoraRteBase.h"
#include "IAgoraRteMediaTrack.h"

namespace agora {

namespace rtc {
class ILocalVideoTrack;
class ILocalAudioTrack;
class IMediaStreamingSource;
}  // namespace rtc

namespace rte {
class AgoraRteWrapperVideoTrack : public IAgoraRteVideoTrack,
                                  public AgoraRteRtcVideoTrackBase {
 public:
  AgoraRteWrapperVideoTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service,
      std::shared_ptr<rtc::ILocalVideoTrack> rtc_video_track);

  //////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of IAgoraRteVideoTrack ////////////
  ///////////////////////////////////////////////////////////////////////
  int SetPreviewCanvas(const VideoCanvas& canvas) override;
  SourceType GetSourceType() override;
  void RegisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;
  void UnregisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;
  int EnableVideoFilter(const std::string& id, bool enable) override;
  int SetFilterProperty(const std::string& id, const std::string& key,
                        const std::string& json_value) override;
  std::string GetFilterProperty(const std::string& id,
                                const std::string& key) override;
  const std::string& GetAttachedStreamId() override;

  ////////////////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of AgoraRteRtcVideoTrackBase
  ///////////////////
  ////////////////////////////////////////////////////////////////////////////////
  std::shared_ptr<agora::rtc::ILocalVideoTrack> GetRtcVideoTrack()
      const override;

  ////////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of AgoraRteTrackBase ////////////////
  ////////////////////////////////////////////////////////////////////////
  void SetStreamId(const std::string& stream_id) override;

 protected:
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMicrophoneAudioTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once


namespace agora {

namespace rte {

class AgoraRteScene;

class AgoraRteMicrophoneAudioTrack final : public IAgoraRteMicrophoneAudioTrack,
                                           public AgoraRteRtcAudioTrackBase {
 public:
  AgoraRteMicrophoneAudioTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteMicrophoneAudioTrack() = default;

  virtual int EnableLocalPlayback() override;

  virtual SourceType GetSourceType() override;

  virtual int AdjustPublishVolume(int volume) override;

  virtual int AdjustPlayoutVolume(int volume) override;

  virtual const std::string& GetAttachedStreamId() override;

  virtual int StartRecording() override;

  virtual void StopRecording() override;

  virtual int EnableEarMonitor(bool enable, int includeAudioFilter) override;

  virtual int SetAudioReverbPreset(AUDIO_REVERB_PRESET reverb_preset) override;

  virtual int SetVoiceChangerPreset(
      VOICE_CHANGER_PRESET voice_changer_preset) override;

  virtual void SetStreamId(const std::string& stream_id) override;

  virtual std::shared_ptr<agora::rtc::ILocalAudioTrack> GetRtcAudioTrack()
      const override;

 private:
  std::shared_ptr<rtc::ILocalAudioTrack> audio_frame_sender_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCustomVideoTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once


namespace agora {
namespace rte {

class AgoraRteCustomVideoTrack final : public IAgoraRteCustomVideoTrack,
                                       public AgoraRteRtcVideoTrackBase {
 public:
  AgoraRteCustomVideoTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  ~AgoraRteCustomVideoTrack() = default;

  virtual SourceType GetSourceType() override;

  virtual int SetPreviewCanvas(const VideoCanvas& canvas) override;

  virtual void RegisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual void UnregisterVideoFrameObserver(
      std::shared_ptr<IAgoraRteVideoFrameObserver> observer) override;

  virtual int EnableVideoFilter(const std::string& id, bool enable) override;

  virtual int SetFilterProperty(const std::string& id, const std::string& key,
                                const std::string& json_value) override;

  virtual std::string GetFilterProperty(const std::string& id,
                                        const std::string& key) override;

  virtual const std::string& GetAttachedStreamId() override;

  virtual void SetStreamId(const std::string& stream_id) override;

  virtual std::shared_ptr<agora::rtc::ILocalVideoTrack> GetRtcVideoTrack()
      const override;

  virtual int PushVideoFrame(const ExternalVideoFrame& frame) override;

 private:
  agora_refptr<rtc::IVideoFrameSender> video_frame_sender_;
};

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteRtcStream.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "IAgoraRteMediaTrack.h"
#include "IAgoraRteScene.h"
#include "IAgoraRtmpStreamingService.h"

namespace agora {
namespace rte {

class AgoraRteScene;

class AgoraRteRtcAudioTrackBase;
class AgoraRteRtcVideoTrackBase;

class AgoraRteRtcRemoteStream;
class AgoraRteRtcLocalStream;

class AgoraRteRtcStreamBase {
 public:
  AgoraRteRtcStreamBase(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<agora::base::IAgoraService>& rtc_service,
      const std::string& token, const JoinOptions& option);

  virtual ~AgoraRteRtcStreamBase() = default;

  std::shared_ptr<rtc::IRtcConnection> GetRtcConnection() {
    return internal_rtc_connection_;
  }

  std::shared_ptr<agora::base::IAgoraService> GetRtcService() {
    return rtc_service_;
  }

  const std::string& GetRtcStreamId() { return rtc_stream_id_; }

  void SetLocalUid(uint32_t uid) { local_uid_ = uid; }

  virtual int RegisterObservers() = 0;

  virtual void UnregisterObservers() = 0;

  int Connect();

  void Disconnect();

  int UpdateLocalAudioObserver(bool enable,
                               const LocalAudioObserverOptions& option = {});
  int UpdateRemoteAudioObserver(bool enable,
                                const RemoteAudioObserverOptions& option = {});

  int UpdateRemoteVideoObserver(bool enable);

  virtual std::shared_ptr<AgoraRteRtcStreamBase> GetSharedSelf() = 0;

 protected:
  std::recursive_mutex callback_lock_;
  std::weak_ptr<AgoraRteScene> scene_;
  std::shared_ptr<rtc::IRtcConnection> internal_rtc_connection_;

  std::string token_;
  std::string rtc_stream_id_;
  std::shared_ptr<agora::base::IAgoraService> rtc_service_;

  std::unique_ptr<AgoraRteRtcAudioFrameObserver> rtc_audio_observer_;
  std::unique_ptr<AgoraRteRtcRemoteVideoObserver> rtc_remote_video_observer_;

  AudioObserverOptions audio_option_;

  bool is_remote_video_observer_added_ = false;
  bool is_local_audio_observer_added_ = false;
  bool is_remote_audio_observer_added_ = false;
  uint32_t local_uid_ = 0;
};

class AgoraRteRtcMajorStream final : public AgoraRteRtcStreamBase,
                                     public AgoraRteMajorStream {
 public:
  AgoraRteRtcMajorStream(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<agora::base::IAgoraService>& rtc_service,
      const std::string& token, const JoinOptions& option);

  ~AgoraRteRtcMajorStream() { Disconnect(); }

  // Scene will never parallel call override functions below, so we don't need
  // to add lock inside if these functions are only called from scene
  //
  int Connect() override;

  void Disconnect() override;

  int RegisterObservers() override;

  void UnregisterObservers() override;

  void AddRemoteStream(const std::shared_ptr<AgoraRteRtcRemoteStream>& stream);

  void RemoveRemoteStream(const std::string& rtc_stream_id);

  std::shared_ptr<AgoraRteRtcRemoteStream> FindRemoteStream(
      const std::string& rtc_stream_id);

  std::shared_ptr<AgoraRteRtcRemoteStream> FindRemoteStream(
      const agora_refptr<rtc::IRemoteVideoTrack>& videoTrack);

  std::shared_ptr<AgoraRteRtcRemoteStream> FindRemoteStream(
      const agora_refptr<rtc::IRemoteAudioTrack>& audioTrack);

  int PushUserInfo(const UserInfo& info, InstanceState state) override {
    return ERR_OK;
  }

  int PushStreamInfo(const StreamInfo& info, InstanceState state) override {
    return ERR_OK;
  }

  int PushMediaInfo(const StreamInfo& info, MediaType media_type,
                    InstanceState state) override {
    return ERR_OK;
  }

  std::shared_ptr<AgoraRteRtcStreamBase> GetSharedSelf() override {
    return std::static_pointer_cast<AgoraRteRtcMajorStream>(shared_from_this());
  }

 private:
  std::vector<std::shared_ptr<AgoraRteRtcRemoteStream>> remote_streams_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcMajorStreamUserObserver>>
      rtc_major_stream_user_observer_;

  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcMajorStreamObserver>>
      rtc_major_stream_observer_;
};

class AgoraRteRtcLocalStream final : public AgoraRteRtcStreamBase,
                                     public AgoraRteLocalStream {
 public:
  AgoraRteRtcLocalStream(
      std::shared_ptr<AgoraRteScene> scene,
      std::shared_ptr<agora::base::IAgoraService> rtc_service,
      const std::string& stream_id, const std::string& token);

  ~AgoraRteRtcLocalStream() {
    DestroyRtmpService();
    Disconnect();
  }

  // Scene will never parallel call override functions below, so we don't need
  // to add lock inside if these functions are only called from scene
  //
  int Connect() override;

  void Disconnect() override;

  size_t GetAudioTrackCount() override;

  size_t GetVideoTrackCount() override;

  int UpdateOption(const StreamOption& option) override;

  int PublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track) override;

  int PublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track) override;

  int UnpublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track) override;

  int UnpublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track) override;

  int SetAudioEncoderConfiguration(
      const AudioEncoderConfiguration& config) override;

  int SetVideoEncoderConfiguration(
      const VideoEncoderConfiguration& config) override;

  int EnableLocalAudioObserver(
      const LocalAudioObserverOptions& option) override {
    return UpdateLocalAudioObserver(true, option);
  }

  int DisableLocalAudioObserver() override {
    return UpdateLocalAudioObserver(false);
  }

  int SetBypassCdnTranscoding(
      const agora::rtc::LiveTranscoding& transcoding) override;

  int AddBypassCdnUrl(const std::string& target_cdn_url,
                      bool transcoding_enabled) override;

  int RemoveBypassCdnUrl(const std::string& target_cdn_url) override;

  int RegisterObservers() override;

  void UnregisterObservers() override;

  void OnConnected();

  void OnAudioTrackPublished(
      const agora_refptr<rtc::ILocalAudioTrack>& audioTrack);

  void OnVideoTrackPublished(
      const agora_refptr<rtc::ILocalVideoTrack>& audioTrack);

  std::shared_ptr<AgoraRteRtcStreamBase> GetSharedSelf() override {
    return std::static_pointer_cast<AgoraRteRtcLocalStream>(shared_from_this());
  }

 protected:
  int SetVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track);

  int SetAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track);

  int UnsetVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track);

  int UnsetAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track);

  int CreateRtmpService();

  int DestroyRtmpService();

  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcLocalStreamUserObserver>>
      rtc_local_stream_user_observer_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcLocalStreamObserver>>
      rtc_local_stream_observer_;

  std::shared_ptr<AgoraRteRtcVideoTrackBase> video_track_;
  std::vector<std::shared_ptr<AgoraRteRtcAudioTrackBase>> audio_tracks_;

  Optional<VideoEncoderConfiguration> config_;

  std::vector<agora::rtc::LiveTranscoding>
      transcoding_vec_;  ///< only cache one transcoding
  std::map<std::string, bool> cdnbypass_url_map_;  ///< cache CDN bypass urls
  std::shared_ptr<rtc::IRtmpStreamingService> rtmp_service_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcLocalStreamCdnObserver>>
      rtmp_observer_;
};

class AgoraRteRtcRemoteStream final : public AgoraRteRemoteStream {
 public:
  AgoraRteRtcRemoteStream(
      const std::string& user_id,
      std::shared_ptr<agora::base::IAgoraService> rtc_service,
      const std::string& stream_id, const std::string& rtc_stream_id,
      std::shared_ptr<AgoraRteRtcStreamBase> major_stream);

  ~AgoraRteRtcRemoteStream();

  // Scene will never parallel call override functions below, so we don't need
  // to add lock inside if these functions are only called from scene
  //
  int SubscribeRemoteAudio() override;

  int UnsubscribeRemoteAudio() override;

  int SubscribeRemoteVideo(const VideoSubscribeOptions& options) override;

  int UnsubscribeRemoteVideo() override;

  int SetRemoteVideoCanvas(const VideoCanvas& canvas) override;

  int EnableRemoteVideoObserver() override;

  int DisableRemoveVideoObserver() override;

  int EnableRemoteAudioObserver(
      const RemoteAudioObserverOptions& option) override;

  int DisableRemoteAudioObserver() override;

  int AdjustRemoteVolume(int volume) override;

  int GetRemoteVolume(int* volume) override;

  void OnAudioTrackSubscribed(
      std::shared_ptr<rtc::IRemoteAudioTrack> audio_track) {
    contains_audio_ = true;
    std::lock_guard<std::recursive_mutex> _(callback_lock_);
    audio_track_ = audio_track;
  }

  void OnVideoTrackSubscribed(
      std::shared_ptr<rtc::IRemoteVideoTrack> video_track) {
    std::lock_guard<std::recursive_mutex> _(callback_lock_);
    contains_video_ = true;
    video_track_ = video_track;

    if (render_) {
      video_track_->addRenderer(render_);
    }
  }

  std::shared_ptr<rtc::IRemoteAudioTrack> GetAudioTrack() const {
    return audio_track_;
  }

  std::shared_ptr<rtc::IRemoteVideoTrack> GetVideoTrack() const {
    return video_track_;
  }

 protected:
  // All remote stream operations goes to local rtc stream , this is just follow
  // how current rtc works since rtc doesn't expose remote connection
  //
  std::weak_ptr<AgoraRteRtcStreamBase> rtc_stream_;
  std::recursive_mutex callback_lock_;
  agora_refptr<rtc::IVideoRenderer> render_;
  agora_refptr<rtc::IMediaNodeFactory> media_node_factory_;
  std::string rtc_stream_id_;
  std::shared_ptr<rtc::IRemoteAudioTrack> audio_track_;
  std::shared_ptr<rtc::IRemoteVideoTrack> video_track_;
};

class AgoraRteRtcObserveHelper {
  public:
  static void onPublishStateChanged(
    const char* channel, rtc::STREAM_PUBLISH_STATE oldState,
    rtc::STREAM_PUBLISH_STATE newState, int elapseSinceLastState,
    MediaType type,const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteStream>& stream);
  private:
    AgoraRteRtcObserveHelper() = default;
};

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteAudioTrackImpl.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "IAgoraRteMediaObserver.h"

namespace agora {
namespace rte {

class AgoraRteAudioTrackImpl final
    : public std::enable_shared_from_this<AgoraRteAudioTrackImpl>,
      public AgoraRteTrackImplBase {
 public:
  AgoraRteAudioTrackImpl(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);

  void SetTrack(std::shared_ptr<rtc::ILocalAudioTrack> track);

  int EnableLocalPlayback();

  int AdjustPublishVolume(int volume);

  int AdjustPlayoutVolume(int volume);

  int SetAudioReverbPreset(AUDIO_REVERB_PRESET reverb_preset);

  int SetVoiceChangerPreset(VOICE_CHANGER_PRESET voice_changer_preset);

  int EnableEarMonitor(bool enable, int includeAudioFilter);

  std::shared_ptr<rtc::ILocalAudioTrack> GetAudioTrack() const;

  int Start() override;

  void Stop() override;

 protected:
  std::shared_ptr<rtc::ILocalAudioTrack> audio_track_;
  rtc::AudioEncoderConfiguration config_;
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteWrapperAudioTrack.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//
#pragma once

#include "AgoraRefPtr.h"
#include "AgoraRteBase.h"
#include "IAgoraRteMediaTrack.h"

namespace agora {

namespace rtc {
class ILocalVideoTrack;
class ILocalAudioTrack;
class IMediaStreamingSource;
}  // namespace rtc

namespace rte {

class AgoraRteScene;

class AgoraRteWrapperAudioTrack : public IAgoraRteAudioTrack,
                                  public AgoraRteRtcAudioTrackBase {
 public:
  AgoraRteWrapperAudioTrack(
      std::shared_ptr<agora::base::IAgoraService> rtc_service,
      std::shared_ptr<rtc::ILocalAudioTrack> rtc_audio_track);

  //////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of IAgoraRteAudioTrack ////////////
  ///////////////////////////////////////////////////////////////////////
  int EnableLocalPlayback() override;
  SourceType GetSourceType() override;
  int AdjustPublishVolume(int volume) override;
  int AdjustPlayoutVolume(int volume) override;
  const std::string& GetAttachedStreamId() override;

  ///////////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of AgoraRteRtcAudioTrackBase ////////////
  ////////////////////////////////////////////////////////////////////////////
  std::shared_ptr<agora::rtc::ILocalAudioTrack> GetRtcAudioTrack()
      const override;

  ////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of AgoraRteTrackBase ////////////
  ////////////////////////////////////////////////////////////////////
  void SetStreamId(const std::string& stream_id) override;

 private:
};
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteRtcCompatibleStream.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "IAgoraRteMediaTrack.h"
#include "IAgoraRteScene.h"

namespace agora {
namespace rte {
class AgoraRteRtCompatibleStreamObserver;
class AgoraRteRtcCompatibleUserObserver;
class AgoraRteRtCompatibleAudioFrameObserver;
class AgoraRteRtcCompatibleRemoteVideoObserver;
class AgoraRteRtcCompatibleStreamCdnObserver;

class AgoraRteRtcCompatibleMajorStream final : public AgoraRteRtcStreamBase,
                                               public AgoraRteMajorStream {
 public:
  AgoraRteRtcCompatibleMajorStream(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<agora::base::IAgoraService>& rtc_service,
      const std::string& token, const JoinOptions& option);
  ~AgoraRteRtcCompatibleMajorStream() {
    DestroyRtmpService();
    Disconnect();
  }
  // Inherited via AgoraRteRtcStreamBase
  virtual int RegisterObservers() override;
  virtual void UnregisterObservers() override;
  virtual std::shared_ptr<AgoraRteRtcStreamBase> GetSharedSelf() override;

  // Inherited via AgoraRteMajorStream
  virtual int Connect() override;
  virtual void Disconnect() override;
  virtual int PushUserInfo(const UserInfo& info, InstanceState state) override;
  virtual int PushStreamInfo(const StreamInfo& info,
                             InstanceState state) override;
  virtual int PushMediaInfo(const StreamInfo& info, MediaType media_type,
                            InstanceState state) override;

  void AddRemoteStream(const std::shared_ptr<AgoraRteRtcRemoteStream>& stream);

  void RemoveRemoteStream(const std::string& rtc_stream_id);

  std::shared_ptr<AgoraRteRtcRemoteStream> FindRemoteStream(
      const std::string& stream_id);

  // Interface from AgoraRteLocalStream
  size_t GetAudioTrackCount();
  size_t GetVideoTrackCount();
  int UpdateOption(const StreamOption& option);
  int PublishLocalAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track);
  int PublishLocalVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track);
  int UnpublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track);
  int UnpublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track);
  int SetAudioEncoderConfiguration(const AudioEncoderConfiguration& config);
  int SetVideoEncoderConfiguration(const VideoEncoderConfiguration& config);
  int EnableLocalAudioObserver(const LocalAudioObserverOptions& option);
  int DisableLocalAudioObserver();
  int SetBypassCdnTranscoding(const agora::rtc::LiveTranscoding& transcoding);
  int AddBypassCdnUrl(const std::string& target_cdn_url,
                      bool transcoding_enabled);
  int RemoveBypassCdnUrl(const std::string& target_cdn_url);

  void OnConnected();

  void OnAudioTrackPublished(
      const agora_refptr<rtc::ILocalAudioTrack>& audioTrack);

  void OnVideoTrackPublished(
      const agora_refptr<rtc::ILocalVideoTrack>& audioTrack);

 protected:
  int SetVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track);

  int SetAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track);

  int UnsetVideoTrack(std::shared_ptr<AgoraRteRtcVideoTrackBase> track);

  int UnsetAudioTrack(std::shared_ptr<AgoraRteRtcAudioTrackBase> track);

  int CreateRtmpService();

  int DestroyRtmpService();

 private:
  std::shared_ptr<AgoraRteRtcVideoTrackBase> video_track_;

  std::vector<std::shared_ptr<AgoraRteRtcAudioTrackBase>> audio_tracks_;

  Optional<VideoEncoderConfiguration> config_;

  std::vector<std::shared_ptr<AgoraRteRtcRemoteStream>> remote_streams_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcCompatibleUserObserver>>
      rtc_major_stream_user_observer_;

  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtCompatibleStreamObserver>>
      rtc_major_stream_observer_;

  std::vector<agora::rtc::LiveTranscoding> transcoding_vec_;  ///< only cache one transcoding
  std::map<std::string, bool> cdnbypass_url_map_;  ///< cache CDN bypass urls
  std::shared_ptr<rtc::IRtmpStreamingService> rtmp_service_;
  std::shared_ptr<RtcCallbackWrapper<AgoraRteRtcCompatibleStreamCdnObserver>>
      rtmp_observer_;
};

class AgoraRteRtcCompatibleLocalStream final : public AgoraRteLocalStream {
 public:
  AgoraRteRtcCompatibleLocalStream(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& major_stream);
  ~AgoraRteRtcCompatibleLocalStream() { Disconnect(); }
  // Inherited via AgoraRteLocalStream
  virtual int Connect() override { return ERR_OK; }
  virtual void Disconnect() override {}
  virtual size_t GetAudioTrackCount() override {
    return major_stream_->GetAudioTrackCount();
  }
  virtual size_t GetVideoTrackCount() override {
    return major_stream_->GetVideoTrackCount();
  }

  virtual int UpdateOption(const StreamOption& option) override {
    return major_stream_->UpdateOption(option);
  }
  virtual int PublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track) override {
    return major_stream_->PublishLocalAudioTrack(track);
  }
  virtual int PublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track) override {
    return major_stream_->PublishLocalVideoTrack(track);
  }
  virtual int UnpublishLocalAudioTrack(
      std::shared_ptr<AgoraRteRtcAudioTrackBase> track) override {
    return major_stream_->UnpublishLocalAudioTrack(track);
  }
  virtual int UnpublishLocalVideoTrack(
      std::shared_ptr<AgoraRteRtcVideoTrackBase> track) override {
    return major_stream_->UnpublishLocalVideoTrack(track);
  }
  virtual int SetAudioEncoderConfiguration(
      const AudioEncoderConfiguration& config) override {
    return major_stream_->SetAudioEncoderConfiguration(config);
  }
  virtual int SetVideoEncoderConfiguration(
      const VideoEncoderConfiguration& config) override {
    return major_stream_->SetVideoEncoderConfiguration(config);
  }
  virtual int EnableLocalAudioObserver(
      const LocalAudioObserverOptions& option) override {
    return major_stream_->EnableLocalAudioObserver(option);
  }
  virtual int DisableLocalAudioObserver() override {
    return major_stream_->DisableLocalAudioObserver();
  }
  virtual int SetBypassCdnTranscoding(
      const agora::rtc::LiveTranscoding& transcoding) override {
    return major_stream_->SetBypassCdnTranscoding(transcoding);
  }
  virtual int AddBypassCdnUrl(const std::string& target_cdn_url,
                              bool transcoding_enabled) override {
    return major_stream_->AddBypassCdnUrl(target_cdn_url, transcoding_enabled);
  };
  virtual int RemoveBypassCdnUrl(const std::string& target_cdn_url) override {
    return major_stream_->RemoveBypassCdnUrl(target_cdn_url);
  }

 private:
  std::shared_ptr<AgoraRteRtcCompatibleMajorStream> major_stream_;
};

class AgoraRteRtCompatibleStreamObserver : public rtc::IRtcConnectionObserver {
 public:
  AgoraRteRtCompatibleStreamObserver(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& stream) {
    stream_ = stream;
    scene_ = scene;
  }

  ~AgoraRteRtCompatibleStreamObserver();

  // Inherited via IRtcConnectionObserver
  virtual void onConnected(const rtc::TConnectionInfo& connectionInfo,
                           rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  virtual void onDisconnected(
      const rtc::TConnectionInfo& connectionInfo,
      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  virtual void onConnecting(
      const rtc::TConnectionInfo& connectionInfo,
      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  virtual void onReconnecting(
      const rtc::TConnectionInfo& connectionInfo,
      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  virtual void onReconnected(
      const rtc::TConnectionInfo& connectionInfo,
      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  virtual void onConnectionLost(
      const rtc::TConnectionInfo& connectionInfo) override;
  virtual void onLastmileQuality(const rtc::QUALITY_TYPE quality) override;
  virtual void onLastmileProbeResult(
      const rtc::LastmileProbeResult& result) override;
  virtual void onTokenPrivilegeWillExpire(const char* token) override;
  virtual void onTokenPrivilegeDidExpire() override;
  virtual void onConnectionFailure(
      const rtc::TConnectionInfo& connectionInfo,
      rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  virtual void onUserJoined(user_id_t userId) override;
  virtual void onUserLeft(user_id_t userId,
                          rtc::USER_OFFLINE_REASON_TYPE reason) override;
  virtual void onTransportStats(const rtc::RtcStats& stats) override;
  virtual void onChannelMediaRelayStateChanged(int state, int code) override;

  void TryToFireClosedEvent() { fire_connection_closed_event_ = true; }

 private:
  std::mutex ob_lock_;
  std::weak_ptr<AgoraRteRtcCompatibleMajorStream> stream_;
  std::weak_ptr<AgoraRteScene> scene_;
  bool fire_connection_closed_event_ = false;
  bool is_close_event_failed_ = true;
};

class AgoraRteRtcCompatibleStreamCdnObserver
    : public rtc::IRtmpStreamingObserver {
 public:
  AgoraRteRtcCompatibleStreamCdnObserver(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& stream);

  void onRtmpStreamingStateChanged(
      const char* url, agora::rtc::RTMP_STREAM_PUBLISH_STATE state,
      agora::rtc::RTMP_STREAM_PUBLISH_ERROR err_code) override;
  void onStreamPublished(const char* url, int error) override;
  void onStreamUnpublished(const char* url) override;
  void onTranscodingUpdated() override;

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcCompatibleMajorStream> stream_;
};

class AgoraRteRtcCompatibleUserObserver : public rtc::ILocalUserObserver {
 public:
  AgoraRteRtcCompatibleUserObserver(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& stream);

  // Inherited via ILocalUserObserver
  virtual void onAudioTrackPublishSuccess(
      agora_refptr<rtc::ILocalAudioTrack> audioTrack) override;
  virtual void onAudioTrackPublicationFailure(
      agora_refptr<rtc::ILocalAudioTrack> audioTrack,
      ERROR_CODE_TYPE error) override;
  virtual void onLocalAudioTrackStateChanged(
      agora_refptr<rtc::ILocalAudioTrack> audioTrack,
      rtc::LOCAL_AUDIO_STREAM_STATE state,
      rtc::LOCAL_AUDIO_STREAM_ERROR errorCode) override;
  virtual void onLocalAudioTrackStatistics(
      const rtc::LocalAudioStats& stats) override;
  virtual void onRemoteAudioTrackStatistics(
      agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
      const rtc::RemoteAudioTrackStats& stats) override;
  virtual void onUserAudioTrackSubscribed(
      user_id_t userId,
      agora_refptr<rtc::IRemoteAudioTrack> audioTrack) override;
  virtual void onUserAudioTrackStateChanged(
      user_id_t userId, agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
      rtc::REMOTE_AUDIO_STATE state, rtc::REMOTE_AUDIO_STATE_REASON reason,
      int elapsed) override;
  virtual void onVideoTrackPublishSuccess(
      agora_refptr<rtc::ILocalVideoTrack> videoTrack) override;
  virtual void onVideoTrackPublicationFailure(
      agora_refptr<rtc::ILocalVideoTrack> videoTrack,
      ERROR_CODE_TYPE error) override;
  virtual void onLocalVideoTrackStateChanged(
      agora_refptr<rtc::ILocalVideoTrack> videoTrack,
      rtc::LOCAL_VIDEO_STREAM_STATE state,
      rtc::LOCAL_VIDEO_STREAM_ERROR errorCode) override;
  virtual void onLocalVideoTrackStatistics(
      agora_refptr<rtc::ILocalVideoTrack> videoTrack,
      const rtc::LocalVideoTrackStats& stats) override;
  virtual void onUserVideoTrackSubscribed(
      user_id_t userId, rtc::VideoTrackInfo trackInfo,
      agora_refptr<rtc::IRemoteVideoTrack> videoTrack) override;
  virtual void onUserVideoTrackStateChanged(
      user_id_t userId, agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
      rtc::REMOTE_VIDEO_STATE state, rtc::REMOTE_VIDEO_STATE_REASON reason,
      int elapsed) override;
  virtual void onRemoteVideoTrackStatistics(
      agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
      const rtc::RemoteVideoTrackStats& stats) override;
  virtual void onAudioVolumeIndication(const rtc::AudioVolumeInfo* speakers,
                                       unsigned int speakerNumber,
                                       int totalVolume) override;
  virtual void onAudioSubscribeStateChanged(
      const char* channel, user_id_t userId,
      rtc::STREAM_SUBSCRIBE_STATE oldState,
      rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) override;
  virtual void onVideoSubscribeStateChanged(
      const char* channel, user_id_t userId,
      rtc::STREAM_SUBSCRIBE_STATE oldState,
      rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) override;
  virtual void onAudioPublishStateChanged(const char* channel,
                                          rtc::STREAM_PUBLISH_STATE oldState,
                                          rtc::STREAM_PUBLISH_STATE newState,
                                          int elapseSinceLastState) override;
  virtual void onVideoPublishStateChanged(const char* channel,
                                          rtc::STREAM_PUBLISH_STATE oldState,
                                          rtc::STREAM_PUBLISH_STATE newState,
                                          int elapseSinceLastState) override;

  void onSubscribeStateChangedCommon(const char* channel, user_id_t userId,
                                     rtc::STREAM_SUBSCRIBE_STATE oldState,
                                     rtc::STREAM_SUBSCRIBE_STATE newState,
                                     int elapseSinceLastState, MediaType type);

  void onPublishStateChangedCommon(const char* channel,
                                   rtc::STREAM_PUBLISH_STATE oldState,
                                   rtc::STREAM_PUBLISH_STATE newState,
                                   int elapseSinceLastState, MediaType type);

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcCompatibleMajorStream> stream_;
};

class AgoraRteRtCompatibleAudioFrameObserver
    : public agora::media::IAudioFrameObserver {
 public:
  AgoraRteRtCompatibleAudioFrameObserver(
      const std::shared_ptr<AgoraRteScene>& scene,
      const std::shared_ptr<AgoraRteRtcStreamBase>& stream);

  // Inherited via IAudioFrameObserver
  virtual bool onRecordAudioFrame(AudioFrame& audioFrame) override;
  virtual bool onPlaybackAudioFrame(AudioFrame& audioFrame) override;
  virtual bool onMixedAudioFrame(AudioFrame& audioFrame) override;
  virtual bool onPlaybackAudioFrameBeforeMixing(
    user_id_t userId, AudioFrame& audioFrame) override;

 private:
  std::weak_ptr<AgoraRteScene> scene_;
  std::weak_ptr<AgoraRteRtcStreamBase> stream_;
};

class AgoraRteRtcCompatibleRemoteVideoObserver
    : public rtc::IVideoFrameObserver2 {
 public:
  AgoraRteRtcCompatibleRemoteVideoObserver(
      const std::shared_ptr<AgoraRteScene>& scene);

  // Inherited via IVideoFrameObserver2
  virtual void onFrame(user_id_t uid, rtc::conn_id_t connectionId,
                       const media::base::VideoFrame* frame) override;

 private:
  std::weak_ptr<AgoraRteScene> scene_;
};

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteStreamingSource.h ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include <memory>

#include "AgoraRefPtr.h"
#include "AgoraRteBase.h"
#include "IAgoraRteStreamingSource.h"

namespace agora {
namespace rte {

class AgoraRteScene;
class AgoraRtePlayList;

class AgoraRteStreamingSource
    : public std::enable_shared_from_this<AgoraRteStreamingSource>,
      public IAgoraRteStreamingSource,
      public agora::rtc::IMediaStreamingSourceObserver {
 public:
  AgoraRteStreamingSource(
      std::shared_ptr<agora::base::IAgoraService> rtc_service);
  virtual ~AgoraRteStreamingSource();

  //////////////////////////////////////////////////////////////////////////
  ///////////////// Override Methods of IAgoraRteStreamingSource ////////////
  ///////////////////////////////////////////////////////////////////////////
  std::shared_ptr<IAgoraRtePlayList> CreatePlayList() override;
  std::shared_ptr<IAgoraRteVideoTrack> GetRteVideoTrack() override;
  std::shared_ptr<IAgoraRteAudioTrack> GetRteAudioTrack() override;
  int Open(const char* url, int64_t start_pos, bool auto_play = true) override;
  int Open(std::shared_ptr<IAgoraRtePlayList> in_play_list, int64_t start_pos,
           bool auto_play = true) override;
  int Close() override;
  bool isVideoValid() override;
  bool isAudioValid() override;
  int GetDuration(int64_t& duration) override;
  int GetStreamCount(int64_t& count) override;
  int GetStreamInfo(int64_t index,
                    media::base::PlayerStreamInfo* out_info) override;
  int SetLoopCount(int64_t loop_count) override;
  int Play() override;
  int Pause() override;
  int Stop() override;
  int Seek(int64_t new_pos) override;
  int SeekToPrev(int64_t pos) override;
  int SeekToNext(int64_t pos) override;
  int SeekToFile(int32_t file_id, int64_t pos) override;
  int GetStreamingSourceStatus(
      RteStreamingSourceStatus& out_source_status) override;
  int GetCurrPosition(int64_t& pos) override;
  agora::rtc::STREAMING_SRC_STATE GetCurrState() override;
  int AppendSeiData(const agora::rtc::InputSeiData& inSeiData) override;
  int RegisterObserver(
      std::shared_ptr<IAgoraRteStreamingSourceObserver> observer) override;
  int UnregisterObserver(
      std::shared_ptr<IAgoraRteStreamingSourceObserver> observer) override;

  void GetCurrFileInfo(RteFileInfo& out_current_file);
  int ProcessOneMediaCompleted();

  //////////////////////////////////////////////////////////////////////////////
  /////////////// Override Methods of IMediaStreamingSourceObserver ////////////
  ///////////////////////////////////////////////////////////////////////////////
  void onStateChanged(agora::rtc::STREAMING_SRC_STATE state,
                      agora::rtc::STREAMING_SRC_ERR err_code) override;
  void onOpenDone(agora::rtc::STREAMING_SRC_ERR err_code) override;
  void onSeekDone(agora::rtc::STREAMING_SRC_ERR err_code) override;
  void onEofOnce(int64_t progress_ms, int64_t repeat_count) override;
  void onProgress(int64_t position_ms) override;
  void onMetaData(const void* data, int length) override;

 protected:
  int32_t ConvertErrCode(int32_t src_err_code);

 protected:
  std::shared_ptr<agora::base::IAgoraService> rtc_service_;
  agora_refptr<rtc::IMediaNodeFactory> rtc_node_factory_;
  agora_refptr<rtc::IMediaStreamingSource> rtc_streaming_source_;
  std::shared_ptr<AgoraRteWrapperVideoTrack> rte_video_track_;
  std::shared_ptr<AgoraRteWrapperAudioTrack> rte_audio_track_;

  std::vector<std::weak_ptr<IAgoraRteStreamingSourceObserver>>
      observer_list_;  // GUARDED_BY(rte_streaming_src_lock_);

  std::recursive_mutex streaming_source_lock_;  ///< locker for streaming source
  std::shared_ptr<AgoraRtePlayList> play_list_ = nullptr;  ///< play list
  RteFileInfo current_file_;  ///< current file information
  int32_t first_file_id_ =
      INVALID_RTE_FILE_ID;           ///< The file id of start file in list
  int64_t setting_loop_count = 1;    ///< default play count is only once
  bool setting_mute_ = false;        ///< default play is unmute
  int64_t list_played_count_ = 0;    ///< the played count of list
  bool auto_play_for_open_ = false;  ///< whether open for play list
};

}  // namespace rte
}  // namespace agora
// ======================= AgoraRteAudioDeviceManager.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "IAudioDeviceManager.h"

namespace agora {
namespace rte {

#if RTE_WIN || RTE_MAC

inline AgoraRteAudioDeviceCollection::AgoraRteAudioDeviceCollection(
    rtc::IAudioDeviceCollection* collection)
    : collection_(collection) {
  if (!collection) {
    RTE_LOG_ERROR << "collection is null ";
    return;
  }
}

inline int AgoraRteAudioDeviceCollection::GetCount() {
  if (collection_) {
    return collection_->getCount();
  }
  return -ERR_FAILED;
}

inline int AgoraRteAudioDeviceCollection::GetDevice(
    int index, char deviceName[MAX_DEVICE_ID_LENGTH],
    char deviceId[MAX_DEVICE_ID_LENGTH]) {
  if (collection_) {
    return collection_->getDevice(index, deviceName, deviceId);
  }
  return -ERR_FAILED;
}

inline int AgoraRteAudioDeviceCollection::SetDevice(
    const char deviceId[MAX_DEVICE_ID_LENGTH]) {
  if (collection_) {
    return collection_->setDevice(deviceId);
  }
  return -ERR_FAILED;
}

inline int AgoraRteAudioDeviceCollection::SetApplicationVolume(int volume) {
  if (collection_) {
    return collection_->setApplicationVolume(volume);
  }
  return -ERR_FAILED;
}

inline int AgoraRteAudioDeviceCollection::GetApplicationVolume(
    int& volume) {
  if (collection_) {
    return collection_->getApplicationVolume(volume);
  }
  return -ERR_FAILED;
}

inline int AgoraRteAudioDeviceCollection::MuteApplication(bool mute) {
  if (collection_) {
    return collection_->setApplicationMute(mute);
  }
  return -ERR_FAILED;
}

inline int AgoraRteAudioDeviceCollection::IsApplicationMuted(bool& mute) {
  if (collection_) {
    return collection_->isApplicationMute(mute);
  }
  return -ERR_FAILED;
}

inline void AgoraRteAudioDeviceCollection::Release() {
  if (collection_) {
    return collection_->release();
  }
}

inline AgoraRteAudioDeviceManager::AgoraRteAudioDeviceManager(
    base::IAgoraService* service, rtc::IAudioDeviceManagerObserver* observer) {
  if (!service) {
    RTE_LOG_ERROR << "service is null ";
    return;
  }
  audio_device_manager_ = service->createAudioDeviceManagerComponent(observer);
}

inline AgoraRteAudioDeviceManager::~AgoraRteAudioDeviceManager() {
  audio_device_manager_ = nullptr;
}

inline IAgoraRteAudioDeviceCollection*
AgoraRteAudioDeviceManager::EnumeratePlaybackDevices() {
  auto rtc_collection = audio_device_manager_->enumeratePlaybackDevices();
  if (!rtc_collection) {
    return nullptr;
  }
  auto rte_collection =
      std::make_unique<AgoraRteAudioDeviceCollection>(rtc_collection);
  return rte_collection.release();
}

inline IAgoraRteAudioDeviceCollection*
AgoraRteAudioDeviceManager::EnumerateRecordingDevices() {
  auto rtc_collection = audio_device_manager_->enumerateRecordingDevices();
  if (!rtc_collection) {
    return nullptr;
  }
  auto rte_collection =
      std::make_unique<AgoraRteAudioDeviceCollection>(rtc_collection);
  return rte_collection.release();
}

inline int AgoraRteAudioDeviceManager::SetPlaybackDevice(
    const char deviceId[MAX_DEVICE_ID_LENGTH]) {
  return audio_device_manager_->setPlaybackDevice(deviceId);
}

inline int AgoraRteAudioDeviceManager::GetPlaybackDevice(
    char deviceId[MAX_DEVICE_ID_LENGTH]) {
  return audio_device_manager_->getPlaybackDevice(deviceId);
}

inline int AgoraRteAudioDeviceManager::SetPlaybackDeviceVolume(int volume) {
  return audio_device_manager_->setPlaybackDeviceVolume(volume);
}

inline int AgoraRteAudioDeviceManager::GetPlaybackDeviceVolume(
    int* volume) {
  return audio_device_manager_->getPlaybackDeviceVolume(volume);
}

inline int AgoraRteAudioDeviceManager::SetRecordingDevice(
    const char deviceId[MAX_DEVICE_ID_LENGTH]) {
  return audio_device_manager_->setRecordingDevice(deviceId);
}

inline int AgoraRteAudioDeviceManager::GetRecordingDevice(
    char deviceId[MAX_DEVICE_ID_LENGTH]) {
  return audio_device_manager_->getRecordingDevice(deviceId);
}

inline int AgoraRteAudioDeviceManager::SetRecordingDeviceVolume(
    int volume) {
  return audio_device_manager_->setRecordingDeviceVolume(volume);
}

inline int AgoraRteAudioDeviceManager::GetRecordingDeviceVolume(
    int* volume) {
  return audio_device_manager_->getRecordingDeviceVolume(volume);
}

inline int AgoraRteAudioDeviceManager::MutePlaybackDevice(bool mute) {
  return audio_device_manager_->setPlaybackDeviceMute(mute);
}

inline int AgoraRteAudioDeviceManager::IsPlaybackDeviceMuted(bool& muted) {
  return audio_device_manager_->getPlaybackDeviceMute(&muted);
}

inline int AgoraRteAudioDeviceManager::MuteRecordingDevice(bool mute) {
  return audio_device_manager_->setRecordingDeviceMute(mute);
}

inline int AgoraRteAudioDeviceManager::IsRecordingDeviceMuted(bool& muted) {
  return audio_device_manager_->getRecordingDeviceMute(&muted);
}

inline int AgoraRteAudioDeviceManager::StartPlaybackDeviceTest(
    const char* testAudioFilePath) {
  return audio_device_manager_->startPlaybackDeviceTest(testAudioFilePath);
}

inline int AgoraRteAudioDeviceManager::StopPlaybackDeviceTest() {
  return audio_device_manager_->stopPlaybackDeviceTest();
}

inline int AgoraRteAudioDeviceManager::StartRecordingDeviceTest(
    int indicationInterval) {
  return audio_device_manager_->startRecordingDeviceTest(indicationInterval);
}

inline int AgoraRteAudioDeviceManager::StopRecordingDeviceTest() {
  return audio_device_manager_->stopRecordingDeviceTest();
}

inline int AgoraRteAudioDeviceManager::StartDeviceLoopbackTest(
    int indicationInterval) {
  return audio_device_manager_->startAudioDeviceLoopbackTest(
      indicationInterval);
}

inline int AgoraRteAudioDeviceManager::StopDeviceLoopbackTest() {
  return audio_device_manager_->stopAudioDeviceLoopbackTest();
}

#endif

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteAudioTrackImpl.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "NGIAgoraAudioTrack.h"

namespace agora {
namespace rte {

inline AgoraRteAudioTrackImpl::AgoraRteAudioTrackImpl(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  rtc_service_ = rtc_service;

  auto media_node_factory = rtc_service->createMediaNodeFactory();

  media_node_factory_ =
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IMediaNodeFactory>(
          media_node_factory);
}

inline void AgoraRteAudioTrackImpl::SetTrack(
    std::shared_ptr<rtc::ILocalAudioTrack> track) {
  audio_track_ = track;
}

inline int AgoraRteAudioTrackImpl::EnableLocalPlayback() {
  return audio_track_->enableLocalPlayback(true);
}

inline int AgoraRteAudioTrackImpl::AdjustPublishVolume(int volume) {
  return audio_track_->adjustPublishVolume(volume);
}

inline int AgoraRteAudioTrackImpl::AdjustPlayoutVolume(int volume) {
  return audio_track_->adjustPlayoutVolume(volume);
}

inline int AgoraRteAudioTrackImpl::SetAudioReverbPreset(
    AUDIO_REVERB_PRESET reverb_preset) {
  // int result = -ERR_FAILED;
  // auto reverb_filter =
  // audio_track_->getAudioFilter(BUILTIN_AUDIO_FILTER_REVERB); if
  // (!reverb_filter) {
  //
  //  reverb_filter =
  //  media_node_factory_->createAudioFilter(BUILTIN_AUDIO_FILTER_REVERB); if
  //  (!reverb_filter) {
  //    RTE_LOG_ERROR << "failed to create reverb audio filter";
  //    return -ERR_FAILED;
  //  }

  //  result = audio_track_->addAudioFilter(reverb_filter,
  //  rtc::IAudioTrack::Default); if (result != ERR_OK) {
  //    RTE_LOG_ERROR << "failed to add reverb audio filter";
  //    return result;
  //  }

  //  auto voice_reshaper_filter =
  //  audio_track_->getAudioFilter(BUILTIN_AUDIO_FILTER_VOICE_RESHAPER); if
  //  (voice_reshaper_filter
  //  && voice_reshaper_filter->isEnabled()) {
  //    voice_reshaper_filter->setEnabled(false);
  //  }

  //  // enable reverb filter if preset is not off, otherwise, disable it
  //  if (reverb_preset != rtc::AUDIO_REVERB_OFF) {
  //    result = reverb_filter->setProperty("preset", &reverb_preset,
  //    sizeof(reverb_preset)); if (result != ERR_OK) {
  //      RTE_LOG_ERROR << "failed to set property for reverb audio filter";
  //      return result;
  //    }

  //    reverb_filter->setEnabled(true);
  //  } else {
  //    reverb_filter->setEnabled(false);
  //  }
  //}

  return ERR_OK;
}

inline int AgoraRteAudioTrackImpl::SetVoiceChangerPreset(
    VOICE_CHANGER_PRESET voice_changer_preset) {
  // int result = -ERR_FAILED;
  // auto voice_reshaper_filter =
  // audio_track_->getAudioFilter(BUILTIN_AUDIO_FILTER_VOICE_RESHAPER); if
  // (!voice_reshaper_filter) {
  //   voice_reshaper_filter =
  //            media_node_factory_->createAudioFilter(BUILTIN_AUDIO_FILTER_VOICE_RESHAPER))
  //  if (!voice_reshaper_filter) {
  //    RTE_LOG_ERROR << "failed to create voice reshaper audio filter";
  //    return result;
  //  }

  //  result =
  //      audio_track_->addAudioFilter(voice_reshaper_filter,
  //      rtc::IAudioTrack::Default);
  //  if (result != ERR_OK) {
  //    RTE_LOG_ERROR << "failed to add voice reshaper audio filter";
  //    return result;
  //  }
  //}

  //// stop reverb filter if we have it and already enabled
  // auto reverb_filter =
  // audio_track_->getAudioFilter(BUILTIN_AUDIO_FILTER_REVERB); if
  // (reverb_filter && reverb_filter->isEnabled()) {
  //  reverb_filter->setEnabled(false);
  //}

  //// enable voice reshaper filter if preset is not off, otherwise, disable it
  // if (voice_changer_preset != rtc::VOICE_CHANGER_OFF) {
  //  result = voice_reshaper_filter->setProperty("preset",
  //  &voice_changer_preset,
  //                                              sizeof(voice_changer_preset));
  //  if (result != ERR_OK) {
  //    RTE_LOG_ERROR << "failed to set property for voice reshaper filter";
  //    return result;
  //  }

  //  voice_reshaper_filter->setEnabled(true);
  //} else {
  //  voice_reshaper_filter->setEnabled(false);
  //}

  return ERR_OK;
}

inline int AgoraRteAudioTrackImpl::EnableEarMonitor(
    bool enable, int includeAudioFilter) {
  audio_track_->enableEarMonitor(enable, includeAudioFilter);
  return ERR_OK;
}

inline std::shared_ptr<agora::rtc::ILocalAudioTrack>
AgoraRteAudioTrackImpl::GetAudioTrack() const {
  return audio_track_;
}

inline int AgoraRteAudioTrackImpl::Start() {
  std::lock_guard<std::mutex> _(track_lock_);
  if (is_started_) {
    return ERR_OK;
  }

  bool enable_track = true;
  audio_track_->setEnabled(enable_track);

  is_started_ = true;

  return ERR_OK;
}

inline void AgoraRteAudioTrackImpl::Stop() {
  std::lock_guard<std::mutex> _(track_lock_);
  bool enable_playback = false;
  audio_track_->enableLocalPlayback(enable_playback);
  bool enable_track = false;
  audio_track_->setEnabled(enable_track);

  is_started_ = false;
}
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCameraVideoTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteCameraObserver::AgoraRteCameraObserver(
    const std::shared_ptr<AgoraRteCameraVideoTrack>& camera_track) {
  camera_track_ = camera_track;
}

inline void AgoraRteCameraObserver::onCameraStateChanged(
    ICameraCapturer::CAMERA_STATE state,
    ICameraCapturer::CAMERA_SOURCE source) {
  auto camera_track = camera_track_.lock();
  if (camera_track) {
    auto& callback = camera_track->GetCallback();
    if (callback) {
      callback(state == ICameraCapturer::CAMERA_STATE::CAMERA_STARTED
                   ? CameraState::kStarted
                   : CameraState::kStopped,
               source == ICameraCapturer::CAMERA_SOURCE::CAMERA_BACK
                   ? CameraSource::kBack
                   : CameraSource::kFront);
    }
  }
}

inline void AgoraRteCameraVideoTrack::Init() {
  auto this_shared =
      std::static_pointer_cast<AgoraRteCameraVideoTrack>(shared_from_this());
  camera_ob_ = std::make_shared<AgoraRteCameraObserver>(this_shared);
  camera_capturer_->registerCameraObserver(camera_ob_.get());
}

inline rtc::ICameraCapturer::CAMERA_SOURCE
AgoraRteCameraVideoTrack::GetRtcCameraSource(CameraSource src) {
  switch (src) {
    case agora::rte::CameraSource::kFront:
      return rtc::ICameraCapturer::CAMERA_FRONT;
      break;
    case agora::rte::CameraSource::kBack:
      return rtc::ICameraCapturer::CAMERA_BACK;
      break;
    default:
      return rtc::ICameraCapturer::CAMERA_FRONT;
      break;
  }
}

#if RTE_IPHONE || RTE_ANDROID
inline int AgoraRteCameraVideoTrack::SetCameraSource(CameraSource source) {
  int result = -ERR_FAILED;
  rtc::ICameraCapturer::CAMERA_SOURCE src =
      rtc::ICameraCapturer::CAMERA_SOURCE::CAMERA_FRONT;

  if (source == CameraSource::kBack) {
    src = rtc::ICameraCapturer::CAMERA_SOURCE::CAMERA_BACK;
  }

  std::lock_guard<std::mutex> _(camera_track_lock_);
  {
    result = camera_capturer_->setCameraSource(src);

    if (result == ERR_OK) {
      is_camera_source_assigned_ = true;
    }
  }

  return result;
}

inline void AgoraRteCameraVideoTrack::SwitchCamera() {
  camera_capturer_->switchCamera();
}

inline int AgoraRteCameraVideoTrack::SetCameraZoom(float zoomValue) {
  return camera_capturer_->setCameraZoom(zoomValue);
}

inline int AgoraRteCameraVideoTrack::SetCameraFocus(float x, float y) {
  return camera_capturer_->setCameraFocus(x, y);
}

inline int AgoraRteCameraVideoTrack::SetCameraAutoFaceFocus(bool enable) {
  return camera_capturer_->setCameraAutoFaceFocus(enable);
}

inline int AgoraRteCameraVideoTrack::SetCameraFaceDetection(bool enable) {
  // TODO(minbo) Align the Arsenal
  return camera_capturer_->enableFaceDetection(enable);
}
#elif RTE_DESKTOP

inline int AgoraRteCameraVideoTrack::SetCameraDevice(
    const std::string& device_id) {
  int result = -ERR_FAILED;
  std::lock_guard<std::mutex> _(camera_track_lock_);
  {
    result = camera_capturer_->initWithDeviceId(device_id.c_str());

    if (result == ERR_OK) {
      is_camera_source_assigned_ = true;
    }
  }

  return result;
}

inline void AgoraRteCameraVideoTrack::SetDeviceOrientation(
    VIDEO_ORIENTATION orientation) {
  camera_capturer_->setDeviceOrientation(orientation);
}

#endif

inline int AgoraRteCameraVideoTrack::SetPreviewCanvas(
    const VideoCanvas& canvas) {
  return track_impl_->SetPreviewCanvas(canvas);
}

inline SourceType AgoraRteCameraVideoTrack::GetSourceType() {
  return SourceType::kVideo_Camera;
}

inline void AgoraRteCameraVideoTrack::RegisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->RegisterLocalVideoFrameObserver(observer);
}

inline void AgoraRteCameraVideoTrack::UnregisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->UnregisterLocalVideoFrameObserver(observer);
}

inline int AgoraRteCameraVideoTrack::EnableVideoFilter(
    const std::string& id, bool enable) {
  return track_impl_->GetVideoTrack()->enableVideoFilter(id.c_str(), enable);
}

inline int AgoraRteCameraVideoTrack::SetFilterProperty(
    const std::string& id, const std::string& key,
    const std::string& json_value) {
  return track_impl_->GetVideoTrack()->setFilterProperty(
      id.c_str(), key.c_str(), json_value.c_str());
}

inline std::string AgoraRteCameraVideoTrack::GetFilterProperty(
    const std::string& id, const std::string& key) {
  char buffer[1024];

  if (track_impl_->GetVideoTrack()->getFilterProperty(id.c_str(), key.c_str(),
                                                      buffer, 1024) == ERR_OK) {
    return buffer;
  }

  return "";
}

inline int AgoraRteCameraVideoTrack::StartCapture(
    CameraCallbackFun callback) {
  int result = ERR_OK;

  callback_ = callback;

  if (!is_camera_source_assigned_) {
    std::lock_guard<std::mutex> _(camera_track_lock_);
    if (!is_camera_source_assigned_) {
      is_camera_source_assigned_ = true;
#if RTE_IPHONE || RTE_ANDROID
      result =
          camera_capturer_->setCameraSource(rtc::ICameraCapturer::CAMERA_FRONT);

#elif RTE_DESKTOP
      constexpr int max_device_name = 260;
      result = -ERR_FAILED;

      std::unique_ptr<rtc::ICameraCapturer::IDeviceInfo> device_info(
          camera_capturer_->createDeviceInfo());
      if (device_info && device_info->NumberOfDevices() > 0) {
        std::string device_id = "";

        auto device_name = std::make_unique<char[]>(max_device_name);
        auto device_uuid = std::make_unique<char[]>(max_device_name);
        auto id = std::make_unique<char[]>(max_device_name);
        if (device_info->GetDeviceName(0, device_name.get(), max_device_name,
                                       id.get(), max_device_name,
                                       device_uuid.get(),
                                       max_device_name) == ERR_OK) {
          device_id.assign(id.get());
        }

        if (!device_id.empty()) {
          result = camera_capturer_->initWithDeviceId(device_id.c_str());
        }
      }
#endif
    }
  }

  if (result == ERR_OK) {
    result = track_impl_->Start();
  }

  return result;
}

inline void AgoraRteCameraVideoTrack::StopCapture() { track_impl_->Stop(); }

inline void AgoraRteCameraVideoTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

inline const std::string& AgoraRteCameraVideoTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

inline std::shared_ptr<agora::rtc::ILocalVideoTrack>
AgoraRteCameraVideoTrack::GetRtcVideoTrack() const {
  return track_impl_->GetVideoTrack();
}

inline AgoraRteCameraVideoTrack::AgoraRteCameraVideoTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  auto track_impl = std::make_shared<AgoraRteVideoTrackImpl>(rtc_service);

  auto media_node_factory = track_impl->GetMediaNodeFactory();

  camera_capturer_ = media_node_factory->createCameraCapturer();

  auto track = rtc_service->createCameraVideoTrack(camera_capturer_);
  track_impl->SetTrack(
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalVideoTrack>(
          track));

  track_impl_ = track_impl;
}

inline AgoraRteCameraVideoTrack::~AgoraRteCameraVideoTrack() {
  camera_capturer_->unregisterCameraObserver(camera_ob_.get());
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCdnStream.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "AgoraRteBase.h"

namespace agora {
namespace rte {

inline AgoraRteCdnLocalStream::AgoraRteCdnLocalStream(
    std::shared_ptr<AgoraRteScene> scene,
    std::shared_ptr<agora::base::IAgoraService> rtc_service,
    const std::string& stream_id, const std::string& url)
    : AgoraRteLocalStream(scene->GetLocalUserInfo().user_id, stream_id,
                          StreamType::kCdnStream),
      scene_(scene),
      url_(url),
      stream_id_(stream_id),
      rtc_service_(rtc_service) {
  createRtmpConnection();
}

inline int AgoraRteCdnLocalStream::Connect() {
  if (url_.empty()) {
    return -ERR_INVALID_ARGUMENT;
  }

  int ret = createRtmpConnection();
  if (ret < 0) {
    return ret;
  }

  ret = rtmp_conn_->connect(url_.c_str());
  if (ret < 0) {
    RTE_LOG_ERROR << "Rtmp connect failed!";
    return ret;
  }

  streaming_state_ = STATE_PUBLISHING;
  return ERR_OK;
}

inline void AgoraRteCdnLocalStream::Disconnect() {
  stopDirectCdnStreamingInternal();
}

inline size_t AgoraRteCdnLocalStream::GetAudioTrackCount() {
  return published_audio_tracks_.size();
}

inline size_t AgoraRteCdnLocalStream::GetVideoTrackCount() {
  return published_video_track_ ? 1 : 0;
}

inline int AgoraRteCdnLocalStream::UpdateOption(
    const StreamOption& option) {
  return agora::ERR_OK;
}

inline int AgoraRteCdnLocalStream::PublishLocalAudioTrack(
    std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  int ret = ERR_OK;
  if (!local_user_) {
    ret = Connect();
    if (ret != ERR_OK || !local_user_) {
      return ret;
    }
  }

  agora_refptr<rtc::ILocalAudioTrack> rtc_track =
      track->GetRtcAudioTrack().get();
  auto it = published_audio_tracks_.find(rtc_track);
  if (it != published_audio_tracks_.end()) {
    return ERR_OK;
  }
  published_audio_tracks_[rtc_track] = StreamMediaState::kIdle;
  local_user_->setAudioStreamConfiguration(audio_encoder_config_);
  ret = local_user_->publishAudio(rtc_track);
  return ret;
}

inline int AgoraRteCdnLocalStream::PublishLocalVideoTrack(
    std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  int ret = ERR_OK;
  if (!local_user_) {
    ret = Connect();
    if (ret != ERR_OK || !local_user_) {
      return ret;
    }
  }

  agora_refptr<rtc::ILocalVideoTrack> rtc_track =
      track->GetRtcVideoTrack().get();
  if (published_video_track_ == rtc_track) {
    return ERR_OK;
  } else if (published_video_track_) {
    return -ERR_FAILED;
  }

  published_video_track_ = rtc_track;

  if (is_video_encoder_set_) {
    ret = published_video_track_->setVideoEncoderConfiguration(
        track_video_encoder_config_);
    if (ret != ERR_OK) return ret;

    ret = local_user_->setVideoStreamConfiguration(video_encoder_config_);

    if (ret != ERR_OK) return ret;
  }

  ret = local_user_->publishVideo(rtc_track);
  return ret;
}

inline int AgoraRteCdnLocalStream::UnpublishLocalAudioTrack(
    std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  if (!local_user_) {
    return -ERR_NOT_INITIALIZED;
  }

  agora_refptr<rtc::ILocalAudioTrack> rtc_track =
      track->GetRtcAudioTrack().get();
  published_audio_tracks_.erase(rtc_track);

  int ret = local_user_->unpublishAudio(rtc_track);

  StreamMediaState old_state = StreamMediaState::kIdle;
  auto it = published_audio_tracks_.find(rtc_track);
  if (it != published_audio_tracks_.end()) {
    old_state = it->second;
    it->second = StreamMediaState::kIdle;
  }

  FireStreamStateChange(MediaType::kAudio, old_state, StreamMediaState::kIdle,
                        StreamStateChangedReason::kPublished);

  return ret;
}

inline int AgoraRteCdnLocalStream::UnpublishLocalVideoTrack(
    std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  if (!local_user_) {
    return -ERR_NOT_INITIALIZED;
  }

  agora_refptr<rtc::ILocalVideoTrack> rtc_track =
      track->GetRtcVideoTrack().get();
  if (published_video_track_ == rtc_track) {
    published_video_track_ = nullptr;
  }

  int ret = local_user_->unpublishVideo(rtc_track);

  StreamMediaState old_state = published_video_stream_state_;
  published_video_stream_state_ = StreamMediaState::kIdle;
  FireStreamStateChange(MediaType::kVideo, old_state, StreamMediaState::kIdle,
                        StreamStateChangedReason::kPublished);
  return ret;
}

inline int AgoraRteCdnLocalStream::SetAudioEncoderConfiguration(
    const AudioEncoderConfiguration& config) {
  rtc::RtmpStreamingAudioConfiguration cfg;
  switch (config.audioProfile) {
    case rtc::AUDIO_PROFILE_MUSIC_STANDARD:
    case rtc::AUDIO_PROFILE_MUSIC_HIGH_QUALITY:
    case rtc::AUDIO_PROFILE_MUSIC_STANDARD_STEREO:
    case rtc::AUDIO_PROFILE_MUSIC_HIGH_QUALITY_STEREO: {
      cfg.sampleRateHz = 48000;
      cfg.bytesPerSample = 2;
      cfg.numberOfChannels = 2;
      cfg.bitrate = 128000;
    } break;

    default: {
      cfg.sampleRateHz = 48000;
      cfg.bytesPerSample = 2;
      cfg.numberOfChannels = 1;
      cfg.bitrate = 96000;
    } break;
  }
  audio_encoder_config_ = std::move(cfg);
  if (local_user_) {
    return local_user_->setAudioStreamConfiguration(audio_encoder_config_);
  }

  return ERR_OK;
}

inline int AgoraRteCdnLocalStream::SetVideoEncoderConfiguration(
    const VideoEncoderConfiguration& config) {
  track_video_encoder_config_ = config;

  rtc::RtmpStreamingVideoConfiguration cfg;
  cfg.width = config.dimensions.width;
  cfg.height = config.dimensions.height;
  cfg.framerate = config.frameRate;
  cfg.bitrate = config.bitrate ? config.bitrate : 800;
  track_video_encoder_config_.bitrate = cfg.bitrate;
  cfg.minBitrate =
      config.minBitrate == rtc::DEFAULT_MIN_BITRATE ? 0 : config.minBitrate;
  track_video_encoder_config_.minBitrate = cfg.minBitrate;
  cfg.maxBitrate = config.bitrate;
  cfg.orientationMode = config.orientationMode;
  is_video_encoder_set_ = true;

  video_encoder_config_ = std::move(cfg);
  if (local_user_) {
    if (published_video_track_) {
      int result = published_video_track_->setVideoEncoderConfiguration(
          track_video_encoder_config_);
      if (result != ERR_OK) return result;
    }

    return local_user_->setVideoStreamConfiguration(video_encoder_config_);
  }

  return ERR_OK;
}

inline int AgoraRteCdnLocalStream::EnableLocalAudioObserver(
    const LocalAudioObserverOptions& option) {
  return -ERR_NOT_SUPPORTED;
}

inline int AgoraRteCdnLocalStream::DisableLocalAudioObserver() {
  return -ERR_NOT_SUPPORTED;
}

inline int AgoraRteCdnLocalStream::SetBypassCdnTranscoding(
    const agora::rtc::LiveTranscoding& transcoding) {
  return -ERR_NOT_SUPPORTED;
}

inline int AgoraRteCdnLocalStream::AddBypassCdnUrl(
    const std::string& target_cdn_url, bool transcoding_enabled) {
  return -ERR_NOT_SUPPORTED;
}

inline int AgoraRteCdnLocalStream::RemoveBypassCdnUrl(
    const std::string& target_cdn_url) {
  return -ERR_NOT_SUPPORTED;
}

inline int AgoraRteCdnLocalStream::createRtmpConnection() {
  if (rtmp_conn_) {
    return ERR_OK;
  }
  if (!rtc_service_) {
    return -ERR_NOT_INITIALIZED;
  }

  rtc::RtmpConnectionConfiguration cfg;
  cfg.audioConfig = audio_encoder_config_;
  cfg.videoConfig = video_encoder_config_;

  auto conn = rtc_service_->createRtmpConnection(cfg);
  if (!conn) {
    RTE_LOG_ERROR << "Create rtmp connection failed!";
    return -ERR_FAILED;
  }

  rtmp_conn_ = conn;
  local_user_ = rtmp_conn_->getRtmpLocalUser();
  local_user_->registerRtmpUserObserver(this);
  rtmp_conn_->registerObserver(this);
  return ERR_OK;
}

inline int AgoraRteCdnLocalStream::stopDirectCdnStreamingInternal() {
  if (local_user_) {
    local_user_->unregisteRtmpUserObserver(this);
    local_user_ = nullptr;
  }

  if (rtmp_conn_) {
    rtmp_conn_->disconnect();
    rtmp_conn_->unregisterObserver(this);
    rtmp_conn_ = nullptr;
  }

  streaming_state_ = STATE_PUBLISH_STOPPED;
  return ERR_OK;
}

// IRtmpConnectionObserver
inline void AgoraRteCdnLocalStream::onConnected(
    const rtc::RtmpConnectionInfo& connectionInfo) {
  streaming_state_ = STATE_PUBLISHED;
}

inline void AgoraRteCdnLocalStream::onDisconnected(
    const rtc::RtmpConnectionInfo& connectionInfo) {
  /*stopDirectCdnStreamingInternal();*/
}

inline void AgoraRteCdnLocalStream::onReconnecting(
    const rtc::RtmpConnectionInfo& connectionInfo) {}

inline void AgoraRteCdnLocalStream::onConnectionFailure(
    const rtc::RtmpConnectionInfo& connectionInfo,
    rtc::RTMP_CONNECTION_ERROR errCode) {
  if (streaming_state_ == STATE_PUBLISH_FAILED ||
      streaming_state_ == STATE_PUBLISH_STOPPED) {
    return;
  }

  /*stopDirectCdnStreamingInternal();*/
}

inline void AgoraRteCdnLocalStream::onTransferStatistics(
    uint64_t video_bitrate, uint64_t audio_bitrate, uint64_t video_frame_rate) {
}

inline void AgoraRteCdnLocalStream::FireStreamStateChange(
    MediaType media_type, StreamMediaState old_state,
    StreamMediaState new_state, StreamStateChangedReason reason) {
  auto scene = scene_.lock();
  if (!scene) {
    return;
  }

  StreamInfo info;
  info.user_id = scene->GetLocalUserInfo().user_id;
  info.stream_id = stream_id_;

  scene->OnLocalStreamStateChanged(info, media_type, old_state, new_state,
                                   reason);
}

inline void AgoraRteCdnLocalStream::onAudioTrackPublishSuccess(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack) {
  StreamMediaState old_state = StreamMediaState::kIdle;
  auto it = published_audio_tracks_.find(audioTrack);
  if (it != published_audio_tracks_.end()) {
    old_state = it->second;
    it->second = StreamMediaState::kStreaming;
  }

  FireStreamStateChange(MediaType::kAudio, old_state,
                        StreamMediaState::kStreaming,
                        StreamStateChangedReason::kPublished);
}

inline void AgoraRteCdnLocalStream::onAudioTrackPublicationFailure(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack,
    rtc::PublishAudioError error) {
  StreamMediaState old_state = StreamMediaState::kIdle;
  auto it = published_audio_tracks_.find(audioTrack);
  if (it != published_audio_tracks_.end()) {
    old_state = it->second;
    it->second = StreamMediaState::kIdle;
  }

  FireStreamStateChange(MediaType::kAudio, old_state, StreamMediaState::kIdle,
                        StreamStateChangedReason::kPublished);
}

inline void AgoraRteCdnLocalStream::onVideoTrackPublishSuccess(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack) {
  StreamMediaState old_state = published_video_stream_state_;
  published_video_stream_state_ = StreamMediaState::kStreaming;
  FireStreamStateChange(MediaType::kVideo, old_state,
                        StreamMediaState::kStreaming,
                        StreamStateChangedReason::kPublished);
}

inline void AgoraRteCdnLocalStream::onVideoTrackPublicationFailure(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    rtc::PublishVideoError error) {
  StreamMediaState old_state = published_video_stream_state_;
  published_video_stream_state_ = StreamMediaState::kIdle;
  FireStreamStateChange(MediaType::kVideo, old_state, StreamMediaState::kIdle,
                        StreamStateChangedReason::kPublished);
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCustomAudioTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {
inline AgoraRteCustomAudioTrack::AgoraRteCustomAudioTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  track_impl_ = std::make_shared<AgoraRteAudioTrackImpl>(rtc_service);

  auto media_node_factory = track_impl_->GetMediaNodeFactory();

  audio_frame_sender_ = media_node_factory->createAudioPcmDataSender();

  auto track = rtc_service->createCustomAudioTrack(audio_frame_sender_);

  track_impl_->SetTrack(
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalAudioTrack>(
          track));
  track_impl_->Start();
}

inline int AgoraRteCustomAudioTrack::EnableLocalPlayback() {
  return track_impl_->EnableLocalPlayback();
}

inline SourceType AgoraRteCustomAudioTrack::GetSourceType() {
  return SourceType::kAudio_Custom;
}

inline int AgoraRteCustomAudioTrack::AdjustPlayoutVolume(int volume) {
  return track_impl_->AdjustPlayoutVolume(volume);
}

inline int AgoraRteCustomAudioTrack::AdjustPublishVolume(int volume) {
  return track_impl_->AdjustPublishVolume(volume);
}

inline const std::string& AgoraRteCustomAudioTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

inline int AgoraRteCustomAudioTrack::PushAudioFrame(AudioFrame& frame) {
  return audio_frame_sender_->sendAudioPcmData(
      frame.buffer, 0, frame.samplesPerChannel, frame.bytesPerSample,
      frame.channels, frame.samplesPerSec);
}

inline void AgoraRteCustomAudioTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

inline std::shared_ptr<agora::rtc::ILocalAudioTrack>
AgoraRteCustomAudioTrack::GetRtcAudioTrack() const {
  return track_impl_->GetAudioTrack();
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteCustomVideoTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {
inline AgoraRteCustomVideoTrack::AgoraRteCustomVideoTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  track_impl_ = std::make_shared<AgoraRteVideoTrackImpl>(rtc_service);

  auto media_node_factory = track_impl_->GetMediaNodeFactory();
  video_frame_sender_ = media_node_factory->createVideoFrameSender();

  auto track = rtc_service->createCustomVideoTrack(video_frame_sender_);

  track_impl_->SetTrack(
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalVideoTrack>(
          track));
  track_impl_->Start();
}

inline SourceType AgoraRteCustomVideoTrack::GetSourceType() {
  return SourceType::kVideo_Custom;
}

inline int AgoraRteCustomVideoTrack::PushVideoFrame(
    const ExternalVideoFrame& frame) {
  return video_frame_sender_->sendVideoFrame(frame);
}

inline int AgoraRteCustomVideoTrack::SetPreviewCanvas(
    const VideoCanvas& canvas) {
  return track_impl_->SetPreviewCanvas(canvas);
}

inline void AgoraRteCustomVideoTrack::RegisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->RegisterLocalVideoFrameObserver(observer);
}

inline void AgoraRteCustomVideoTrack::UnregisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->UnregisterLocalVideoFrameObserver(observer);
}

inline int AgoraRteCustomVideoTrack::EnableVideoFilter(
    const std::string& id, bool enable) {
  return track_impl_->GetVideoTrack()->enableVideoFilter(id.c_str(), enable);
}

inline int AgoraRteCustomVideoTrack::SetFilterProperty(
    const std::string& id, const std::string& key,
    const std::string& json_value) {
  return track_impl_->GetVideoTrack()->setFilterProperty(
      id.c_str(), key.c_str(), json_value.c_str());
}

inline std::string AgoraRteCustomVideoTrack::GetFilterProperty(
    const std::string& id, const std::string& key) {
  char buffer[1024];

  if (track_impl_->GetVideoTrack()->getFilterProperty(id.c_str(), key.c_str(),
                                                      buffer, 1024) == ERR_OK) {
    return buffer;
  }

  return "";
}

inline void AgoraRteCustomVideoTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

inline const std::string& AgoraRteCustomVideoTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

inline std::shared_ptr<agora::rtc::ILocalVideoTrack>
AgoraRteCustomVideoTrack::GetRtcVideoTrack() const {
  return track_impl_->GetVideoTrack();
}
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMediaFactory.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteMediaFactory::AgoraRteMediaFactory(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  rtc_services_ = rtc_service;
}

inline std::shared_ptr<IAgoraRteCameraVideoTrack>
AgoraRteMediaFactory::CreateCameraVideoTrack() {
  auto result = std::make_shared<AgoraRteCameraVideoTrack>(rtc_services_);
  result->Init();
  return result;
}

inline std::shared_ptr<IAgoraRteScreenVideoTrack>
AgoraRteMediaFactory::CreateScreenVideoTrack() {
  auto result = std::make_shared<AgoraRteScreenVideoTrack>(rtc_services_);
  result->Init();
  return result;
}

inline std::shared_ptr<IAgoraRteMixedVideoTrack>
AgoraRteMediaFactory::CreateMixedVideoTrack() {
  auto result = std::make_shared<AgoraRteMixedVideoTrack>(rtc_services_);
  result->Init();
  return result;
}

inline std::shared_ptr<IAgoraRteCustomVideoTrack>
AgoraRteMediaFactory::CreateCustomVideoTrack() {
  auto result = std::make_shared<AgoraRteCustomVideoTrack>(rtc_services_);
  result->Init();
  return result;
}

inline std::shared_ptr<IAgoraRteMicrophoneAudioTrack>
AgoraRteMediaFactory::CreateMicrophoneAudioTrack() {
  auto result = std::make_shared<AgoraRteMicrophoneAudioTrack>(rtc_services_);
  result->Init();
  return result;
}

inline std::shared_ptr<IAgoraRteCustomAudioTrack>
AgoraRteMediaFactory::CreateCustomAudioTrack() {
  auto result = std::make_shared<AgoraRteCustomAudioTrack>(rtc_services_);
  result->Init();
  return result;
}

inline std::shared_ptr<IAgoraRteMediaPlayer>
AgoraRteMediaFactory::CreateMediaPlayer() {
  auto result = std::make_shared<AgoraRteMediaPlayer>(rtc_services_);
  return result;
}

inline std::shared_ptr<IAgoraRteStreamingSource>
AgoraRteMediaFactory::CreateStreamingSource() {
  auto result = std::make_shared<AgoraRteStreamingSource>(rtc_services_);
  return result;
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMediaPlayer.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteMediaPlayer::AgoraRteMediaPlayer(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  auto factory = createAgoraMediaComponentFactory();
  auto media_player = factory->createMediaPlayer();

  media_player_ =
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IMediaPlayer>(
          media_player);
  media_player_->initialize(rtc_service.get());

  rtc_service_ = rtc_service;

  auto player = media_player_.get();
  auto video_track =
      static_cast<rtc::IMediaPlayerEx*>(player)->getLocalVideoTrack();
  video_track_ = std::make_shared<AgoraRteWrapperVideoTrack>(
      rtc_service_,
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalVideoTrack>(
          video_track));

  auto audio_track =
      static_cast<rtc::IMediaPlayerEx*>(player)->getLocalAudioTrack();

  audio_track_ = std::make_shared<AgoraRteWrapperAudioTrack>(
      rtc_service_,
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalAudioTrack>(
          audio_track));
}

inline AgoraRteMediaPlayer::~AgoraRteMediaPlayer() {
  // Release track first before we release others
  //
  audio_track_ = nullptr;
  video_track_ = nullptr;

  if (!media_player_obs_.empty()) {
    media_player_->unregisterAudioFrameObserver(
        internal_media_play_audio_ob_.get());
    media_player_->unregisterVideoFrameObserver(
        internal_media_play_video_ob_.get());
    media_player_->unregisterPlayerSourceObserver(
        internal_media_play_src_ob_.get());
  }

  if (play_list_ != nullptr) {
    play_list_->SetCurrFileById(INVALID_RTE_FILE_ID);
    play_list_ = nullptr;
  }
}

inline std::shared_ptr<IAgoraRtePlayList>
AgoraRteMediaPlayer::CreatePlayList() {
  return std::make_shared<AgoraRtePlayList>();
}

inline int AgoraRteMediaPlayer::Open(const std::string& url,
                                         int64_t start_pos) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);

  if (play_list_ != nullptr) {  // previous list have not closed
    return -agora::ERR_INVALID_STATE;
  }

  current_file_.Reset();
  current_file_.file_id = 1;
  current_file_.file_url = url;
  current_file_.index = 0;
  first_file_id_ = current_file_.file_id;

  auto_play_for_open_ = false;
  return media_player_->open(url.c_str(), start_pos);
}

inline int AgoraRteMediaPlayer::Open(
    std::shared_ptr<IAgoraRtePlayList> in_play_list, int64_t start_pos) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);

  if (play_list_ != nullptr) {  // previous list have not closed
    return -agora::ERR_INVALID_STATE;
  }
  if (in_play_list == nullptr) {
    return -agora::ERR_INVALID_ARGUMENT;
  }
  if (in_play_list->GetFileCount() <= 0) {
    return -agora::ERR_INVALID_ARGUMENT;
  }

  play_list_ = std::static_pointer_cast<AgoraRtePlayList>(in_play_list);

  current_file_.Reset();
  play_list_->GetFirstFileInfo(current_file_);
  play_list_->SetCurrFileById(current_file_.file_id);
  assert(current_file_.IsValid());
  first_file_id_ = current_file_.file_id;
  list_played_count_ = 0;

  auto_play_for_open_ = false;
  return media_player_->open(current_file_.file_url.c_str(), start_pos);
}

inline int AgoraRteMediaPlayer::Play() { return media_player_->play(); }

inline int AgoraRteMediaPlayer::Pause() { return media_player_->pause(); }

inline int AgoraRteMediaPlayer::Resume() { return media_player_->resume(); }

inline int AgoraRteMediaPlayer::Stop() {
  int ret = media_player_->stop();

  {
    std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
    if (play_list_ != nullptr) {
      play_list_->SetCurrFileById(INVALID_RTE_FILE_ID);
      play_list_ = nullptr;
    }
  }

  return ret;
}

inline int AgoraRteMediaPlayer::Seek(int64_t pos) {
  if (play_list_ != nullptr) {
    RteFileInfo curr_file_info;
    play_list_->GetCurrentFileInfo(curr_file_info);
    if ((curr_file_info.duration > 0) && (pos >= curr_file_info.duration)) {
      RTE_LOG_ERROR << "<AgoraRteMediaPlayer.Seek> pos is more than duration";
      return -agora::ERR_INVALID_ARGUMENT;
    }
  }

  return media_player_->seek(pos);
}

inline int AgoraRteMediaPlayer::SeekToPrev(int64_t pos) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  if (play_list_ == nullptr) {  // It don't support for only one file
    return -agora::ERR_INVALID_STATE;
  }
  if (play_list_->CurrentIsFirstFile()) {
    RTE_LOG_ERROR
        << "<AgoraRteMediaPlayer.SeekToPrev> current already is first file";
    return -agora::ERR_INVALID_STATE;
  }
  RteFileInfoSharePtr prev_file_ptr = play_list_->FindPrevFile(false);
  assert(prev_file_ptr != nullptr);
  if ((prev_file_ptr->duration > 0) && (pos >= prev_file_ptr->duration)) {
    RTE_LOG_ERROR
        << "<AgoraRteMediaPlayer.SeekToPrev> pos is more than duration";
    return -agora::ERR_INVALID_ARGUMENT;
  }

  // stop current file
  int ret = media_player_->stop();
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to stop media player, error : " << ret;
    return ret;
  }

  // switch to previous file
  ret = play_list_->MoveCurrentToPrev(false);

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to move to previous file, error : " << ret;
    return ret;
  }

  current_file_.Reset();
  play_list_->GetCurrentFileInfo(current_file_);
  assert(current_file_.IsValid());

  // start play previous file
  auto_play_for_open_ = true;
  ret = media_player_->open(current_file_.file_url.c_str(), pos);
  return ret;
}

inline int AgoraRteMediaPlayer::SeekToNext(int64_t pos) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  if (play_list_ == nullptr) {
    return -agora::ERR_INVALID_STATE;
  }
  if (play_list_->CurrentIsLastFile()) {
    RTE_LOG_ERROR
        << "<AgoraRteMediaPlayer.SeekToNext> current already is last file";
    return -agora::ERR_INVALID_STATE;
  }
  RteFileInfoSharePtr next_file_ptr = play_list_->FindNextFile(false);
  assert(next_file_ptr != nullptr);
  if ((next_file_ptr->duration > 0) && (pos >= next_file_ptr->duration)) {
    RTE_LOG_ERROR
        << "<AgoraRteMediaPlayer.SeekToNext> pos is more than duration";
    return -agora::ERR_INVALID_ARGUMENT;
  }

  // stop current file
  int ret = media_player_->stop();
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to stop media player, error : " << ret;
    return ret;
  }

  // switch to next file
  ret = play_list_->MoveCurrentToNext(false);

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to move to next file, error : " << ret;
    return ret;
  }

  current_file_.Reset();
  play_list_->GetCurrentFileInfo(current_file_);
  assert(current_file_.IsValid());

  // start play next file
  auto_play_for_open_ = true;
  ret = media_player_->open(current_file_.file_url.c_str(), pos);
  return ret;
}

inline int AgoraRteMediaPlayer::SeekToFile(int32_t file_id, int64_t pos) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  if (play_list_ == nullptr) {
    return -agora::ERR_INVALID_STATE;
  }

  if (file_id == current_file_.file_id) {
    // Seek in current file
    return media_player_->seek(pos);
  }

  // stop current file
  media_player_->stop();

  // switch to new file
  int ret = play_list_->SetCurrFileById(file_id);
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "<AgoraRteMediaPlayer.SeekToFile> failed, file_id= "
                  << file_id << ", pos= " << pos;
    return -agora::ERR_INVALID_ARGUMENT;
  }
  current_file_.Reset();
  play_list_->GetCurrentFileInfo(current_file_);
  assert(current_file_.IsValid());

  // start play new file
  auto_play_for_open_ = true;
  ret = media_player_->open(current_file_.file_url.c_str(), pos);
  return ret;
}

inline int AgoraRteMediaPlayer::ChangePlaybackSpeed(
    MEDIA_PLAYER_PLAYBACK_SPEED speed) {
  return media_player_->changePlaybackSpeed(speed);
}

inline int AgoraRteMediaPlayer::AdjustPlayoutVolume(int volume) {
  return media_player_->adjustPlayoutVolume(volume);
}

inline int AgoraRteMediaPlayer::Mute(bool mute) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  setting_mute_ = mute;
  media_player_->mute(mute);  // don't care the result
  return agora::ERR_OK;
}

inline int AgoraRteMediaPlayer::SelectAudioTrack(int index) {
  return media_player_->selectAudioTrack(index);
}

inline int AgoraRteMediaPlayer::SetLoopCount(int loop_count) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  setting_loop_count_ = loop_count;
  list_played_count_ = 0;

  if (play_list_ == nullptr) {  // Open only one file
    return media_player_->setLoopCount(loop_count);
  }

  return agora::ERR_OK;
}

inline int AgoraRteMediaPlayer::MuteAudio(bool audio_mute) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  return media_player_->muteAudio(audio_mute);
}

inline bool AgoraRteMediaPlayer::IsAudioMuted() {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  bool audio_muted = media_player_->isAudioMuted();
  return audio_muted;
}

inline int AgoraRteMediaPlayer::MuteVideo(bool video_mute) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  return media_player_->muteVideo(video_mute);
}

inline bool AgoraRteMediaPlayer::IsVideoMuted() {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  bool video_muted = media_player_->isVideoMuted();
  return video_muted;
}

inline int AgoraRteMediaPlayer::SetView(View view) {
  return media_player_->setView(view);
}

inline int AgoraRteMediaPlayer::SetRenderMode(RENDER_MODE_TYPE mode) {
  return media_player_->setRenderMode(mode);
}

inline int AgoraRteMediaPlayer::SetPlayerOption(const std::string& key,
                                                    int value) {
  return media_player_->setPlayerOption(key.c_str(), value);
}

inline int AgoraRteMediaPlayer::SetPlayerOptionString(
    const std::string& key, const std::string& value) {
  return media_player_->setPlayerOption(key.c_str(), value.c_str());
}

inline int AgoraRteMediaPlayer::GetPlayerStatus(
    RtePlayerStatus& out_playing_status) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);

  out_playing_status.curr_file_id = current_file_.file_id;
  out_playing_status.curr_file_url = current_file_.file_url;
  out_playing_status.curr_file_index = current_file_.index;
  out_playing_status.curr_file_duration = current_file_.duration;
  out_playing_status.curr_file_begin_time = current_file_.begin_time;
  out_playing_status.player_state = media_player_->getState();
  media_player_->getPlayPosition(out_playing_status.progress_pos);
  return agora::ERR_OK;
}

inline agora::rte::MEDIA_PLAYER_STATE AgoraRteMediaPlayer::GetState() {
  return media_player_->getState();
}

inline int AgoraRteMediaPlayer::GetDuration(int64_t& duration) {
  return media_player_->getDuration(duration);
}

inline int AgoraRteMediaPlayer::GetPlayPosition(int64_t& pos) {
  return media_player_->getPlayPosition(pos);
}

inline int AgoraRteMediaPlayer::GetPlayoutVolume(int& volume) {
  return media_player_->getPlayoutVolume(volume);
}

inline int AgoraRteMediaPlayer::GetStreamCount(int64_t& count) {
  return media_player_->getStreamCount(count);
}

inline int AgoraRteMediaPlayer::GetStreamInfo(int64_t index,
                                                  PlayerStreamInfo& info) {
  return media_player_->getStreamInfo(index, &info);
}

inline bool AgoraRteMediaPlayer::IsMuted() {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  return setting_mute_;
}

inline const char* AgoraRteMediaPlayer::GetPlayerVersion() {
  return media_player_->getPlayerSdkVersion();
}

inline int AgoraRteMediaPlayer::SetStreamId(const std::string& stream_id) {
  stream_id_ = stream_id;
  return ERR_OK;
}

inline int AgoraRteMediaPlayer::GetStreamId(std::string& stream_id) const {
  stream_id = stream_id_;
  return ERR_OK;
}

inline int AgoraRteMediaPlayer::RegisterMediaPlayerObserver(
    std::shared_ptr<IAgoraRteMediaPlayerObserver> observer) {
  {
    std::lock_guard<std::mutex> _(media_player_lock_);
    if (!internal_media_play_src_ob_) {
      internal_media_play_src_ob_ =
          RtcCallbackWrapper<AgoraRteMediaPlayerSrcObserver>::Create(
              shared_from_this());
      internal_media_play_audio_ob_ =
          RtcCallbackWrapper<AgoraRteMediaPlayerAudioObserver>::Create(
              shared_from_this());
      internal_media_play_video_ob_ =
          RtcCallbackWrapper<AgoraRteMediaPlayerVideoObserver>::Create(
              shared_from_this());
    }
  }

  UnregisterMediaPlayerObserver(observer);

  {
    std::lock_guard<std::mutex> _(media_player_lock_);

    if (media_player_obs_.empty()) {
      media_player_->registerAudioFrameObserver(
          internal_media_play_audio_ob_.get());
      media_player_->registerVideoFrameObserver(
          internal_media_play_video_ob_.get());
      media_player_->registerPlayerSourceObserver(
          internal_media_play_src_ob_.get());
    }

    media_player_obs_.push_back(observer);
  }

  return ERR_OK;
}

inline int AgoraRteMediaPlayer::UnregisterMediaPlayerObserver(
    std::shared_ptr<IAgoraRteMediaPlayerObserver> observer) {
  AgoraRteUtils::UnregisterSharedPtrFromContainer(media_player_lock_,
                                                  media_player_obs_, observer);

  {
    std::lock_guard<std::mutex> _(media_player_lock_);

    if (media_player_obs_.empty()) {
      media_player_->unregisterAudioFrameObserver(
          internal_media_play_audio_ob_.get());
      media_player_->unregisterVideoFrameObserver(
          internal_media_play_video_ob_.get());
      media_player_->unregisterPlayerSourceObserver(
          internal_media_play_src_ob_.get());
    }
  }
  return ERR_OK;
}

inline std::shared_ptr<IAgoraRteAudioTrack>
AgoraRteMediaPlayer::GetAudioTrack() {
  return audio_track_;
}

inline std::shared_ptr<IAgoraRteVideoTrack>
AgoraRteMediaPlayer::GetVideoTrack() {
  return video_track_;
}

inline void AgoraRteMediaPlayer::GetCurrFileInfo(
    RteFileInfo& out_current_file) {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  current_file_.CloneTo(out_current_file);
}

inline int AgoraRteMediaPlayer::ProcessOpened() {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  if (play_list_ == nullptr) {  // Only play one file
    return agora::ERR_OK;
  }

  media_player_->mute(setting_mute_);

  if ((!auto_play_for_open_) && (play_list_->CurrentIsFirstFile())) {
    return agora::ERR_OK;
  }

  // Auto play media file
  int ret = media_player_->play();
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "<AgoraRteMediaPlayer.ProcessOpened> fail to play()="
                  << ret;
  }
  return ret;
}

inline int AgoraRteMediaPlayer::ProcessOneMediaCompleted() {
  std::lock_guard<std::recursive_mutex> _(file_manager_lock_);
  int ret;

  if (play_list_ == nullptr) {  // Only play one file
    return agora::ERR_OK;
  }

  // Stop current playing file
  ret = media_player_->stop();

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to stop media player, error : " << ret;
    return ret;
  }

  if (!play_list_->CurrentIsLastFile()) {
    // switch to next file
    RTE_LOG_ERROR << "<AgoraRteMediaPlayer.ProcessOneMediaCompleted> switch to "
                     "next file...";
    ret = play_list_->MoveCurrentToNext(false);
    if (ret != agora::ERR_OK) {
      RTE_LOG_ERROR
          << "<AgoraRteMediaPlayer.ProcessOneMediaCompleted> failed, ret= "
          << ret;
      return -agora::ERR_INVALID_STATE;
    }

    // start play next file
    play_list_->GetCurrentFileInfo(current_file_);
    auto_play_for_open_ = true;
    ret = media_player_->open(current_file_.file_url.c_str(), 0);
    return ret;
  }

  list_played_count_++;
  if (list_played_count_ >=
      setting_loop_count_)  // all file loops are finished finished
  {
    play_list_->SetCurrFileById(INVALID_RTE_FILE_ID);  // reset current file
    current_file_.Reset();
    RTE_LOG_ERROR
        << "<AgoraRteMediaPlayer.ProcessOneMediaCompleted> all loop finished";
    return -agora::ERR_CANCELED;
  } else  // Repeat play first file
  {
    RTE_LOG_ERROR << "<AgoraRteMediaPlayer.ProcessOneMediaCompleted> switch to "
                     "first file...";

    // Switch to first file
    play_list_->GetFirstFileInfo(current_file_);
    play_list_->SetCurrFileById(current_file_.file_id);

    // start play first file
    auto_play_for_open_ = true;
    ret = media_player_->open(current_file_.file_url.c_str(), 0);
  }

  return ret;
}

inline void AgoraRteMediaPlayerSrcObserver::onPlayerSourceStateChanged(
    media::base::MEDIA_PLAYER_STATE state, media::base::MEDIA_PLAYER_ERROR ec) {
  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, state, ec](const auto& ob) {
          ob->OnPlayerStateChanged(current_file, state, ec);
        });

    if (media::base::PLAYER_STATE_OPEN_COMPLETED == state) {
      palyer->ProcessOpened();
    } else if (media::base::PLAYER_STATE_PLAYBACK_COMPLETED == state ||
               media::base::PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED ==
                   state) {
      int ret = palyer->ProcessOneMediaCompleted();
      if (-agora::ERR_CANCELED == ret) {  // All file and loop are finished
        AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
            palyer->media_player_lock_, palyer->media_player_obs_,
            [ec](const auto& ob) { ob->OnAllMediasCompleted(ec); });
      }
    }
  }
}

inline void AgoraRteMediaPlayerSrcObserver::onPositionChanged(
    int64_t position) {
  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, position](const auto& ob) {
          ob->OnPositionChanged(current_file, position);
        });
  }
}

inline void AgoraRteMediaPlayerSrcObserver::onPlayerEvent(
    media::base::MEDIA_PLAYER_EVENT event) {
  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, event](const auto& ob) {
          ob->OnPlayerEvent(current_file, event);
        });
  }
}

inline void AgoraRteMediaPlayerSrcObserver::onMetaData(const void* data,
                                                           int length) {
  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, data, length](const auto& ob) {
          ob->OnMetadata(current_file,
                         MEDIA_PLAYER_METADATA_TYPE::PLAYER_METADATA_TYPE_SEI,
                         static_cast<const uint8_t*>(data), length);
        });
  }
}

inline void AgoraRteMediaPlayerSrcObserver::onCompleted() {
  // IAgoraRteMediaPlayerObserver doesn't need this callback
  //
}

inline void AgoraRteMediaPlayerSrcObserver::onPlayBufferUpdated(
    int64_t playCachedBuffer) {
  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, playCachedBuffer](const auto& ob) {
          ob->OnPlayerBufferUpdated(current_file, playCachedBuffer);
        });
  }
}

inline void AgoraRteMediaPlayerAudioObserver::onFrame(
    AudioPcmFrame* audio_frame) {
  // AGO_LOG("<AudioObserver.onFrame> timestamp=%d\n",
  //        static_cast<int>(audio_frame->capture_timestamp));

  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, &audio_frame](const auto& ob) {
          ob->OnAudioFrame(current_file, *audio_frame);
        });
  }
}

inline void AgoraRteMediaPlayerVideoObserver::onFrame(
    const VideoFrame* video_frame) {
  auto palyer = media_player_.lock();
  if (palyer) {
    RteFileInfo current_file;
    palyer->GetCurrFileInfo(current_file);
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteMediaPlayerObserver>(
        palyer->media_player_lock_, palyer->media_player_obs_,
        [&current_file, &video_frame](const auto& ob) {
          ob->OnVideoFrame(current_file, *video_frame);
        });
  }
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMicrophoneAudioTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

namespace agora {
namespace rte {

inline AgoraRteMicrophoneAudioTrack::AgoraRteMicrophoneAudioTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  track_impl_ = std::make_shared<AgoraRteAudioTrackImpl>(rtc_service);
  auto track = rtc_service->createLocalAudioTrack();
  track_impl_->SetTrack(
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalAudioTrack>(
          track));
}

inline SourceType AgoraRteMicrophoneAudioTrack::GetSourceType() {
  return SourceType::kAudio_Microphone;
}

inline int AgoraRteMicrophoneAudioTrack::AdjustPublishVolume(int volume) {
  return track_impl_->AdjustPublishVolume(volume);
}

inline int AgoraRteMicrophoneAudioTrack::AdjustPlayoutVolume(int volume) {
  return track_impl_->AdjustPlayoutVolume(volume);
}

inline const std::string&
AgoraRteMicrophoneAudioTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

inline int AgoraRteMicrophoneAudioTrack::StartRecording() {
  return track_impl_->Start();
}

inline void AgoraRteMicrophoneAudioTrack::StopRecording() {
  track_impl_->Stop();
}

inline int AgoraRteMicrophoneAudioTrack::EnableEarMonitor(
    bool enable, int includeAudioFilter) {
  return track_impl_->EnableEarMonitor(enable, includeAudioFilter);
}

inline void AgoraRteMicrophoneAudioTrack::SetStreamId(
    const std::string& stream_id) {
  return track_impl_->SetStreamId(stream_id);
}

inline std::shared_ptr<agora::rtc::ILocalAudioTrack>
AgoraRteMicrophoneAudioTrack::GetRtcAudioTrack() const {
  return track_impl_->GetAudioTrack();
}

inline int AgoraRteMicrophoneAudioTrack::EnableLocalPlayback() {
  return track_impl_->EnableLocalPlayback();
}

inline int AgoraRteMicrophoneAudioTrack::SetAudioReverbPreset(
    AUDIO_REVERB_PRESET reverb_preset) {
  return -ERR_FAILED;
}

inline int AgoraRteMicrophoneAudioTrack::SetVoiceChangerPreset(
    VOICE_CHANGER_PRESET voice_changer_preset) {
  return -ERR_FAILED;
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteMixedVideoTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include <list>


namespace agora {
namespace rte {

inline AgoraRteMixedVideoTrack::AgoraRteMixedVideoTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  track_impl_ = std::make_shared<AgoraRteVideoTrackImpl>(rtc_service);

  auto media_node_factory = track_impl_->GetMediaNodeFactory();
  video_mixer_ = media_node_factory->createVideoMixer();

  auto track = rtc_service->createMixedVideoTrack(video_mixer_);

  track_impl_->SetTrack(
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalVideoTrack>(
          track));
  track_impl_->Start();
}

inline AgoraRteMixedVideoTrack::~AgoraRteMixedVideoTrack() {
  video_mixer_->clearLayout();
}

inline int AgoraRteMixedVideoTrack::AddImage(
    const std::string& id, const rtc::MixerLayoutConfig& layout) {
  if (layout.image_path == nullptr) {
    return -ERR_INVALID_ARGUMENT;
  }

  rtc::ImageType imageType;
  const std::string fileName = layout.image_path;
  std::string fileSuffix = fileName.substr(fileName.find_last_of('.'));
  std::transform(fileSuffix.begin(), fileSuffix.end(), fileSuffix.begin(),
                 ::tolower);

  if (fileSuffix == ".jpg" || fileSuffix == ".jpeg") {
    imageType = rtc::kJpeg;
  } else if (fileSuffix == ".png" || fileSuffix == ".flv" ||
             fileSuffix == ".mp4") {
    imageType = rtc::kPng;
  } else if (fileSuffix == ".gif") {
    imageType = rtc::kGif;
  } else {
    return -ERR_NOT_SUPPORTED;
  }

  return video_mixer_->addImageSource(id.c_str(), layout, imageType);
}

inline int AgoraRteMixedVideoTrack::RemoveImage(const std::string& id) {
  if (id.empty()) {
    return -ERR_INVALID_ARGUMENT;
  }

  return video_mixer_->delImageSource(id.c_str());
}

inline int AgoraRteMixedVideoTrack::SetLayout(
    const LayoutConfigs layoutConfigs) {
  int ret = ERR_OK;

  video_mixer_->clearLayout();

  auto background = layoutConfigs.canvas;
  ret = video_mixer_->setBackground(
      background.width, background.height,
      fps_from_encoder_ ? fps_from_encoder_ : background.fps,
      background.image_file_path.c_str());

  if (ret != ERR_OK) {
    video_mixer_->clearLayout();
    return ret;
  }

  auto layouts = layoutConfigs.layouts;

  ret = ERR_OK;
  std::list<std::string> image_id_list;
  int layouts_size = static_cast<int>(layouts.size());
  for (int i = 0; i < layouts_size; i++) {
    auto layout = layouts[i];

    rtc::MixerLayoutConfig mixerLayoutConfig;
    mixerLayoutConfig.alpha = layout.alpha;
    mixerLayoutConfig.x = layout.x;
    mixerLayoutConfig.y = layout.y;
    mixerLayoutConfig.width = layout.width;
    mixerLayoutConfig.height = layout.height;
    mixerLayoutConfig.zOrder = layout.zOrder;
    mixerLayoutConfig.mirror = layout.mirror;
    mixerLayoutConfig.image_path = layout.image_path.c_str();

    if (layout.layout_type == LayoutType::Track) {
      ret = video_mixer_->setStreamLayout(layout.id.c_str(), mixerLayoutConfig);
    } else if (layout.layout_type == LayoutType::Image) {
      image_id_list.push_back(layout.id);
      ret = AddImage(layout.id, mixerLayoutConfig);
    }

    if (ret != ERR_OK) {
      break;
    }
  }

  if (ret == ERR_OK) {
    ret = video_mixer_->refresh();
  } else {
    video_mixer_->clearLayout();
    return ret;
  }

  layout_configs_ = layoutConfigs;

  return ret;
}

inline int AgoraRteMixedVideoTrack::GetLayout(
    LayoutConfigs& layoutConfigs) {
  if (layout_configs_.has_value()) {
    layoutConfigs = layout_configs_.value();
    return ERR_OK;
  }
  return ERR_NOT_READY;
}

inline int AgoraRteMixedVideoTrack::AddTrack(
    std::shared_ptr<IAgoraRteVideoTrack> video_track) {
  auto track = AgoraRteUtils::CastToImpl(video_track);

  agora_refptr<rtc::ILocalVideoTrack> agora_track(
      track->GetRtcVideoTrack().get());
  return video_mixer_->addVideoTrack(track->GetTrackId().c_str(), agora_track);
}

inline int AgoraRteMixedVideoTrack::RemoveTrack(
    std::shared_ptr<IAgoraRteVideoTrack> video_track) {
  auto track = AgoraRteUtils::CastToImpl(video_track);

  agora_refptr<rtc::ILocalVideoTrack> agora_track(
      track->GetRtcVideoTrack().get());
  return video_mixer_->removeVideoTrack(track->GetTrackId().c_str(),
                                        agora_track);
}

inline int AgoraRteMixedVideoTrack::AddMediaPlayer(
    std::shared_ptr<IAgoraRteMediaPlayer> mediaPlayer) {
  auto video_track = std::static_pointer_cast<AgoraRteMediaPlayer>(mediaPlayer)
                         ->GetVideoTrack();
  std::string id;

  // GetStreamId from mediaPlayer is different from rtc's GetStreamId
  //
  mediaPlayer->GetStreamId(id);

  if (!id.empty() && video_track) {
    agora_refptr<rtc::ILocalVideoTrack> track(
        AgoraRteUtils::CastToImpl(video_track)->GetRtcVideoTrack().get());
    return video_mixer_->addVideoTrack(id.c_str(), track);
  }

  return -ERR_FAILED;
}

inline int AgoraRteMixedVideoTrack::RemoveMediaPlayer(
    std::shared_ptr<IAgoraRteMediaPlayer> mediaPlayer) {
  auto video_track = std::static_pointer_cast<AgoraRteMediaPlayer>(mediaPlayer)
                         ->GetVideoTrack();
  std::string id;
  // GetStreamId from mediaPlayer is different from rtc's GetStreamId
  //
  mediaPlayer->GetStreamId(id);

  if (!id.empty() && video_track) {
    agora_refptr<rtc::ILocalVideoTrack> track(
        AgoraRteUtils::CastToImpl(video_track)->GetRtcVideoTrack().get());
    return video_mixer_->removeVideoTrack(id.c_str(), track);
  }

  return -ERR_FAILED;
}

inline int AgoraRteMixedVideoTrack::SetPreviewCanvas(
    const VideoCanvas& canvas) {
  return track_impl_->SetPreviewCanvas(canvas);
}

inline SourceType AgoraRteMixedVideoTrack::GetSourceType() {
  return SourceType::kVideo_Mix;
}

inline void AgoraRteMixedVideoTrack::RegisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->RegisterLocalVideoFrameObserver(observer);
}

inline void AgoraRteMixedVideoTrack::UnregisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->UnregisterLocalVideoFrameObserver(observer);
}

inline int AgoraRteMixedVideoTrack::EnableVideoFilter(const std::string& id,
                                                          bool enable) {
  return track_impl_->GetVideoTrack()->enableVideoFilter(id.c_str(), enable);
}

inline int AgoraRteMixedVideoTrack::SetFilterProperty(
    const std::string& id, const std::string& key,
    const std::string& json_value) {
  return track_impl_->GetVideoTrack()->setFilterProperty(
      id.c_str(), key.c_str(), json_value.c_str());
}

inline std::string AgoraRteMixedVideoTrack::GetFilterProperty(
    const std::string& id, const std::string& key) {
  char buffer[1024];

  if (track_impl_->GetVideoTrack()->getFilterProperty(id.c_str(), key.c_str(),
                                                      buffer, 1024) == ERR_OK) {
    return buffer;
  }

  return "";
}

inline void AgoraRteMixedVideoTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

inline const std::string& AgoraRteMixedVideoTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

inline std::shared_ptr<agora::rtc::ILocalVideoTrack>
AgoraRteMixedVideoTrack::GetRtcVideoTrack() const {
  return track_impl_->GetVideoTrack();
}

inline int AgoraRteMixedVideoTrack::BeforeVideoEncodingChanged(
    const VideoEncoderConfiguration& config) {
  if (layout_configs_.has_value()) {
    auto& config = layout_configs_.value();
    fps_from_encoder_ = config.canvas.fps;
    int result = video_mixer_->setBackground(
        config.canvas.width, config.canvas.height, fps_from_encoder_,
        config.canvas.image_file_path.c_str());
    if (result == ERR_OK) {
      result = video_mixer_->refresh();
    }
    return result;
  }

  return ERR_OK;
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRtePlayList.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "string.h"

namespace agora {
namespace rte {

template <typename T>
constexpr auto ERR_CODE(T err) {
  return -1 * static_cast<int>(err);
}

/////////////////////////////////////////////////////////////////////////////
///////////////// Methods Implementation of AgoraRtePlayList ////////////
//////////////////////////////////////////////////////////////////////////////
inline AgoraRtePlayList::AgoraRtePlayList() {}

inline AgoraRtePlayList::~AgoraRtePlayList() {
  streaming_source_.reset();
  file_list_.clear();
}

//////////////////////////////////////////////////////////////////////////
///////////////// Override Methods of IAgoraRtePlayList //////////////////
///////////////////////////////////////////////////////////////////////////

inline int AgoraRtePlayList::ClearFileList() {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if (current_file_ != nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.ClearFileList> list is in use";
    return ERR_CODE(agora::ERR_ALREADY_IN_USE);
  }

  file_list_.clear();
  total_duration_ = 0;
  current_file_ = nullptr;
  return ERR_CODE(agora::ERR_OK);
}

inline int32_t AgoraRtePlayList::GetFileCount() {
  std::lock_guard<std::recursive_mutex> _(list_lock_);
  int32_t file_count = static_cast<int32_t>(file_list_.size());
  return file_count;
}

inline int64_t AgoraRtePlayList::GetTotalDuration() {
  std::lock_guard<std::recursive_mutex> _(list_lock_);
  return total_duration_;
}

inline int AgoraRtePlayList::GetFileInfoById(int32_t file_id,
                                                 RteFileInfo& out_file_info) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);
  RteFileInfoSharePtr found_file = FindFileById(file_id);
  if (found_file == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.GetFileInfoById> file_id=" << file_id
                  << ", NOT found";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  out_file_info.file_id = found_file->file_id;
  out_file_info.file_url = found_file->file_url;
  out_file_info.duration = found_file->duration;
  out_file_info.index = found_file->index;
  out_file_info.begin_time = found_file->begin_time;
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::GetFileInfoByIndex(
    int32_t file_index, RteFileInfo& out_file_info) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);
  RteFileInfoSharePtr found_file = FindFileByIndex(file_index);
  if (found_file == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.GetFileInfoByIndex> file_index="
                  << file_index << ", NOT found";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  out_file_info.file_id = found_file->file_id;
  out_file_info.file_url = found_file->file_url;
  out_file_info.duration = found_file->duration;
  out_file_info.index = found_file->index;
  out_file_info.begin_time = found_file->begin_time;
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::GetFirstFileInfo(
    RteFileInfo& first_file_info) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if (file_list_.empty()) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.GetFirstFileInfo> no files";
    return ERR_CODE(agora::ERR_INVALID_STATE);
  }

  RteFileInfoSharePtr front = file_list_.front();
  front->CloneTo(first_file_info);
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::GetLastFileInfo(RteFileInfo& last_file_info) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if (file_list_.empty()) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.GetFirstFileInfo> no files";
    return ERR_CODE(agora::ERR_INVALID_STATE);
  }

  RteFileInfoSharePtr back = file_list_.back();
  back->CloneTo(last_file_info);
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::GetFileList(
    std::vector<RteFileInfo>& out_file_list) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  std::list<RteFileInfoSharePtr>::iterator it;
  out_file_list.clear();
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    out_file_list.push_back(*file_info);
  }

  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::InsertFile(const char* file_url,
                                            int32_t insert_index,
                                            RteFileInfo& out_file_info) {
  if ((file_url == nullptr) || (strlen(file_url) <= 0)) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.InsertFile> file_url invalid";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  media::base::PlayerStreamInfo video_info;
  media::base::PlayerStreamInfo audio_info;
  int ret = ParseMediaInfo(file_url, video_info, audio_info);
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR
        << "<AgoraRtePlayList.InsertFile> fail to parse media info, file_url="
        << file_url;
    return ERR_CODE(agora::ERR_LOAD_MEDIA_ENGINE);
  }

  {
    std::lock_guard<std::recursive_mutex> _(list_lock_);

    int32_t file_count = static_cast<int32_t>(file_list_.size());
    RteFileInfoSharePtr file_info = std::make_shared<RteFileInfo>();
    file_info->file_id = GetNextFileId();
    file_info->file_url = file_url;
    file_info->duration = (video_info.duration > audio_info.duration)
                              ? video_info.duration
                              : audio_info.duration;
    file_info->video_width = video_info.videoWidth;
    file_info->video_height = video_info.videoHeight;
    file_info->frame_rate = video_info.videoFrameRate;

    if (insert_index <= 0) {
      // insert into front of list
      file_list_.push_front(file_info);
    } else if (insert_index >= file_count) {
      // append into tail of list
      file_list_.push_back(file_info);
    } else {
      // find position and insert file info
      std::list<RteFileInfoSharePtr>::iterator it = file_list_.begin();
      int32_t i = 0;
      while (it != file_list_.end()) {
        if (i == insert_index) {
          file_list_.insert(it, file_info);
          break;
        }

        i++;
        it++;
      }
    }

    // Update begin_time & file index & total duration
    UpdateFileList(total_duration_);

    out_file_info.file_id = file_info->file_id;
    out_file_info.file_url = file_info->file_url;
    out_file_info.duration = file_info->duration;
    out_file_info.video_width = file_info->video_width;
    out_file_info.video_height = file_info->video_height;
    out_file_info.frame_rate = file_info->frame_rate;
    out_file_info.index = file_info->index;
    out_file_info.begin_time = file_info->begin_time;
  }

  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::AppendFile(const char* file_url,
                                            RteFileInfo& out_file_info) {
  if ((file_url == nullptr) || (strlen(file_url) <= 0)) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.AppendFile> file_url invalid";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  media::base::PlayerStreamInfo video_info;
  media::base::PlayerStreamInfo audio_info;
  int ret = ParseMediaInfo(file_url, video_info, audio_info);
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR
        << "<AgoraRtePlayList.AppendFile> fail to parse duration, file_url="
        << file_url;
    return ERR_CODE(agora::ERR_LOAD_MEDIA_ENGINE);
  }

  {
    std::lock_guard<std::recursive_mutex> _(list_lock_);
    RteFileInfoSharePtr file_info = std::make_shared<RteFileInfo>();
    file_info->file_id = GetNextFileId();
    file_info->file_url = file_url;
    file_info->duration = (video_info.duration > audio_info.duration)
                              ? video_info.duration
                              : audio_info.duration;
    file_info->video_width = video_info.videoWidth;
    file_info->video_height = video_info.videoHeight;
    file_info->frame_rate = video_info.videoFrameRate;

    // append into tail of list
    file_list_.push_back(file_info);

    // Update begin_time & file index & total duration
    UpdateFileList(total_duration_);

    out_file_info.file_id = file_info->file_id;
    out_file_info.file_url = file_info->file_url;
    out_file_info.duration = file_info->duration;
    out_file_info.video_width = file_info->video_width;
    out_file_info.video_height = file_info->video_height;
    out_file_info.frame_rate = file_info->frame_rate;
    out_file_info.index = file_info->index;
    out_file_info.begin_time = file_info->begin_time;
  }

  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::RemoveFileById(int32_t remove_file_id) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  RteFileInfoSharePtr found_file = FindFileById(remove_file_id);
  if (found_file == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileById> file_id="
                  << remove_file_id << ", NOT found";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  // Removing file is current playing file, close current file firstly
  if ((current_file_ != nullptr) &&
      (current_file_->file_id == found_file->file_id)) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileById> file_id="
                  << remove_file_id << ", is playing";
    return ERR_CODE(agora::ERR_ALREADY_IN_USE);
  }

  file_list_.remove(found_file);

  // Update begin_time & file index & total duration
  UpdateFileList(total_duration_);

  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::RemoveFileByIndex(int32_t remove_file_index) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  int32_t file_count = static_cast<int32_t>(file_list_.size());
  if (remove_file_index < 0 || remove_file_index >= file_count) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileByIndex> index="
                  << remove_file_index << ", Invalid";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  RteFileInfoSharePtr found_file = FindFileByIndex(remove_file_index);
  if (found_file == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileByIndex> index="
                  << remove_file_index << ", NOT found";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  // Removing file is current playing file, close current file firstly
  if ((current_file_ != nullptr) &&
      (current_file_->file_id == found_file->file_id)) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileByIndex> index="
                  << remove_file_index << ", is playing";
    return ERR_CODE(agora::ERR_ALREADY_IN_USE);
  }

  file_list_.remove(found_file);

  // Update begin_time & file index & total duration
  UpdateFileList(total_duration_);

  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::RemoveFileByUrl(const char* remove_file_url) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if ((remove_file_url == nullptr) || (strlen(remove_file_url) <= 0)) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileByUrl> url Invalid";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  std::vector<RteFileInfoSharePtr> found_list;
  FindFileByUrl(remove_file_url, found_list);
  if (found_list.empty()) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileByUrl> url="
                  << remove_file_url << ", NOT found";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  for (RteFileInfoSharePtr found_info : found_list) {
    if ((current_file_ != nullptr) &&
        (current_file_->file_id == found_info->file_id)) {
      RTE_LOG_ERROR << "<AgoraRtePlayList.RemoveFileByIndex> url="
                    << remove_file_url << ", is playing";
      return ERR_CODE(agora::ERR_ALREADY_IN_USE);
    }
  }

  // Removing found files
  for (RteFileInfoSharePtr found_info : found_list) {
    file_list_.remove(found_info);
  }

  // Update begin_time & file index & total duration
  UpdateFileList(total_duration_);

  return ERR_CODE(agora::ERR_OK);
}

///////////////////////////////////////////////////////////////
///////////////// Only public for RTE Internal ///////////////
///////////////////////////////////////////////////////////////

inline int AgoraRtePlayList::SetCurrFileById(int32_t file_id) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if (file_id == INVALID_RTE_FILE_ID) {
    current_file_ = nullptr;
    return ERR_CODE(agora::ERR_OK);
  }

  RteFileInfoSharePtr found_file = FindFileById(file_id);
  if (found_file == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.SetCurrentFile> file_id=" << file_id
                  << ", NOT found";
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  current_file_ = found_file;
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::SetCurrFileByListTime(
    int64_t time_in_list, int64_t& out_time_in_file) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if ((time_in_list < 0) || (time_in_list >= total_duration_)) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.SetCurrFileByListTime> Invalid time "
                  << time_in_list;
    return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
  }

  std::list<RteFileInfoSharePtr>::iterator it;
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    int64_t end_time = file_info->begin_time + file_info->duration;
    if ((time_in_list >= file_info->begin_time) && (time_in_list < end_time)) {
      current_file_ = file_info;
      out_time_in_file = (time_in_list - file_info->begin_time);
      return agora::ERR_OK;
    }
  }

  RTE_LOG_ERROR << "<AgoraRtePlayList.SetCurrFileByListTime> Invalid time "
                << time_in_list;
  return ERR_CODE(agora::ERR_INVALID_ARGUMENT);
}

inline bool AgoraRtePlayList::CurrentIsFirstFile() {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if ((file_list_.empty()) || (current_file_ == nullptr)) {
    return false;
  }

  RteFileInfoSharePtr front = file_list_.front();
  if (front->file_id == current_file_->file_id) {
    return true;
  }

  return false;
}

inline bool AgoraRtePlayList::CurrentIsLastFile() {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if ((file_list_.empty()) || (current_file_ == nullptr)) {
    return false;
  }

  RteFileInfoSharePtr back = file_list_.back();
  if (back->file_id == current_file_->file_id) {
    return true;
  }

  return false;
}

inline int AgoraRtePlayList::GetCurrentFileInfo(
    RteFileInfo& out_curr_file_info) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  if (current_file_ == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.GetCurrentFileInfo> no current file";
    return ERR_CODE(agora::ERR_INVALID_STATE);
  }

  out_curr_file_info.file_id = current_file_->file_id;
  out_curr_file_info.file_url = current_file_->file_url;
  out_curr_file_info.duration = current_file_->duration;
  out_curr_file_info.video_width = current_file_->video_width;
  out_curr_file_info.video_height = current_file_->video_height;
  out_curr_file_info.frame_rate = current_file_->frame_rate;
  out_curr_file_info.index = current_file_->index;
  out_curr_file_info.begin_time = current_file_->begin_time;
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::MoveCurrentToPrev(bool list_loop) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  RteFileInfoSharePtr file_info = FindPrevFile(list_loop);
  if (file_info == nullptr) {
    RTE_LOG_ERROR
        << "<AgoraRtePlayList.MoveCurrentFileToPrev> no previous file";
    return ERR_CODE(agora::ERR_INVALID_STATE);
  }

  // set new current file
  current_file_ = file_info;
  return ERR_CODE(agora::ERR_OK);
}

inline int AgoraRtePlayList::MoveCurrentToNext(bool list_loop) {
  std::lock_guard<std::recursive_mutex> _(list_lock_);

  RteFileInfoSharePtr file_info = FindNextFile(list_loop);
  if (file_info == nullptr) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.MoveCurrentFileToNext> no next file";
    return ERR_CODE(agora::ERR_INVALID_STATE);
  }

  // set new current file
  current_file_ = file_info;
  return ERR_CODE(agora::ERR_OK);
}

//////////////////////////////////////////////////////////////////////////////
////////////////// Implementation of Internal Methods  ////////////////////////
///////////////////////////////////////////////////////////////////////////////
inline int AgoraRtePlayList::ParseMediaInfo(
    const char* file_url, media::base::PlayerStreamInfo& video_info,
    media::base::PlayerStreamInfo& audio_info) {
  if (streaming_source_ == nullptr) {
    streaming_source_ = AgoraRteSDK::GetRtcService()
                            ->createMediaNodeFactory()
                            ->createMediaStreamingSource();
  }

  if (streaming_source_ == nullptr) {
    return ERR_CODE(agora::ERR_NOT_READY);
  }

  int ret = streaming_source_->parseMediaInfo(file_url, video_info, audio_info);
  if (ret != agora::rtc::STREAMING_SRC_ERR_NONE) {
    RTE_LOG_ERROR << "<AgoraRtePlayList.ParseMediaDuration> fail to parse, url="
                  << file_url;
    return ERR_CODE(agora::ERR_NOT_SUPPORTED);
  }

  int64_t out_duration = (video_info.duration > audio_info.duration)
                             ? video_info.duration
                             : audio_info.duration;
  if (out_duration <= 0) {
    RTE_LOG_ERROR
        << "<AgoraRtePlayList.ParseMediaDuration> media info invalid, url="
        << file_url;
    return ERR_CODE(agora::ERR_NOT_SUPPORTED);
  }

  return ERR_CODE(agora::ERR_OK);
}

inline RteFileInfoSharePtr
AgoraRtePlayList::FindFileById(int32_t find_file_id) {
  std::list<RteFileInfoSharePtr>::iterator it;
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    if (file_info->file_id == find_file_id) {
      return file_info;
    }
  }

  return nullptr;
}

inline RteFileInfoSharePtr
AgoraRtePlayList::FindFileByIndex(int32_t find_file_index) {
  std::list<RteFileInfoSharePtr>::iterator it;
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    if (file_info->index == find_file_index) {
      return file_info;
    }
  }

  return nullptr;
}

inline void AgoraRtePlayList::FindFileByUrl(
    const char* find_file_url,
    std::vector<RteFileInfoSharePtr>& file_info_list) {
  std::list<RteFileInfoSharePtr>::iterator it;
  std::string str_find_url = find_file_url;

  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    std::string str_file_url = file_info->file_url;
    if (str_find_url.compare(str_file_url) == 0) {
      file_info_list.push_back(file_info);
    }
  }
}

inline RteFileInfoSharePtr AgoraRtePlayList::FindPrevFile(bool list_loop) {
  RteFileInfoSharePtr prev_file = nullptr;

  if (current_file_ == nullptr)  // current file is none
  {
    if (list_loop) {
      prev_file = file_list_.back();
    }
    return prev_file;
  }

  std::list<RteFileInfoSharePtr>::iterator it;
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    if (file_info->file_id == current_file_->file_id) {
      if ((list_loop) && (prev_file == nullptr)) {
        prev_file = file_list_.back();
      }
      return prev_file;
    }
    prev_file = file_info;
  }

  return nullptr;  // not found current file
}

inline RteFileInfoSharePtr AgoraRtePlayList::FindNextFile(bool list_loop) {
  RteFileInfoSharePtr next_file = nullptr;

  if (current_file_ == nullptr)  // current file is none
  {
    next_file = file_list_.front();
    return next_file;
  }

  std::list<RteFileInfoSharePtr>::iterator it;
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    if (file_info->file_id == current_file_->file_id) {
      it++;  // next file

      if (it == file_list_.end()) {
        if (list_loop) {
          next_file = file_list_.front();
        }
      } else {
        next_file = (*it);
      }

      return next_file;
    }
  }

  return nullptr;  // not found current file
}

//
// Func: update file index and begin global timestamp
//
inline void AgoraRtePlayList::UpdateFileList(int64_t& out_total_duration) {
  std::list<RteFileInfoSharePtr>::iterator it;
  int64_t begin_time = 0;
  int32_t file_index = 0;
  for (it = file_list_.begin(); it != file_list_.end(); it++) {
    RteFileInfoSharePtr file_info = (*it);
    file_info->begin_time = begin_time;
    file_info->index = file_index;

    begin_time += file_info->duration;
    file_index++;
  }
  out_total_duration = begin_time;
}
//
////
//// Func: Locate file information and position by global timestamp
////
// int AgoraRtePlayList::LocatePosition(int64_t global_timestamp,
// RteFileInfoSharePtr& found_file_info,
//                                          int64_t& pos_in_file) {
//  std::list<RteFileInfoSharePtr>::iterator it;
//  for (it = file_list_.begin(); it != file_list_.end(); it++) {
//    RteFileInfoSharePtr file_info = (*it);
//    int64_t end_time = file_info->begin_time + file_info->duration;
//    if ((global_timestamp >= file_info->begin_time) && (global_timestamp <
//    end_time)) {
//      found_file_info = file_info;
//      pos_in_file = (global_timestamp - file_info->begin_time);
//      return agora::rtc::STREAMING_SRC_ERR_NONE;
//    }
//  }
//
//  // global_timestamp < 0 or global_timestamp >= total_duration
//  return ERR_CODE(agora::rtc::STREAMING_SRC_ERR_INVALID_PARAM);
//}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteRtcCompatibleStream.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "AgoraRteBase.h"

namespace agora {
namespace rte {

inline AgoraRteRtcCompatibleMajorStream::AgoraRteRtcCompatibleMajorStream(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<agora::base::IAgoraService>& rtc_service,
    const std::string& token, const JoinOptions& option)
    : AgoraRteRtcStreamBase(scene, rtc_service, token, option),
      AgoraRteMajorStream(scene->GetLocalUserInfo().user_id,
                          StreamType::kRtcStream) {
  rtc_stream_id_ = scene->GetLocalUserInfo().user_id;
}

inline int AgoraRteRtcCompatibleMajorStream::RegisterObservers() {
  int result = -ERR_FAILED;
  auto scene = scene_.lock();

  if (scene) {
    rtc_major_stream_observer_ =
        RtcCallbackWrapper<AgoraRteRtCompatibleStreamObserver>::Create(
            scene, std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                       shared_from_this()));

    result = internal_rtc_connection_->registerObserver(
        rtc_major_stream_observer_.get(), [](rtc::IRtcConnectionObserver* obs) {
          auto wrapper = static_cast<agora::rte::RtcCallbackWrapper<
              AgoraRteRtCompatibleStreamObserver>*>(obs);
          wrapper->DeleteMe();
        });

    if (result != ERR_OK) return result;

    rtc_major_stream_user_observer_ =
        RtcCallbackWrapper<AgoraRteRtcCompatibleUserObserver>::Create(
            scene, std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                       shared_from_this()));

    auto rtc_local_user = internal_rtc_connection_->getLocalUser();
    result = -ERR_FAILED;

    if (rtc_local_user) {
      result = rtc_local_user->registerLocalUserObserver(
          rtc_major_stream_user_observer_.get(),
          [](rtc::ILocalUserObserver* obs) {
            auto wrapper = static_cast<agora::rte::RtcCallbackWrapper<
                AgoraRteRtcCompatibleUserObserver>*>(obs);
            wrapper->DeleteMe();
          });
    }
  }

  return result;
}

inline void AgoraRteRtcCompatibleMajorStream::UnregisterObservers() {
  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_local_user) {
    rtc_local_user->unregisterLocalUserObserver(
        rtc_major_stream_user_observer_.get());
    internal_rtc_connection_->unregisterObserver(
        rtc_major_stream_observer_.get());

    // Do _not_ call DeleteMe for rtc_user_observer_ as it will be called inside
    // sdk
    //
    auto state = internal_rtc_connection_->getConnectionInfo().state;
    if (state == rtc::CONNECTION_STATE_CONNECTING ||
        state == rtc::CONNECTION_STATE_CONNECTED ||
        state == rtc::CONNECTION_STATE_RECONNECTING) {
      // When the connection is gone, rtc will never fire callback again as we
      // just unregistered the observer above, so here we tell observer to try
      // to fire the exiting event if that is not fired yet
      //
      rtc_major_stream_observer_->TryToFireClosedEvent();
    }
  }
}

inline std::shared_ptr<AgoraRteRtcStreamBase>
AgoraRteRtcCompatibleMajorStream::GetSharedSelf() {
  return std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
      shared_from_this());
}

inline int AgoraRteRtcCompatibleMajorStream::Connect() {
  return AgoraRteRtcStreamBase::Connect();
}

inline void AgoraRteRtcCompatibleMajorStream::Disconnect() {
  return AgoraRteRtcStreamBase::Disconnect();
}

inline int AgoraRteRtcCompatibleMajorStream::PushUserInfo(
    const UserInfo& info, InstanceState state) {
  return 0;
}

inline int AgoraRteRtcCompatibleMajorStream::PushStreamInfo(
    const StreamInfo& info, InstanceState state) {
  return 0;
}

inline int AgoraRteRtcCompatibleMajorStream::PushMediaInfo(
    const StreamInfo& info, MediaType media_type, InstanceState state) {
  return 0;
}

inline void AgoraRteRtcCompatibleMajorStream::AddRemoteStream(
    const std::shared_ptr<AgoraRteRtcRemoteStream>& stream) {
  bool found = false;
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetStreamId() == stream->GetStreamId()) {
      found = true;
      break;
    }
  }

  if (!found) {
    remote_streams_.push_back(stream);
  } else {
    RTE_LOG_WARNING << "Find duplicated remote stream id: "
                    << stream->GetStreamId();
  }
}

inline void AgoraRteRtcCompatibleMajorStream::RemoveRemoteStream(
    const std::string& rtc_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetStreamId() == rtc_stream_id) {
      // Set current stream as the last one, then pop last one
      //
      stream = remote_streams_.back();
      remote_streams_.pop_back();
      break;
    }
  }
}

inline std::shared_ptr<AgoraRteRtcRemoteStream>
AgoraRteRtcCompatibleMajorStream::FindRemoteStream(
    const std::string& stream_id) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetStreamId() == stream_id) {
      return stream;
    }
  }

  return nullptr;
}

inline AgoraRteRtcCompatibleLocalStream::AgoraRteRtcCompatibleLocalStream(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& major_stream)
    : AgoraRteLocalStream(scene->GetLocalUserInfo().user_id,
                          major_stream->GetRtcStreamId(),
                          StreamType::kRtcStream) {
  major_stream_ = major_stream;
}

inline size_t AgoraRteRtcCompatibleMajorStream::GetAudioTrackCount() {
  return audio_tracks_.size();
}

inline size_t AgoraRteRtcCompatibleMajorStream::GetVideoTrackCount() {
  return video_track_ ? 1 : 0;
}

inline int AgoraRteRtcCompatibleMajorStream::UpdateOption(
    const StreamOption& input_option) {
  assert(input_option.type == StreamType::kRtcStream);
  auto& option = static_cast<const RtcStreamOptions&>(input_option);
  return internal_rtc_connection_->renewToken(option.token.c_str());
}

inline int AgoraRteRtcCompatibleMajorStream::PublishLocalAudioTrack(
    std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  Connect();

  int result = SetAudioTrack(track);
  auto local_user = internal_rtc_connection_->getLocalUser();
  auto audio_track = track->GetRtcAudioTrack();

  if (result == ERR_OK && local_user && audio_track) {
    result = local_user->publishAudio(audio_track.get());
  }

  if (result != ERR_OK) {
    UnsetAudioTrack(track);
  }

  return result;
}

inline int AgoraRteRtcCompatibleMajorStream::PublishLocalVideoTrack(
    std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  Connect();

  int result = SetVideoTrack(track);
  auto local_user = internal_rtc_connection_->getLocalUser();
  auto video_track = track->GetRtcVideoTrack();

  if (result == ERR_OK && local_user && video_track) {
    result = local_user->publishVideo(video_track.get());
  }

  if (result != ERR_OK) {
    UnsetVideoTrack(track);
  }

  return result;
}

inline int AgoraRteRtcCompatibleMajorStream::UnpublishLocalAudioTrack(
    std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  int result = UnsetAudioTrack(track);
  auto local_user = internal_rtc_connection_->getLocalUser();
  auto audio_track = track->GetRtcAudioTrack();

  if (result == ERR_OK && local_user && audio_track) {
    result = local_user->unpublishAudio(audio_track.get());
  }

  if (result != ERR_OK) {
    UnsetAudioTrack(track);
  } else {
    contains_audio_ = !audio_tracks_.empty();
  }

  return result;
}

inline int AgoraRteRtcCompatibleMajorStream::UnpublishLocalVideoTrack(
    std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  int result = UnsetVideoTrack(track);
  auto local_user = internal_rtc_connection_->getLocalUser();
  auto video_track = track->GetRtcVideoTrack();

  if (result == ERR_OK && local_user && video_track) {
    result = local_user->unpublishVideo(video_track.get());
  }

  if (result != ERR_OK) {
    UnsetVideoTrack(track);
  } else {
    contains_video_ = false;
  }

  return result;
}

inline int AgoraRteRtcCompatibleMajorStream::SetAudioEncoderConfiguration(
    const AudioEncoderConfiguration& config) {
  auto local_user = internal_rtc_connection_->getLocalUser();
  if (local_user) {
    return local_user->setAudioEncoderConfiguration(config);
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcCompatibleMajorStream::SetVideoEncoderConfiguration(
    const VideoEncoderConfiguration& config) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);

  config_ = config;

  if (video_track_) {
    video_track_->BeforeVideoEncodingChanged(config);
    auto track = video_track_->GetRtcVideoTrack();
    if (track) {
      return track->setVideoEncoderConfiguration(config);
    }
  }
  return -ERR_FAILED;
}

inline int AgoraRteRtcCompatibleMajorStream::UnsetVideoTrack(
    std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  // Block callback while we are erasing track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (video_track_ && video_track_->GetTrackId() == track->GetTrackId()) {
    video_track_ = nullptr;
    return ERR_OK;
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcCompatibleMajorStream::UnsetAudioTrack(
    std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  // Block callback while we are erasing track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  for (auto& audio_track : audio_tracks_) {
    if (audio_track->GetTrackId() == track->GetTrackId()) {
      // Set last track to current track, then pop last one
      //
      audio_track = audio_tracks_.back();
      audio_tracks_.pop_back();
      return ERR_OK;
    }
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcCompatibleMajorStream::SetVideoTrack(
    std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  // Block callback while we are inserting track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (!video_track_) {
    video_track_ = track;
    if (config_.has_value()) {
      SetVideoEncoderConfiguration(config_.value());
    }
    return ERR_OK;
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcCompatibleMajorStream::SetAudioTrack(
    std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  // Block callback while we are inserting track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  bool found = false;
  for (auto& audio_track : audio_tracks_) {
    if (audio_track->GetTrackId() == track->GetTrackId()) {
      found = true;
      break;
    }
  }

  if (!found) {
    audio_tracks_.push_back(track);
    return ERR_OK;
  }

  return -ERR_FAILED;
}

inline void AgoraRteRtcCompatibleMajorStream::OnConnected() {
  CreateRtmpService();
  int ret = 0;

  if (!transcoding_vec_.empty()) {
    agora::rtc::LiveTranscoding transcoding = transcoding_vec_.front();
    ret = rtmp_service_->setLiveTranscoding(transcoding);
    if (ret != agora::ERR_OK) {
      RTE_LOG_INFO
          << "<AgoraRteRtcCompatibleMajorStream::OnConnected> transcoding failed, ret="
          << ret;
    }
    transcoding_vec_.clear();
  }

  for (auto& map_elm : cdnbypass_url_map_) {
    ret = rtmp_service_->addPublishStreamUrl(map_elm.first.c_str(),
                                                 map_elm.second);
    if (ret != agora::ERR_OK) {
      RTE_LOG_INFO << "<AgoraRteRtcCompatibleMajorStream::OnConnected> failed, url="
                   << map_elm.first;
    }
  }
  cdnbypass_url_map_.clear();
}

inline void AgoraRteRtcCompatibleMajorStream::OnAudioTrackPublished(
    const agora_refptr<rtc::ILocalAudioTrack>& audioTrack) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  for (auto& track : audio_tracks_) {
    if (track->GetRtcAudioTrack().get() == audioTrack.get()) {
      track->OnTrackPublished();
      break;
    }
  }
}

inline void AgoraRteRtcCompatibleMajorStream::OnVideoTrackPublished(
    const agora_refptr<rtc::ILocalVideoTrack>& audioTrack) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (video_track_->GetRtcVideoTrack().get() == audioTrack.get()) {
    video_track_->OnTrackPublished();
  }
}

inline int AgoraRteRtcCompatibleMajorStream::EnableLocalAudioObserver(
    const LocalAudioObserverOptions& option) {
  return UpdateLocalAudioObserver(true, option);
}

inline int AgoraRteRtcCompatibleMajorStream::DisableLocalAudioObserver() {
  return UpdateLocalAudioObserver(false);
}

inline int AgoraRteRtcCompatibleMajorStream::SetBypassCdnTranscoding(
    const agora::rtc::LiveTranscoding& transcoding) {
  rtc::TConnectionInfo conn_info =
      internal_rtc_connection_->getConnectionInfo();
  int ret = agora::ERR_OK;

  if (conn_info.state != rtc::CONNECTION_STATE_CONNECTED) {
    // cache the transcoding, we don'nt deep clone the content of
    // LiveTranscoding
    transcoding_vec_.clear();
    transcoding_vec_.push_back(transcoding);

  } else {
    CreateRtmpService();

    transcoding_vec_.clear();
    ret = rtmp_service_->setLiveTranscoding(transcoding);
  }

  return ret;
}

inline int AgoraRteRtcCompatibleMajorStream::AddBypassCdnUrl(
    const std::string& target_cdn_url, bool transcoding_enabled) {
  rtc::TConnectionInfo conn_info =
      internal_rtc_connection_->getConnectionInfo();
  int ret = agora::ERR_OK;

  if (conn_info.state != rtc::CONNECTION_STATE_CONNECTED) {
    // cache the bypass cdn url
    cdnbypass_url_map_.emplace(target_cdn_url, transcoding_enabled);

  } else {
    CreateRtmpService();

    for (auto& map_elm : cdnbypass_url_map_) {
      ret = rtmp_service_->addPublishStreamUrl(map_elm.first.c_str(),
                                               map_elm.second);
      if (ret != agora::ERR_OK) {
        RTE_LOG_INFO << "<AgoraRteRtcCompatibleMajorStream::AddBypassCdnUrl> failed, url="
                     << map_elm.first;
      }
    }
    cdnbypass_url_map_.clear();

    ret = rtmp_service_->addPublishStreamUrl(target_cdn_url.c_str(),
                                             transcoding_enabled);
  }

  return ret;
}

inline int AgoraRteRtcCompatibleMajorStream::RemoveBypassCdnUrl(
    const std::string& target_cdn_url) {
  rtc::TConnectionInfo conn_info =
      internal_rtc_connection_->getConnectionInfo();
  int ret = agora::ERR_OK;

  if (conn_info.state != rtc::CONNECTION_STATE_CONNECTED) {
    // earsh the bypass cdn url
    cdnbypass_url_map_.erase(target_cdn_url);

  } else {
    CreateRtmpService();

    for (auto& map_elm : cdnbypass_url_map_) {
      ret = rtmp_service_->removePublishStreamUrl(map_elm.first.c_str());
      if (ret != agora::ERR_OK) {
        RTE_LOG_INFO
            << "<AgoraRteRtcCompatibleMajorStream::RemoveBypassCdnUrl> failed, url="
            << map_elm.first;
      }
    }
    cdnbypass_url_map_.clear();

    ret = rtmp_service_->removePublishStreamUrl(target_cdn_url.c_str());
  }

  return ret;
}

inline int AgoraRteRtcCompatibleMajorStream::CreateRtmpService() {
  if (rtmp_service_.get() != nullptr) {
    return agora::ERR_OK;
  }

  SdkProfile sdk_profile;
  AgoraRteSDK::GetProfile(sdk_profile);
  auto rtmp_srv = rtc_service_->createRtmpStreamingService(
      internal_rtc_connection_.get(), sdk_profile.appid.c_str());
  assert(rtmp_srv.get() != nullptr);
  rtmp_srv->AddRef();
  rtmp_service_ = {static_cast<rtc::IRtmpStreamingService*>(rtmp_srv.get()),
                   [](rtc::IRtmpStreamingService* con) {
                     if (con) con->Release();
                   }};

  auto scene = scene_.lock();
  rtmp_observer_ =
      RtcCallbackWrapper<AgoraRteRtcCompatibleStreamCdnObserver>::Create(
          scene, std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                     shared_from_this()));
  rtmp_service_->registerObserver(rtmp_observer_.get());
  return agora::ERR_OK;
}

inline int AgoraRteRtcCompatibleMajorStream::DestroyRtmpService() {
  if (rtmp_service_.get() != nullptr) {
    rtmp_service_->unregisterObserver(rtmp_observer_.get());
    rtmp_service_.reset();
    rtmp_observer_.reset();
  }
  return agora::ERR_OK;
}

inline AgoraRteRtcCompatibleUserObserver::AgoraRteRtcCompatibleUserObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& stream) {
  scene_ = scene;
  stream_ = stream;
}

inline AgoraRteRtCompatibleAudioFrameObserver::
    AgoraRteRtCompatibleAudioFrameObserver(
        const std::shared_ptr<AgoraRteScene>& scene,
        const std::shared_ptr<AgoraRteRtcStreamBase>& stream) {
  scene_ = scene;
  stream_ = stream;
}

inline AgoraRteRtcCompatibleRemoteVideoObserver::
    AgoraRteRtcCompatibleRemoteVideoObserver(
        const std::shared_ptr<AgoraRteScene>& scene) {
  scene_ = scene;
}

inline AgoraRteRtCompatibleStreamObserver::
    ~AgoraRteRtCompatibleStreamObserver() {
  if (fire_connection_closed_event_ && !is_close_event_failed_) {
    //  When we are here , the connection is already gone, so we get scene for
    //  weak_ptr
    //
    auto scene = scene_.lock();

    if (scene) {
      scene->ChangeSceneState(SceneState::kDisconnected,
                              rtc::CONNECTION_CHANGED_REASON_TYPE::
                                  CONNECTION_CHANGED_LEAVE_CHANNEL);
    }
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onConnected(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
        rtc::CONNECTION_STATE_CONNECTED);
    scene->ChangeSceneState(conn_stat, reason);
    stream->OnConnected();
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onDisconnected(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Disconnected: " << connectionInfo.localUserId;
  // is_close_event_failed_ is to tell whether we should fire
  // CONNECTION_STATE_DISCONNECTED event in here or destructor function. Note
  // that fire_connection_closed_event_ is for rtc connection to tell us to fire
  // the event before our observer is unregistered
  //
  is_close_event_failed_ = true;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
        rtc::CONNECTION_STATE_DISCONNECTED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onConnecting(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Connecting: " << connectionInfo.localUserId;
  is_close_event_failed_ = false;
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
        rtc::CONNECTION_STATE_CONNECTING);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onReconnecting(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Reconnecting: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
        rtc::CONNECTION_STATE_RECONNECTING);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onReconnected(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Reconnected: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
        rtc::CONNECTION_STATE_CONNECTED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onConnectionLost(
    const rtc::TConnectionInfo& connectionInfo) {
  RTE_LOG_VERBOSE << "LostConnect: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat =
        AgoraRteUtils::GetConnStateFromRtcState(rtc::CONNECTION_STATE_FAILED);
    scene->ChangeSceneState(
        conn_stat,
        rtc::CONNECTION_CHANGED_REASON_TYPE::CONNECTION_CHANGED_LOST);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onUserJoined(
    user_id_t userId) {
  RTE_LOG_VERBOSE << "userId: " << userId;
  std::string user_id_str(userId);
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  std::string rtc_stream_id(userId);
  RteRtcStreamInfo info;

  if (scene && local_stream) {
    auto is_extract_ok =
        AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);
    // Compatible mode could accept both NGAPI protocal connection and
    // RTC protocal connection
    //
    if (is_extract_ok) {
      // NGAPI protocal connection
      //
      if (info.is_major_stream) {
        scene->AddRemoteUser(info.user_id);
      } else if (info.user_id != scene->GetLocalUserInfo().user_id) {
        auto remote_stream = std::make_shared<AgoraRteRtcRemoteStream>(
            info.user_id, local_stream->GetRtcService(), info.stream_id,
            rtc_stream_id, local_stream);

        local_stream->AddRemoteStream(remote_stream);
        scene->AddRemoteStream(info.stream_id, remote_stream);
      }
    }
    else {
      // RTC protocal connection
      //
      scene->AddRemoteUser(user_id_str);

      auto remote_stream = std::make_shared<AgoraRteRtcRemoteStream>(
          user_id_str, local_stream->GetRtcService(), user_id_str, user_id_str,
          local_stream);

      local_stream->AddRemoteStream(remote_stream);
      scene->AddRemoteStream(user_id_str, remote_stream);
    }
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onUserLeft(
    user_id_t userId, rtc::USER_OFFLINE_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "userId: " << userId;
  std::string user_id_str(userId);
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (scene && local_stream) {
    scene->RemoveRemoteUser(user_id_str);

    local_stream->RemoveRemoteStream(user_id_str);
    scene->RemoveRemoteStream(user_id_str);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onTokenPrivilegeWillExpire(
    const char* token) {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (local_stream && scene) {
    std::string token_s(token);
    scene->OnSceneTokenWillExpired(token_s);
  }
}

inline void
AgoraRteRtCompatibleStreamObserver::onTokenPrivilegeDidExpire() {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (local_stream && scene) {
    scene->OnSceneTokenExpired();
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onLastmileQuality(
    const rtc::QUALITY_TYPE quality) {}

inline void AgoraRteRtCompatibleStreamObserver::onLastmileProbeResult(
    const rtc::LastmileProbeResult& result) {}

inline void AgoraRteRtCompatibleStreamObserver::onConnectionFailure(
    const rtc::TConnectionInfo& connectionInfo,
    rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat =
        AgoraRteUtils::GetConnStateFromRtcState(rtc::CONNECTION_STATE_FAILED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtCompatibleStreamObserver::onTransportStats(
    const rtc::RtcStats& stats) {}

inline void
AgoraRteRtCompatibleStreamObserver::onChannelMediaRelayStateChanged(int state,
                                                                     int code) {
}

inline void AgoraRteRtcCompatibleUserObserver::onAudioTrackPublishSuccess(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->OnAudioTrackPublished(audioTrack);
  }
}

inline
AgoraRteRtcCompatibleStreamCdnObserver::AgoraRteRtcCompatibleStreamCdnObserver(
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteRtcCompatibleMajorStream>& stream) {
  scene_ = scene;
  stream_ = stream;
}

inline void
AgoraRteRtcCompatibleStreamCdnObserver::onRtmpStreamingStateChanged(
    const char* url, agora::rtc::RTMP_STREAM_PUBLISH_STATE state,
    agora::rtc::RTMP_STREAM_PUBLISH_ERROR err_code) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnStateChanged(stream_id, url, state, err_code);
  }
}

inline void AgoraRteRtcCompatibleStreamCdnObserver::onStreamPublished(
    const char* url, int error) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnPublished(stream_id, url, error);
  }
}

inline void AgoraRteRtcCompatibleStreamCdnObserver::onStreamUnpublished(
    const char* url) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnUnpublished(stream_id, url);
  }
}

inline void AgoraRteRtcCompatibleStreamCdnObserver::onTranscodingUpdated() {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnTranscodingUpdated(stream_id);
  }
}

// 1
inline void
AgoraRteRtcCompatibleUserObserver::onAudioTrackPublicationFailure(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack, ERROR_CODE_TYPE error) {}
// 1
inline void
AgoraRteRtcCompatibleUserObserver::onLocalAudioTrackStateChanged(
    agora_refptr<rtc::ILocalAudioTrack> audioTrack,
    rtc::LOCAL_AUDIO_STREAM_STATE state,
    rtc::LOCAL_AUDIO_STREAM_ERROR errorCode) {}
// 1
inline void AgoraRteRtcCompatibleUserObserver::onLocalAudioTrackStatistics(
    const rtc::LocalAudioStats& stats) {}

inline void AgoraRteRtcCompatibleUserObserver::onRemoteAudioTrackStatistics(
    agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
    const rtc::RemoteAudioTrackStats& stats) {}

inline void AgoraRteRtcCompatibleUserObserver::onUserAudioTrackSubscribed(
    user_id_t userId, agora_refptr<rtc::IRemoteAudioTrack> audioTrack) {
  std::string user_id_str(userId);
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto remote_stream = stream->FindRemoteStream(user_id_str);
    if (remote_stream) {
      auto audio_track =
          AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IRemoteAudioTrack>(
              audioTrack);
      remote_stream->OnAudioTrackSubscribed(audio_track);
    }
  }
}

inline void AgoraRteRtcCompatibleUserObserver::onUserAudioTrackStateChanged(
    user_id_t userId, agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
    rtc::REMOTE_AUDIO_STATE state, rtc::REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) {}

inline void AgoraRteRtcCompatibleUserObserver::onVideoTrackPublishSuccess(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->OnVideoTrackPublished(videoTrack);
  }
}

inline void
AgoraRteRtcCompatibleUserObserver::onVideoTrackPublicationFailure(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack, ERROR_CODE_TYPE error) {}

inline void
AgoraRteRtcCompatibleUserObserver::onLocalVideoTrackStateChanged(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    rtc::LOCAL_VIDEO_STREAM_STATE state,
    rtc::LOCAL_VIDEO_STREAM_ERROR errorCode) {}

inline void AgoraRteRtcCompatibleUserObserver::onLocalVideoTrackStatistics(
    agora_refptr<rtc::ILocalVideoTrack> videoTrack,
    const rtc::LocalVideoTrackStats& stats) {}

inline void AgoraRteRtcCompatibleUserObserver::onUserVideoTrackSubscribed(
    user_id_t userId, rtc::VideoTrackInfo trackInfo,
    agora_refptr<rtc::IRemoteVideoTrack> videoTrack) {
  std::string user_id_str(userId);
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto remote_stream = stream->FindRemoteStream(user_id_str);
    if (remote_stream) {
      auto video_track =
          AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IRemoteVideoTrack>(
              videoTrack);
      remote_stream->OnVideoTrackSubscribed(video_track);
    }
  }
}

inline void AgoraRteRtcCompatibleUserObserver::onUserVideoTrackStateChanged(
    user_id_t userId, agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
    rtc::REMOTE_VIDEO_STATE state, rtc::REMOTE_VIDEO_STATE_REASON reason,
    int elapsed) {}

inline void AgoraRteRtcCompatibleUserObserver::onRemoteVideoTrackStatistics(
    agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
    const rtc::RemoteVideoTrackStats& stats) {}

inline void AgoraRteRtcCompatibleUserObserver::onAudioVolumeIndication(
    const rtc::AudioVolumeInfo* speakers, unsigned int speakerNumber,
    int totalVolume) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream) {
    std::vector<AudioVolumeInfo> infos(speakerNumber);
    for (unsigned int i = 0; i < speakerNumber; i++) {
      infos.push_back({speakers->userId, speakers->volume});
      speakers++;
    }
    scene->OnAudioVolumeIndication(infos, totalVolume);
  }
}

inline void
AgoraRteRtcCompatibleUserObserver::onSubscribeStateChangedCommon(
    const char* channel, user_id_t userId, rtc::STREAM_SUBSCRIBE_STATE oldState,
    rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState,
    MediaType type) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();
  std::string user_id_str(userId);

  if (scene && stream) {
    SubscribeState state = SubscribeState::kFailed;
    bool notify_callback = false;

    if (oldState == rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBING &&
        newState == rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBED) {
      state = SubscribeState::kSubscribed;
      notify_callback = true;
    }

    if (oldState == rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBED &&
        newState != rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBED) {
      state = SubscribeState::kNoSubscribe;
      notify_callback = true;
    }

    if (notify_callback) {
      StreamInfo info;

      info.stream_id = user_id_str;
      info.user_id = user_id_str;

      if (state == SubscribeState::kSubscribed) {
        switch (type) {
          case MediaType::kAudio:
            stream->OnAudioConnected();
            break;
          case MediaType::kVideo:
            stream->OnVideoConnected();
            break;
          default:
            break;
        }

        scene->OnRemoteStreamStateChanged(
            info, type, StreamMediaState::kIdle, StreamMediaState::kStreaming,
            StreamStateChangedReason::kSubscribed);
      } else {
        switch (type) {
          case MediaType::kAudio:
            stream->OnAudioDisconnected();
            break;
          case MediaType::kVideo:
            stream->OnVideoDisconnected();
            break;
          default:
            break;
        }

        scene->OnRemoteStreamStateChanged(
            info, type, StreamMediaState::kStreaming, StreamMediaState::kIdle,
            StreamStateChangedReason::kUnsubscribed);
      }
    }
  }
}

inline void AgoraRteRtcCompatibleUserObserver::onAudioSubscribeStateChanged(
    const char* channel, user_id_t userId, rtc::STREAM_SUBSCRIBE_STATE oldState,
    rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) {
  onSubscribeStateChangedCommon(channel, userId, oldState, newState,
                                elapseSinceLastState, MediaType::kAudio);
}

inline void AgoraRteRtcCompatibleUserObserver::onVideoSubscribeStateChanged(
    const char* channel, user_id_t userId, rtc::STREAM_SUBSCRIBE_STATE oldState,
    rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) {
  onSubscribeStateChangedCommon(channel, userId, oldState, newState,
                                elapseSinceLastState, MediaType::kVideo);
}

inline void AgoraRteRtcCompatibleUserObserver::onAudioPublishStateChanged(
    const char* channel, rtc::STREAM_PUBLISH_STATE oldState,
    rtc::STREAM_PUBLISH_STATE newState, int elapseSinceLastState) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream){
    AgoraRteRtcObserveHelper::onPublishStateChanged(channel, oldState, newState, elapseSinceLastState,
                              MediaType::kAudio, scene,stream);
  }
}

inline void AgoraRteRtcCompatibleUserObserver::onVideoPublishStateChanged(
    const char* channel, rtc::STREAM_PUBLISH_STATE oldState,
    rtc::STREAM_PUBLISH_STATE newState, int elapseSinceLastState) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream){
  AgoraRteRtcObserveHelper::onPublishStateChanged(channel, oldState, newState, elapseSinceLastState,
                              MediaType::kVideo, scene, stream);
  }
}

inline bool AgoraRteRtCompatibleAudioFrameObserver::onRecordAudioFrame(
    AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnRecordAudioFrame(audioFrame);
  }

  return true;
}

inline bool AgoraRteRtCompatibleAudioFrameObserver::onPlaybackAudioFrame(
    AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnPlaybackAudioFrame(audioFrame);
  }

  return true;
}

inline bool AgoraRteRtCompatibleAudioFrameObserver::onMixedAudioFrame(
    AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnMixedAudioFrame(audioFrame);
  }

  return true;
}

inline bool
AgoraRteRtCompatibleAudioFrameObserver::onPlaybackAudioFrameBeforeMixing(
  user_id_t userId, AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnPlaybackAudioFrameBeforeMixing(userId, audioFrame);
  }
  return true;
}

inline void AgoraRteRtcCompatibleRemoteVideoObserver::onFrame(
    user_id_t userId, rtc::conn_id_t connectionId,
    const media::base::VideoFrame* frame) {
  std::string user_id_str(userId);
  auto scene = scene_.lock();
  if (scene) {
    scene->OnRemoteVideoFrame(user_id_str, *frame);
  }
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteRtcStream.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "AgoraRteBase.h"
#include "IAgoraRtmpStreamingService.h"

namespace agora {
namespace rte {

inline AgoraRteRtcStreamBase::AgoraRteRtcStreamBase(
  const std::shared_ptr<AgoraRteScene>& scene,
  const std::shared_ptr<agora::base::IAgoraService>& rtc_service,
  const std::string& token, const JoinOptions& option) : scene_(scene),
                                                         token_(token) {
  rtc::RtcConnectionConfiguration rtc_conn_cfg_ex;
  rtc_conn_cfg_ex.autoSubscribeAudio = false;
  rtc_conn_cfg_ex.autoSubscribeVideo = false;
  rtc_conn_cfg_ex.enableAudioRecordingOrPlayout =
    scene->IsAudioRecordingOrPlayOutEnabled();

  if (option.is_user_visible_to_remote) {
    rtc_conn_cfg_ex.clientRoleType =
      rtc::CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;
  } else {
    rtc_conn_cfg_ex.clientRoleType =
      rtc::CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;
  }

  auto rtc_con = rtc_service->createRtcConnection(rtc_conn_cfg_ex);
  assert(rtc_con.get() != nullptr);
  rtc_con->AddRef();

  internal_rtc_connection_ = {static_cast<rtc::IRtcConnection*>(rtc_con.get()),
                              [](rtc::IRtcConnection* con) {
                                if (con) {
                                  con->Release();
                                }
                              }};

  rtc_service_ = rtc_service;
}

inline int AgoraRteRtcStreamBase::Connect() {
  auto state = internal_rtc_connection_->getConnectionInfo().state;

  if (state != rtc::CONNECTION_STATE_DISCONNECTED &&
      state != rtc::CONNECTION_STATE_FAILED) {
    // The connection state is still active, return directly
    //
    return ERR_OK;
  }

  auto scene = scene_.lock();

  if (scene) {
    std::string scene_id = scene->GetSceneInfo().scene_id;

    auto rtc_local_user = internal_rtc_connection_->getLocalUser();

    if (rtc_local_user) {
      int result = RegisterObservers();
      if (result != ERR_OK) {
        return result;
      }

      RTE_LOG_INFO << "Connecting " << rtc_stream_id_;

      return internal_rtc_connection_->connect(token_.c_str(), scene_id.c_str(),
                                               rtc_stream_id_.c_str());
    }
  }

  return -ERR_FAILED;
}

inline void AgoraRteRtcStreamBase::Disconnect() {
  UnregisterObservers();

  RTE_LOG_INFO << "Disconnect " << rtc_stream_id_;

  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_audio_observer_) {
    rtc_local_user->unregisterAudioFrameObserver(rtc_audio_observer_.get());
  }

  if (rtc_remote_video_observer_) {
    rtc_local_user->unregisterVideoFrameObserver(
      rtc_remote_video_observer_.get());
  }

  internal_rtc_connection_->disconnect();
}

inline AgoraRteRtcMajorStream::AgoraRteRtcMajorStream(
  const std::shared_ptr<AgoraRteScene>& scene,
  const std::shared_ptr<agora::base::IAgoraService>& rtc_service,
  const std::string& token, const JoinOptions& option)
  : AgoraRteRtcStreamBase(scene, rtc_service, token, option),
    AgoraRteMajorStream(scene->GetLocalUserInfo().user_id,
                        StreamType::kRtcStream) {
  rtc_stream_id_ = AgoraRteUtils::GenerateRtcStreamId(
    true, scene->GetLocalUserInfo().user_id, "");
}

inline int AgoraRteRtcMajorStream::Connect() {
  return AgoraRteRtcStreamBase::Connect();
}

inline void AgoraRteRtcMajorStream::Disconnect() {
  AgoraRteRtcStreamBase::Disconnect();
}

inline int AgoraRteRtcMajorStream::RegisterObservers() {
  int result = -ERR_FAILED;
  auto scene = scene_.lock();

  if (scene) {
    rtc_major_stream_observer_ =
      RtcCallbackWrapper<AgoraRteRtcMajorStreamObserver>::Create(
        scene, std::static_pointer_cast<AgoraRteRtcMajorStream>(
          shared_from_this()));

    result = internal_rtc_connection_->registerObserver(
      rtc_major_stream_observer_.get(), [](rtc::IRtcConnectionObserver* obs) {
        auto wrapper = static_cast<
          agora::rte::RtcCallbackWrapper<AgoraRteRtcMajorStreamObserver>*>(
          obs);
        wrapper->DeleteMe();
      });

    if (result != ERR_OK) {
      return result;
    }

    rtc_major_stream_user_observer_ =
      RtcCallbackWrapper<AgoraRteRtcMajorStreamUserObserver>::Create(
        scene, std::static_pointer_cast<AgoraRteRtcMajorStream>(
          shared_from_this()));

    auto rtc_local_user = internal_rtc_connection_->getLocalUser();
    result = -ERR_FAILED;

    if (rtc_local_user) {
      result = rtc_local_user->registerLocalUserObserver(
        rtc_major_stream_user_observer_.get(),
        [](rtc::ILocalUserObserver* obs) {
          auto wrapper = static_cast<agora::rte::RtcCallbackWrapper<
            AgoraRteRtcMajorStreamUserObserver>*>(obs);
          wrapper->DeleteMe();
        });
    }
  }

  return result;
}

inline int AgoraRteRtcStreamBase::UpdateRemoteAudioObserver(
  bool enable, const RemoteAudioObserverOptions& option) {
  if (enable == is_remote_audio_observer_added_) {
    if (!enable || memcmp(&option, &audio_option_.remote_option,
                          sizeof(RemoteAudioObserverOptions)) == 0) {
      return ERR_OK;
    }
  }

  int result = -ERR_FAILED;
  auto scene = scene_.lock();
  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_local_user && scene) {
    result = ERR_OK;
    if (enable) {
      if (!rtc_audio_observer_) {
        rtc_audio_observer_ = std::make_unique<AgoraRteRtcAudioFrameObserver>(
          scene, GetSharedSelf());
        result = rtc_local_user->registerAudioFrameObserver(
          rtc_audio_observer_.get());
        if (result != ERR_OK) {
          return result;
        }
      }

      size_t numberOfChannels = option.numberOfChannels;
      uint32_t sampleRateHz = option.sampleRateHz;

      rtc_local_user->setPlaybackAudioFrameBeforeMixingParameters(
        numberOfChannels, sampleRateHz);

      is_remote_audio_observer_added_ = true;
      audio_option_.remote_option = option;

    } else {
      // Disable
      //
      if (!is_local_audio_observer_added_) {
        // Both local and remote audio are not required
        //
        result = rtc_local_user->unregisterAudioFrameObserver(
          rtc_audio_observer_.get());
        if (result != ERR_OK) {
          return result;
        }
      } else {
        // Just disable remote audio observer
        //
        rtc_local_user->setPlaybackAudioFrameBeforeMixingParameters(0, 0);
      }

      is_remote_audio_observer_added_ = false;
    }
  }
  return result;
}

inline int AgoraRteRtcStreamBase::UpdateLocalAudioObserver(
  bool enable, const LocalAudioObserverOptions& option) {
  if (enable == is_local_audio_observer_added_) {
    if (!enable || memcmp(&option, &audio_option_.local_option,
                          sizeof(LocalAudioObserverOptions)) == 0) {
      return ERR_OK;
    }
  }

  int result = -ERR_FAILED;
  auto scene = scene_.lock();
  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_local_user && scene) {
    result = ERR_OK;
    if (enable) {
      if (!rtc_audio_observer_) {
        rtc_audio_observer_ = std::make_unique<AgoraRteRtcAudioFrameObserver>(
          scene, GetSharedSelf());
        result = rtc_local_user->registerAudioFrameObserver(
          rtc_audio_observer_.get());
        if (result != ERR_OK) { return result; }
      }

      size_t numberOfChannels = option.numberOfChannels;
      uint32_t sampleRateHz = option.sampleRateHz;

      if (option.after_record) {
        rtc_local_user->setRecordingAudioFrameParameters(numberOfChannels,
                                                         sampleRateHz);
      } else {
        rtc_local_user->setRecordingAudioFrameParameters(0, 0);
      }

      if (option.after_mix) {
        rtc_local_user->setMixedAudioFrameParameters(numberOfChannels,
                                                     sampleRateHz);
      } else {
        rtc_local_user->setMixedAudioFrameParameters(0, 0);
      }

      if (option.before_playback) {
        rtc_local_user->setPlaybackAudioFrameParameters(numberOfChannels,
                                                        sampleRateHz);
      } else {
        rtc_local_user->setPlaybackAudioFrameParameters(0, 0);
      }

      is_local_audio_observer_added_ = true;
      audio_option_.local_option = option;

    } else {
      // Disable
      //
      if (!is_remote_audio_observer_added_) {
        // Both local and remote audio are not required
        //
        result = rtc_local_user->unregisterAudioFrameObserver(
          rtc_audio_observer_.get());
        if (result != ERR_OK) {
          return result;
        }
      } else {
        // Just disable local audio observer
        //
        rtc_local_user->setRecordingAudioFrameParameters(0, 0);
        rtc_local_user->setMixedAudioFrameParameters(0, 0);
        rtc_local_user->setPlaybackAudioFrameParameters(0, 0);
      }

      is_local_audio_observer_added_ = false;
    }
  }
  return result;
}

inline int AgoraRteRtcStreamBase::UpdateRemoteVideoObserver(bool enable) {
  if (enable == is_remote_video_observer_added_) {
    return ERR_OK;
  }
  int result = -ERR_FAILED;
  auto scene = scene_.lock();
  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_local_user && scene) {
    if (enable) {
      if (!rtc_remote_video_observer_) {
        rtc_remote_video_observer_ =
          std::make_unique<AgoraRteRtcRemoteVideoObserver>(scene);
      }

      result = rtc_local_user->registerVideoFrameObserver(
        rtc_remote_video_observer_.get());
      if (result != ERR_OK) {
        return result;
      }

      is_remote_video_observer_added_ = true;
    } else {
      result = rtc_local_user->unregisterVideoFrameObserver(
        rtc_remote_video_observer_.get());
      if (result != ERR_OK) {
        return result;
      }
      is_remote_video_observer_added_ = false;
    }
  }

  return result;
}

inline void AgoraRteRtcMajorStream::UnregisterObservers() {
  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_local_user) {
    rtc_local_user->unregisterLocalUserObserver(
      rtc_major_stream_user_observer_.get());
    internal_rtc_connection_->unregisterObserver(
      rtc_major_stream_observer_.get());

    // Do _not_ call DeleteMe for rtc_user_observer_ as it will be called inside
    // sdk
    //
    auto state = internal_rtc_connection_->getConnectionInfo().state;
    if (state == rtc::CONNECTION_STATE_CONNECTING ||
        state == rtc::CONNECTION_STATE_CONNECTED ||
        state == rtc::CONNECTION_STATE_RECONNECTING) {
      // When the connection is gone, rtc will never fire callback again as we
      // just unregistered the observer above, so here we tell observer to try
      // to fire the exiting event if that is not fired yet
      //
      rtc_major_stream_observer_->TryToFireClosedEvent();
    }
  }
}

inline AgoraRteRtcLocalStream::AgoraRteRtcLocalStream(
  std::shared_ptr<AgoraRteScene> scene,
  std::shared_ptr<agora::base::IAgoraService> rtc_service,
  const std::string& stream_id, const std::string& token)
  : AgoraRteRtcStreamBase(scene, rtc_service, token, {true}),
    AgoraRteLocalStream(scene->GetLocalUserInfo().user_id, stream_id,
                        StreamType::kRtcStream) {
  rtc_stream_id_ = AgoraRteUtils::GenerateRtcStreamId(
    false, scene->GetLocalUserInfo().user_id, stream_id);
}

inline int AgoraRteRtcLocalStream::Connect() {
  return AgoraRteRtcStreamBase::Connect();
}

inline void AgoraRteRtcLocalStream::Disconnect() {
  return AgoraRteRtcStreamBase::Disconnect();
}

inline size_t AgoraRteRtcLocalStream::GetAudioTrackCount() {
  return audio_tracks_.size();
}

inline size_t AgoraRteRtcLocalStream::GetVideoTrackCount() {
  return video_track_ ? 1 : 0;
}

inline int AgoraRteRtcLocalStream::RegisterObservers() {
  int result = -ERR_FAILED;
  auto scene = scene_.lock();
  if (scene) {
    rtc_local_stream_observer_ =
      RtcCallbackWrapper<AgoraRteRtcLocalStreamObserver>::Create(
        scene, std::static_pointer_cast<AgoraRteRtcLocalStream>(
          shared_from_this()));

    result = internal_rtc_connection_->registerObserver(
      rtc_local_stream_observer_.get(), [](rtc::IRtcConnectionObserver* obs) {
        auto wrapper = static_cast<
          agora::rte::RtcCallbackWrapper<AgoraRteRtcLocalStreamObserver>*>(
          obs);
        wrapper->DeleteMe();
      });

    if (result != ERR_OK) { return result; }

    rtc_local_stream_user_observer_ =
      RtcCallbackWrapper<AgoraRteRtcLocalStreamUserObserver>::Create(
        scene, std::static_pointer_cast<AgoraRteRtcLocalStream>(
          shared_from_this()));

    auto rtc_local_user = internal_rtc_connection_->getLocalUser();
    result = rtc_local_user->registerLocalUserObserver(
      rtc_local_stream_user_observer_.get(),
      [](rtc::ILocalUserObserver* obs) {
        auto wrapper = static_cast<agora::rte::RtcCallbackWrapper<
          AgoraRteRtcLocalStreamUserObserver>*>(obs);
        wrapper->DeleteMe();
      });
  }
  return result;
}

inline void AgoraRteRtcLocalStream::UnregisterObservers() {
  auto rtc_local_user = internal_rtc_connection_->getLocalUser();

  if (rtc_local_user) {
    rtc_local_user->unregisterLocalUserObserver(
      rtc_local_stream_user_observer_.get());
    internal_rtc_connection_->unregisterObserver(
      rtc_local_stream_observer_.get());
  }
}

inline int AgoraRteRtcLocalStream::UpdateOption(
  const StreamOption& input_option) {
  assert(input_option.type == StreamType::kRtcStream);
  auto& option = static_cast<const RtcStreamOptions&>(input_option);
  return internal_rtc_connection_->renewToken(option.token.c_str());
}

inline int AgoraRteRtcLocalStream::PublishLocalAudioTrack(
  std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  int result = SetAudioTrack(track);
  if (result != ERR_OK) {
    return result;
  }

  auto local_user = internal_rtc_connection_->getLocalUser();
  auto audio_track = track->GetRtcAudioTrack();

  if (result == ERR_OK && local_user && audio_track) {
    result = local_user->publishAudio(audio_track.get());
  }

  if (result != ERR_OK) {
    UnsetAudioTrack(track);
  }

  return result;
}

inline int AgoraRteRtcLocalStream::PublishLocalVideoTrack(
  std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  int result = SetVideoTrack(track);
  if (result != ERR_OK) {
    return result;
  }

  auto local_user = internal_rtc_connection_->getLocalUser();
  auto video_track = track->GetRtcVideoTrack();

  if (result == ERR_OK && local_user && video_track) {
    result = local_user->publishVideo(video_track.get());
  }

  if (result != ERR_OK) {
    UnsetVideoTrack(track);
  }

  return result;
}

inline int AgoraRteRtcLocalStream::UnpublishLocalAudioTrack(
  std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  int result = UnsetAudioTrack(track);
  if (result != ERR_OK) {
    return result;
  }

  auto local_user = internal_rtc_connection_->getLocalUser();
  auto audio_track = track->GetRtcAudioTrack();

  if (result == ERR_OK && local_user && audio_track) {
    result = local_user->unpublishAudio(audio_track.get());
  }

  if (result == ERR_OK) {
    contains_audio_ = !audio_tracks_.empty();
  }

  return result;
}

inline int AgoraRteRtcLocalStream::UnpublishLocalVideoTrack(
  std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  int result = UnsetVideoTrack(track);
  if (result != ERR_OK) {
    return result;
  }

  auto local_user = internal_rtc_connection_->getLocalUser();
  auto video_track = track->GetRtcVideoTrack();

  if (result == ERR_OK && local_user && video_track) {
    result = local_user->unpublishVideo(video_track.get());
  }

  if (result == ERR_OK) {
    contains_video_ = false;
  }

  return result;
}

inline int AgoraRteRtcLocalStream::SetAudioEncoderConfiguration(
  const AudioEncoderConfiguration& config) {
  auto local_user = internal_rtc_connection_->getLocalUser();
  if (local_user) {
    return local_user->setAudioEncoderConfiguration(config);
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcLocalStream::SetVideoEncoderConfiguration(
  const VideoEncoderConfiguration& config) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);

  config_ = config;

  if (video_track_) {
    video_track_->BeforeVideoEncodingChanged(config);
    auto track = video_track_->GetRtcVideoTrack();
    if (track) {
      return track->setVideoEncoderConfiguration(config);
    }
  }
  return -ERR_FAILED;
}

inline int AgoraRteRtcLocalStream::CreateRtmpService() {
  if (rtmp_service_.get() != nullptr) {
    return agora::ERR_OK;
  }

  SdkProfile sdk_profile;
  AgoraRteSDK::GetProfile(sdk_profile);
  auto rtmp_srv = rtc_service_->createRtmpStreamingService(
    internal_rtc_connection_.get(), sdk_profile.appid.c_str());
  assert(rtmp_srv.get() != nullptr);
  rtmp_srv->AddRef();
  rtmp_service_ = {static_cast<rtc::IRtmpStreamingService*>(rtmp_srv.get()),
                   [](rtc::IRtmpStreamingService* con) {
                     if (con) {
                       con->Release();
                     }
                   }};

  auto scene = scene_.lock();
  rtmp_observer_ =
    RtcCallbackWrapper<AgoraRteRtcLocalStreamCdnObserver>::Create(
      scene,
      std::static_pointer_cast<AgoraRteRtcLocalStream>(shared_from_this()));
  rtmp_service_->registerObserver(rtmp_observer_.get());
  return agora::ERR_OK;
}

inline int AgoraRteRtcLocalStream::DestroyRtmpService() {
  if (rtmp_service_.get() != nullptr) {
    rtmp_service_->unregisterObserver(rtmp_observer_.get());
    rtmp_service_.reset();
    rtmp_observer_.reset();
  }

  return agora::ERR_OK;
}

inline int AgoraRteRtcLocalStream::SetBypassCdnTranscoding(
  const agora::rtc::LiveTranscoding& transcoding) {

  rtc::TConnectionInfo conn_info =
      internal_rtc_connection_->getConnectionInfo();
  int ret = agora::ERR_OK;

  if (conn_info.state != rtc::CONNECTION_STATE_CONNECTED) {
    // cache the transcoding, we don'nt deep clone the content of LiveTranscoding
    transcoding_vec_.clear();
    transcoding_vec_.push_back(transcoding);

  } else {
    CreateRtmpService();

    transcoding_vec_.clear();
    ret = rtmp_service_->setLiveTranscoding(transcoding);
  }

  return ret;
}

inline int AgoraRteRtcLocalStream::AddBypassCdnUrl(
  const std::string& target_cdn_url, bool transcoding_enabled) {
  rtc::TConnectionInfo conn_info = internal_rtc_connection_->getConnectionInfo();
  int ret = agora::ERR_OK;

  if (conn_info.state != rtc::CONNECTION_STATE_CONNECTED) {
    // cache the bypass cdn url
    cdnbypass_url_map_.emplace(target_cdn_url, transcoding_enabled);

  } else {
    CreateRtmpService();

    for (auto& map_elm : cdnbypass_url_map_) {
      ret = rtmp_service_->addPublishStreamUrl(map_elm.first.c_str(),
                                               map_elm.second );
      if (ret != agora::ERR_OK) {
        RTE_LOG_INFO
            << "<AgoraRteRtcLocalStream::AddBypassCdnUrl> failed, url="
            << map_elm.first;
      }
    }
    cdnbypass_url_map_.clear();

    ret = rtmp_service_->addPublishStreamUrl(target_cdn_url.c_str(),
                                              transcoding_enabled);
  }

  return ret;
}

inline int AgoraRteRtcLocalStream::RemoveBypassCdnUrl(
  const std::string& target_cdn_url) {

  rtc::TConnectionInfo conn_info =
      internal_rtc_connection_->getConnectionInfo();
  int ret = agora::ERR_OK;

  if (conn_info.state != rtc::CONNECTION_STATE_CONNECTED) {
    // earsh the bypass cdn url
    cdnbypass_url_map_.erase(target_cdn_url);

  } else {
    CreateRtmpService();

    for (auto& map_elm : cdnbypass_url_map_) {
      ret = rtmp_service_->removePublishStreamUrl(map_elm.first.c_str());
      if (ret != agora::ERR_OK) {
        RTE_LOG_INFO
            << "<AgoraRteRtcLocalStream::RemoveBypassCdnUrl> failed, url="
            << map_elm.first;
      }
    }
    cdnbypass_url_map_.clear();

    ret = rtmp_service_->removePublishStreamUrl(target_cdn_url.c_str());
  }

  return ret;
}

inline int AgoraRteRtcLocalStream::UnsetVideoTrack(
  std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  // Block callback while we are erasing track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (video_track_ && video_track_->GetTrackId() == track->GetTrackId()) {
    video_track_ = nullptr;
    return ERR_OK;
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcLocalStream::UnsetAudioTrack(
  std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  // Block callback while we are erasing track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  for (auto& audio_track : audio_tracks_) {
    if (audio_track->GetTrackId() == track->GetTrackId()) {
      // Set last track to current track, then pop last one
      //
      audio_track = audio_tracks_.back();
      audio_tracks_.pop_back();
      return ERR_OK;
    }
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcLocalStream::SetVideoTrack(
  std::shared_ptr<AgoraRteRtcVideoTrackBase> track) {
  // Block callback while we are inserting track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (!video_track_) {
    video_track_ = track;
    if (config_.has_value()) {
      SetVideoEncoderConfiguration(config_.value());
    }
    return ERR_OK;
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcLocalStream::SetAudioTrack(
  std::shared_ptr<AgoraRteRtcAudioTrackBase> track) {
  // Block callback while we are inserting track
  //
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  bool found = false;
  for (auto& audio_track : audio_tracks_) {
    if (audio_track->GetTrackId() == track->GetTrackId()) {
      found = true;
      break;
    }
  }

  if (!found) {
    audio_tracks_.push_back(track);
    return ERR_OK;
  }

  return -ERR_FAILED;
}

inline void AgoraRteRtcLocalStream::OnConnected( )
{
  CreateRtmpService();
  int ret = 0;

  if (!transcoding_vec_.empty()) {
    agora::rtc::LiveTranscoding transcoding = transcoding_vec_.front();
    ret = rtmp_service_->setLiveTranscoding(transcoding);
    if (ret != agora::ERR_OK) {
      RTE_LOG_INFO
          << "<AgoraRteRtcLocalStream::OnConnected> transcoding failed, ret="
          << ret;
    }
    transcoding_vec_.clear();
  }


  for (auto& map_elm : cdnbypass_url_map_) {
    int ret = rtmp_service_->addPublishStreamUrl(map_elm.first.c_str(),
                                                  map_elm.second);
    if (ret != agora::ERR_OK) {
      RTE_LOG_INFO << "<AgoraRteRtcLocalStream::OnConnected> add url failed, url="
          << map_elm.first << ", ret=" << ret;
    }
  }
  cdnbypass_url_map_.clear();
}


inline void AgoraRteRtcLocalStream::OnAudioTrackPublished(
  const agora_refptr<rtc::ILocalAudioTrack>& audioTrack) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  for (auto& track : audio_tracks_) {
    if (track->GetRtcAudioTrack().get() == audioTrack.get()) {
      track->OnTrackPublished();
      break;
    }
  }
}

inline void AgoraRteRtcLocalStream::OnVideoTrackPublished(
  const agora_refptr<rtc::ILocalVideoTrack>& audioTrack) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (video_track_->GetRtcVideoTrack().get() == audioTrack.get()) {
    video_track_->OnTrackPublished();
  }
}

inline AgoraRteRtcRemoteStream::AgoraRteRtcRemoteStream(
  const std::string& user_id,
  std::shared_ptr<agora::base::IAgoraService> rtc_service,
  const std::string& stream_id, const std::string& rtc_stream_id,
  std::shared_ptr<AgoraRteRtcStreamBase> local_stream)
  : AgoraRteRemoteStream(user_id, stream_id, StreamType::kRtcStream) {
  media_node_factory_ = rtc_service->createMediaNodeFactory();
  rtc_stream_ = local_stream;

  rtc_stream_id_ = rtc_stream_id;
}

inline AgoraRteRtcRemoteStream::~AgoraRteRtcRemoteStream() {
  RTE_LOG_VERBOSE << "Remove Remote Stream";
  if (render_) {
    if (video_track_) {
      video_track_->removeRenderer(render_);
    }
    render_->unsetView();
  }
}

inline int AgoraRteRtcRemoteStream::SubscribeRemoteAudio() {
  auto major_stream = rtc_stream_.lock();
  if (major_stream) {
    auto rtc_connection = major_stream->GetRtcConnection();
    auto rtc_user = rtc_connection->getLocalUser();
    if (rtc_user) {
      return rtc_user->subscribeAudio(rtc_stream_id_.c_str());
    }
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::UnsubscribeRemoteAudio() {
  auto major_stream = rtc_stream_.lock();
  if (major_stream) {
    auto rtc_connection = major_stream->GetRtcConnection();
    auto rtc_user = rtc_connection->getLocalUser();
    if (rtc_user) {
      int result = rtc_user->unsubscribeAudio(rtc_stream_id_.c_str());

      if (result == ERR_OK) {
        contains_audio_ = false;
      }

      return result;
    }
  }
  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::SubscribeRemoteVideo(
  const VideoSubscribeOptions& options) {
  auto major_stream = rtc_stream_.lock();
  if (major_stream) {
    auto rtc_connection = major_stream->GetRtcConnection();
    auto rtc_user = rtc_connection->getLocalUser();
    if (rtc_user) {
      rtc::ILocalUser::VideoSubscriptionOptions subs_options;
      subs_options.type = options.type;

      return rtc_user->subscribeVideo(rtc_stream_id_.c_str(), subs_options);
    }
  }
  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::UnsubscribeRemoteVideo() {
  auto major_stream = rtc_stream_.lock();
  if (major_stream) {
    auto rtc_connection = major_stream->GetRtcConnection();
    auto rtc_user = rtc_connection->getLocalUser();
    if (rtc_user) {
      int result = rtc_user->unsubscribeVideo(rtc_stream_id_.c_str());
      if (result == ERR_OK) {
        contains_video_ = false;
      }
    }
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::SetRemoteVideoCanvas(
  const VideoCanvas& canvas) {
  std::lock_guard<std::recursive_mutex> _(callback_lock_);
  if (!render_) {
    render_ = media_node_factory_->createVideoRenderer();
  }

  int result = -ERR_FAILED;

  if (render_) {
    result = render_->setMirror(canvas.mirrorMode);

    if (result != ERR_OK) {
      return result;
    }

    result = render_->setRenderMode(canvas.renderMode);

    if (result != ERR_OK) {
      return result;
    }

    result = render_->setView(canvas.view);

    if (result != ERR_OK) {
      return result;
    }

    if (video_track_) {
      video_track_->addRenderer(render_);
    }
  }

  return result;
}

inline int AgoraRteRtcRemoteStream::EnableRemoteVideoObserver() {
  auto rtc_stream = rtc_stream_.lock();
  if (rtc_stream) {
    constexpr bool enable = true;
    return rtc_stream->UpdateRemoteVideoObserver(enable);
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::DisableRemoveVideoObserver() {
  auto rtc_stream = rtc_stream_.lock();
  if (rtc_stream) {
    constexpr bool enable = true;
    return rtc_stream->UpdateRemoteVideoObserver(!enable);
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::EnableRemoteAudioObserver(
  const RemoteAudioObserverOptions& option) {
  auto rtc_stream = rtc_stream_.lock();
  if (rtc_stream) {
    constexpr bool enable = true;
    return rtc_stream->UpdateRemoteAudioObserver(enable, option);
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::DisableRemoteAudioObserver() {
  auto rtc_stream = rtc_stream_.lock();
  if (rtc_stream) {
    constexpr bool enable = true;
    return rtc_stream->UpdateRemoteAudioObserver(!enable);
  }

  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::AdjustRemoteVolume(int volume) {
  auto rtc_stream = rtc_stream_.lock();
  if (!rtc_stream) {
    return -ERR_FAILED;
  }

  auto rtc_connection = rtc_stream->GetRtcConnection();
  if (!rtc_connection) { return -ERR_FAILED; }

  if (volume < 0) {
    volume = 0;
  }

  if (volume > 100) {
    volume = 100;
  }

  auto local_user = rtc_connection->getLocalUser();
  if (local_user) {
    return local_user->adjustUserPlaybackSignalVolume(rtc_stream_id_.c_str(),
                                                      volume);
  }
  return -ERR_FAILED;
}

inline int AgoraRteRtcRemoteStream::GetRemoteVolume(int* volume) {
  auto rtc_stream = rtc_stream_.lock();
  if (!rtc_stream) {
    return -ERR_FAILED;
  }

  auto rtc_connection = rtc_stream->GetRtcConnection();
  if (!rtc_connection) {
    return -ERR_FAILED;
  }

  return rtc_connection->getLocalUser()->getUserPlaybackSignalVolume(
    rtc_stream_id_.c_str(), volume);
}

inline void AgoraRteRtcMajorStream::AddRemoteStream(
  const std::shared_ptr<AgoraRteRtcRemoteStream>& stream) {
  bool found = false;
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& elem : remote_streams_) {
    if (elem->GetStreamId() == stream->GetStreamId()) {
      found = true;
      break;
    }
  }

  if (!found) {
    remote_streams_.push_back(stream);
  } else {
    RTE_LOG_WARNING << "Find duplicated remote stream id: "
                    << stream->GetStreamId();
  }
}

inline void AgoraRteRtcMajorStream::RemoveRemoteStream(
  const std::string& rtc_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetStreamId() == rtc_stream_id) {
      // Set current stream as the last one, then pop last one
      //
      stream = remote_streams_.back();
      remote_streams_.pop_back();
      break;
    }
  }
}

inline std::shared_ptr<AgoraRteRtcRemoteStream>
AgoraRteRtcMajorStream::FindRemoteStream(const std::string& stream_id) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetStreamId() == stream_id) {
      return stream;
    }
  }

  return nullptr;
}

inline std::shared_ptr<AgoraRteRtcRemoteStream>
AgoraRteRtcMajorStream::FindRemoteStream(
  const agora_refptr<rtc::IRemoteVideoTrack>& videoTrack) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetVideoTrack().get() == videoTrack.get()) {
      return stream;
    }
  }
  return nullptr;
}

inline std::shared_ptr<AgoraRteRtcRemoteStream>
AgoraRteRtcMajorStream::FindRemoteStream(
  const agora_refptr<rtc::IRemoteAudioTrack>& audioTrack) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  for (auto& stream : remote_streams_) {
    if (stream->GetAudioTrack().get() == audioTrack.get()) {
      return stream;
    }
  }
  return nullptr;
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteRtcStreamObserver.cpp ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include "AgoraRteBase.h"
#include "AgoraRteSdkImpl.hpp" 
#include "IAgoraRtmpStreamingService.h"

namespace agora {
namespace rte {

inline void RteStatsConvertHelper::LocalAudioStats(const rtc::LocalAudioStats& stats,
                              RteLocalAudioStats& dest_stats) {
  dest_stats.numChannels = stats.numChannels;
  dest_stats.sentSampleRate = stats.sentSampleRate;
  dest_stats.sentBitrate = stats.sentBitrate;
  dest_stats.internalCodec = stats.internalCodec;
}

inline void RteStatsConvertHelper::LocalVideoStats(const rtc::LocalVideoTrackStats& stats,
                              RteLocalVideoStats& dest_stats) {
  dest_stats.encoderOutputFrameRate = stats.encode_frame_rate;
  dest_stats.rendererOutputFrameRate = stats.render_frame_rate;
  dest_stats.encodedFrameWidth = stats.width;
  dest_stats.encodedFrameHeight = stats.height;
  dest_stats.encodedFrameCount = static_cast<int>(stats.frames_encoded);
}

inline void RteStatsConvertHelper::RemoteAudioStats(const rtc::RemoteAudioTrackStats& stats,
                               RteRemoteAudioStats& dest_stats) {
  dest_stats.quality = stats.quality;
  dest_stats.networkTransportDelay = stats.network_transport_delay;
  dest_stats.jitterBufferDelay = static_cast<int>(stats.jitter_buffer_delay);
  dest_stats.audioLossRate = stats.audio_loss_rate;
  dest_stats.numChannels = stats.num_channels;
  dest_stats.receivedSampleRate = stats.received_sample_rate;
  dest_stats.receivedBitrate = stats.received_bitrate;
  dest_stats.totalFrozenTime = stats.total_frozen_time;
  dest_stats.frozenRate = stats.frozen_rate;
  dest_stats.mosValue = static_cast<int>(stats.mos_value);
}

inline void RteStatsConvertHelper::RemoteVideoStats(const rtc::RemoteVideoTrackStats& stats,
                               RteRemoteVideoStats dest_stats) {
  dest_stats.delay = stats.delay;
  dest_stats.width = stats.width;
  dest_stats.height = stats.height;
  dest_stats.receivedBitrate = stats.receivedBitrate;
  dest_stats.decoderOutputFrameRate = stats.decoderOutputFrameRate;
  dest_stats.rendererOutputFrameRate = stats.rendererOutputFrameRate;
  dest_stats.frameLossRate = stats.frameLossRate;
  dest_stats.packetLossRate = stats.packetLossRate;
  dest_stats.rxStreamType = stats.rxStreamType;
  dest_stats.totalFrozenTime = stats.totalFrozenTime;
  dest_stats.frozenRate = stats.frozenRate;
  dest_stats.avSyncTimeMs = stats.avSyncTimeMs;
}

inline AgoraRteRtcMajorStreamObserver::~AgoraRteRtcMajorStreamObserver() {
  if (fire_connection_closed_event_ && !is_close_event_failed_) {
    // The connection is already gone, so we get scene for weak_ptr
    //
    auto scene = scene_.lock();

    if (scene) {
      scene->ChangeSceneState(SceneState::kDisconnected,
                              rtc::CONNECTION_CHANGED_REASON_TYPE::
                              CONNECTION_CHANGED_LEAVE_CHANNEL);
    }
  }
}

inline void AgoraRteRtcMajorStreamObserver::onConnected(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Connected: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
      rtc::CONNECTION_STATE_CONNECTED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onDisconnected(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Disconnected: " << connectionInfo.localUserId;
  // is_close_event_failed_ is to tell whether we should fire
  // CONNECTION_STATE_DISCONNECTED event in here or destructor function. Note
  // that fire_connection_closed_event_ is for rtc connection to tell us to fire
  // the event before our observer is unregistered
  //
  is_close_event_failed_ = true;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
      rtc::CONNECTION_STATE_DISCONNECTED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onConnecting(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Connecting: " << connectionInfo.localUserId;
  is_close_event_failed_ = false;
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
      rtc::CONNECTION_STATE_CONNECTING);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onReconnecting(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Reconnecting: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
      rtc::CONNECTION_STATE_RECONNECTING);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onReconnected(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  RTE_LOG_VERBOSE << "Reconnected: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
    auto conn_stat = AgoraRteUtils::GetConnStateFromRtcState(
      rtc::CONNECTION_STATE_CONNECTED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onConnectionLost(
  const rtc::TConnectionInfo& connectionInfo) {
  RTE_LOG_VERBOSE << "LostConnect: " << connectionInfo.localUserId;

  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat =
      AgoraRteUtils::GetConnStateFromRtcState(rtc::CONNECTION_STATE_FAILED);
    scene->ChangeSceneState(
      conn_stat,
      rtc::CONNECTION_CHANGED_REASON_TYPE::CONNECTION_CHANGED_LOST);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onUserJoined(user_id_t userId) {
  RTE_LOG_VERBOSE << "userId: " << userId;
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (scene && local_stream) {
    std::string rtc_stream_id(userId);
    RteRtcStreamInfo info;

    auto is_extract_ok =
      AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);
    // Here we only accept NGAPI protocol connection
    //
    if (is_extract_ok) {
      if (info.is_major_stream) {
        scene->AddRemoteUser(info.user_id);
      } else if (info.user_id != scene->GetLocalUserInfo().user_id) {
        auto remote_stream = std::make_shared<AgoraRteRtcRemoteStream>(
          info.user_id, local_stream->GetRtcService(), info.stream_id,
          rtc_stream_id, local_stream);

        local_stream->AddRemoteStream(remote_stream);
        scene->AddRemoteStream(info.stream_id, remote_stream);
      }
    } else {
      RTE_LOG_ERROR << "Failed to parser userId: " << userId;
      assert(false);
    }
  }
}

inline void AgoraRteRtcMajorStreamObserver::onUserLeft(
  user_id_t userId, rtc::USER_OFFLINE_REASON_TYPE reason) {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (scene && local_stream) {
    std::string rtc_stream_id(userId);
    RteRtcStreamInfo info;

    auto is_extract_ok =
      AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);

    if (is_extract_ok) {
      if (info.is_major_stream) {
        scene->RemoveRemoteUser(info.user_id);
      } else if (info.user_id != scene->GetLocalUserInfo().user_id) {
        local_stream->RemoveRemoteStream(info.stream_id);
        scene->RemoveRemoteStream(info.stream_id);
      }
    } else {
      RTE_LOG_ERROR << "Failed to parser userId: " << userId;
      assert(false);
    }
  }
}

inline void AgoraRteRtcMajorStreamObserver::onTokenPrivilegeWillExpire(
  const char* token) {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (local_stream && scene) {
    std::string token_s(token);
    scene->OnSceneTokenWillExpired(token_s);
  }
}

inline void AgoraRteRtcMajorStreamObserver::onTokenPrivilegeDidExpire() {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();

  if (local_stream && scene) {
    scene->OnSceneTokenExpired();
  }
}

inline void AgoraRteRtcLocalStreamObserver::onTokenPrivilegeWillExpire(
  const char* token) {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();
  if (local_stream && scene) {
    std::string token_s(token);
    scene->OnStreamTokenWillExpire(local_stream->GetStreamId(), token_s);
  }
}

inline void
AgoraRteRtcLocalStreamObserver::onTransportStats(const rtc::RtcStats& stats) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream) {
    scene->OnRtcStats(stream->GetStreamId(), stats);
  }
}

inline void AgoraRteRtcLocalStreamObserver::onTokenPrivilegeDidExpire() {
  auto local_stream = stream_.lock();
  auto scene = scene_.lock();
  if (local_stream && scene) {
    scene->OnStreamTokenExpired(local_stream->GetStreamId());
  }
}

inline void AgoraRteRtcLocalStreamObserver::onConnected(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  if (stream) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
    stream->OnConnected();
  }
}

inline void AgoraRteRtcLocalStreamObserver::onConnecting(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  if (stream) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
  }
}

inline void AgoraRteRtcLocalStreamObserver::onReconnecting(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  if (stream) {
    stream->SetLocalUid(static_cast
                          <uint32_t>(connectionInfo
        .internalUid));
  }
}

inline void AgoraRteRtcLocalStreamObserver::onReconnected(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  if (stream) {
    stream->SetLocalUid(static_cast<uint32_t>(connectionInfo.internalUid));
  }
}

inline void AgoraRteRtcMajorStreamObserver::onConnectionFailure(
  const rtc::TConnectionInfo& connectionInfo,
  rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto conn_stat =
      AgoraRteUtils::GetConnStateFromRtcState(rtc::CONNECTION_STATE_FAILED);
    scene->ChangeSceneState(conn_stat, reason);
  }
}

inline void
AgoraRteRtcMajorStreamObserver::onTransportStats(const rtc::RtcStats& stats) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    scene->OnRtcStats(stream->GetStreamId(), stats);
  }
}

inline void AgoraRteRtcLocalStreamUserObserver::onAudioTrackPublishSuccess(
  agora_refptr<rtc::ILocalAudioTrack> audioTrack) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->OnAudioTrackPublished(audioTrack);
  }
}

inline void AgoraRteRtcLocalStreamUserObserver::onLocalAudioTrackStatistics(
  const rtc::LocalAudioStats& stats) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    RteLocalAudioStats audio_stats = {};
    RteStatsConvertHelper::LocalAudioStats(stats, audio_stats);
    local_stream_stats_manager_.SetRteLocalAudioStats(audio_stats);
  }
}

inline void
AgoraRteRtcMajorStreamUserObserver::onRemoteAudioTrackStatistics(
  agora_refptr<rtc::IRemoteAudioTrack> audioTrack,
  const rtc::RemoteAudioTrackStats& stats) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream) {
    RteRemoteAudioStats dest_stats = {};
    RteStatsConvertHelper::RemoteAudioStats(stats, dest_stats);
    remote_stream_stats_manager_.SetRteRemoteAudioStats(stream->GetStreamId(),
                                                        dest_stats);
  }
}

inline void AgoraRteRtcMajorStreamUserObserver::onUserAudioTrackSubscribed(
  user_id_t userId, agora_refptr<rtc::IRemoteAudioTrack> audioTrack) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string rtc_stream_id(userId);
    RteRtcStreamInfo info;

    AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);

    auto remote_stream = stream->FindRemoteStream(info.stream_id);
    if (remote_stream) {
      auto audio_track =
        AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IRemoteAudioTrack>(
          audioTrack);
      remote_stream->OnAudioTrackSubscribed(audio_track);
    }
  }
}

inline void AgoraRteRtcLocalStreamUserObserver::onVideoTrackPublishSuccess(
  agora_refptr<rtc::ILocalVideoTrack> videoTrack) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    stream->OnVideoTrackPublished(videoTrack);
  }
}

inline void AgoraRteRtcLocalStreamUserObserver::onLocalVideoTrackStatistics(
  agora_refptr<rtc::ILocalVideoTrack> videoTrack,
  const rtc::LocalVideoTrackStats& stats) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    RteLocalVideoStats video_stats = {};
    RteStatsConvertHelper::LocalVideoStats(stats, video_stats);
    local_stream_stats_manager_.SetRteLocalVideoStats(video_stats);

    auto local_stream_stats = local_stream_stats_manager_.GetLocalStreamStats();
    scene->OnLocalStreamStats(stream->GetStreamId(), local_stream_stats);
  }
}

inline void
AgoraRteRtcLocalStreamUserObserver::LocalStreamStatsManager::SetRteLocalAudioStats(
  const RteLocalAudioStats& stats) {
  std::lock_guard<std::mutex> lock(local_stream_stats_mutex_);
  local_stream_stats_.audio_stats = stats;
}

inline void
AgoraRteRtcLocalStreamUserObserver::LocalStreamStatsManager::SetRteLocalVideoStats(
  const RteLocalVideoStats& stats) {
  std::lock_guard<std::mutex> lock(local_stream_stats_mutex_);
  local_stream_stats_.video_stats = stats;
}

inline LocalStreamStats
AgoraRteRtcLocalStreamUserObserver::LocalStreamStatsManager::GetLocalStreamStats() {
  std::lock_guard<std::mutex> lock(local_stream_stats_mutex_);
  return local_stream_stats_;
}

inline void AgoraRteRtcMajorStreamUserObserver::onUserVideoTrackSubscribed(
  user_id_t userId, rtc::VideoTrackInfo trackInfo,
  agora_refptr<rtc::IRemoteVideoTrack> videoTrack) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string rtc_stream_id(userId);
    RteRtcStreamInfo info;
    AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);

    auto remote_stream = stream->FindRemoteStream(info.stream_id);
    if (remote_stream) {
      auto video_track =
        AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IRemoteVideoTrack>(
          videoTrack);
      remote_stream->OnVideoTrackSubscribed(video_track);
    }
  }
}

inline void
AgoraRteRtcMajorStreamUserObserver::onRemoteVideoTrackStatistics(
  agora_refptr<rtc::IRemoteVideoTrack> videoTrack,
  const rtc::RemoteVideoTrackStats& stats) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    auto remote_stream = stream->FindRemoteStream(videoTrack);
    if (remote_stream) {
      RteRemoteVideoStats dest_stats = {};
      RteStatsConvertHelper::RemoteVideoStats(stats, dest_stats);
      std::string stream_id = remote_stream->GetStreamId();
      remote_stream_stats_manager_.SetRteRemoteVideoStats(stream_id,
                                                          dest_stats);

      RemoteStreamStats remote_stream_stats = {};
      if (remote_stream_stats_manager_.GetRemoteStreamStats(stream_id,
                                                            remote_stream_stats)) {
        scene->OnRemoteStreamStats(stream_id,
                                   remote_stream_stats);
      }
    }
  }
}

inline void AgoraRteRtcMajorStreamUserObserver::onAudioVolumeIndication(
  const rtc::AudioVolumeInfo* speakers, unsigned int speakerNumber,
  int totalVolume) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream) {
    std::vector<AudioVolumeInfo> infos(speakerNumber);
    for (unsigned int i = 0; i < speakerNumber; i++) {
      std::string user_id = speakers->userId;
      if ("0" != user_id) {
        RteRtcStreamInfo info;
        auto is_extract_ok = AgoraRteUtils::ExtractRtcStreamInfo(user_id, info);
        if (!is_extract_ok) {
          RTE_LOG_ERROR << "extract rtc stream info fail, userId: " << user_id;
          continue;
        }
        user_id = info.user_id;
      }

      infos.push_back({user_id, speakers->volume});
      speakers++;
    }
    scene->OnAudioVolumeIndication(infos, totalVolume);
  }
}

inline void
AgoraRteRtcMajorStreamUserObserver::onAudioSubscribeStateChanged(
  const char* channel, user_id_t userId, rtc::STREAM_SUBSCRIBE_STATE oldState,
  rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) {
  onSubscribeStateChangedCommon(channel, userId, oldState, newState,
                                elapseSinceLastState, MediaType::kAudio);
}

inline void
AgoraRteRtcMajorStreamUserObserver::onVideoSubscribeStateChanged(
  const char* channel, user_id_t userId, rtc::STREAM_SUBSCRIBE_STATE oldState,
  rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState) {
  onSubscribeStateChangedCommon(channel, userId, oldState, newState,
                                elapseSinceLastState, MediaType::kVideo);
}

inline void AgoraRteRtcLocalStreamUserObserver::onAudioPublishStateChanged(
  const char* channel, rtc::STREAM_PUBLISH_STATE oldState,
  rtc::STREAM_PUBLISH_STATE newState, int elapseSinceLastState) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream){
    AgoraRteRtcObserveHelper::onPublishStateChanged(channel, oldState, newState, elapseSinceLastState,
                              MediaType::kAudio, scene,stream);
  }
}

inline void AgoraRteRtcLocalStreamUserObserver::onVideoPublishStateChanged(
  const char* channel, rtc::STREAM_PUBLISH_STATE oldState,
  rtc::STREAM_PUBLISH_STATE newState, int elapseSinceLastState) {
  auto scene = scene_.lock();
  auto stream = stream_.lock();
  if (scene && stream){
    AgoraRteRtcObserveHelper::onPublishStateChanged(channel, oldState, newState, elapseSinceLastState,
                              MediaType::kVideo, scene, stream);
  }
}

inline void
AgoraRteRtcMajorStreamUserObserver::onSubscribeStateChangedCommon(
  const char* channel, user_id_t userId, rtc::STREAM_SUBSCRIBE_STATE oldState,
  rtc::STREAM_SUBSCRIBE_STATE newState, int elapseSinceLastState,
  MediaType type) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (scene && stream) {
    SubscribeState state = SubscribeState::kFailed;
    bool notify_callback = false;

    if (oldState == rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBING &&
        newState == rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBED) {
      state = SubscribeState::kSubscribed;
      notify_callback = true;
    }

    if (oldState == rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBED &&
        newState != rtc::STREAM_SUBSCRIBE_STATE::SUB_STATE_SUBSCRIBED) {
      state = SubscribeState::kNoSubscribe;
      notify_callback = true;
    }

    if (notify_callback) {
      StreamInfo stream_info;
      RteRtcStreamInfo rtc_info;
      std::string rtc_stream_id(userId);

      auto is_extract_ok =
        AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, rtc_info);

      if (is_extract_ok) {
        stream_info.stream_id = rtc_info.stream_id;
        stream_info.user_id = rtc_info.user_id;

        if (state == SubscribeState::kSubscribed) {
          switch (type) {
            case MediaType::kAudio:
              stream->OnAudioConnected();
              break;
            case MediaType::kVideo:
              stream->OnVideoConnected();
              break;
            default:
              break;
          }

          scene->OnRemoteStreamStateChanged(
            stream_info, type, StreamMediaState::kIdle,
            StreamMediaState::kStreaming,
            StreamStateChangedReason::kSubscribed);
        } else {
          switch (type) {
            case MediaType::kAudio:
              stream->OnAudioDisconnected();
              break;
            case MediaType::kVideo:
              stream->OnVideoDisconnected();
              break;
            default:
              break;
          }

          scene->OnRemoteStreamStateChanged(
            stream_info, type, StreamMediaState::kStreaming,
            StreamMediaState::kIdle, StreamStateChangedReason::kUnsubscribed);
        }
      }
    }
  }
}

inline void
AgoraRteRtcMajorStreamUserObserver::RemoteStreamStatsManager::SetRteRemoteAudioStats(
  const std::string& stream_id, const RteRemoteAudioStats& stats) {
  std::lock_guard<std::mutex> lock(remote_stream_stats_list_mutex_);
  remote_stream_stats_list_[stream_id].audio_stats = stats;
}

inline void
AgoraRteRtcMajorStreamUserObserver::RemoteStreamStatsManager::SetRteRemoteVideoStats(
  const std::string& stream_id, const RteRemoteVideoStats& stats) {
  std::lock_guard<std::mutex> lock(remote_stream_stats_list_mutex_);
  remote_stream_stats_list_[stream_id].video_stats = stats;
}

inline bool
AgoraRteRtcMajorStreamUserObserver::RemoteStreamStatsManager::GetRemoteStreamStats(
  const std::string& stream_id, RemoteStreamStats& stats) {
  std::lock_guard<std::mutex> lock(remote_stream_stats_list_mutex_);
  auto search = remote_stream_stats_list_.find(stream_id);
  if (remote_stream_stats_list_.end() == search) {
    return false;
  }

  stats = search->second;
  return true;
}

inline void AgoraRteRtcLocalStreamCdnObserver::onRtmpStreamingStateChanged(
  const char* url, agora::rtc::RTMP_STREAM_PUBLISH_STATE state,
  agora::rtc::RTMP_STREAM_PUBLISH_ERROR err_code) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnStateChanged(stream_id, url, state, err_code);
  }
}

inline void AgoraRteRtcLocalStreamCdnObserver::onStreamPublished(
  const char* url, int error) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnPublished(stream_id, url, error);
  }
}

inline void AgoraRteRtcLocalStreamCdnObserver::onStreamUnpublished(
  const char* url) {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnUnpublished(stream_id, url);
  }
}

inline void AgoraRteRtcLocalStreamCdnObserver::onTranscodingUpdated() {
  auto stream = stream_.lock();
  auto scene = scene_.lock();

  if (stream && scene) {
    std::string stream_id = stream->GetStreamId();
    scene->OnCdnTranscodingUpdated(stream_id);
  }
}

inline bool AgoraRteRtcAudioFrameObserver::onRecordAudioFrame(
  AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnRecordAudioFrame(audioFrame);
  }

  return true;
}

inline bool AgoraRteRtcAudioFrameObserver::onPlaybackAudioFrame(
  AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnPlaybackAudioFrame(audioFrame);
  }

  return true;
}

inline bool AgoraRteRtcAudioFrameObserver::onMixedAudioFrame(
  AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    scene->OnMixedAudioFrame(audioFrame);
  }

  return true;
}

inline bool AgoraRteRtcAudioFrameObserver::onPlaybackAudioFrameBeforeMixing(
  user_id_t userId, AudioFrame& audioFrame) {
  auto scene = scene_.lock();
  if (scene) {
    RteRtcStreamInfo info;
    std::string rtc_stream_id(userId);

    auto is_extract_ok =
      AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);

    if (is_extract_ok) {
      scene->OnPlaybackAudioFrameBeforeMixing(userId, audioFrame);
    }
  }
  return true;
}

inline void AgoraRteRtcRemoteVideoObserver::onFrame(
  user_id_t userId, rtc::conn_id_t connectionId,
  const media::base::VideoFrame* frame) {
  auto scene = scene_.lock();
  if (scene) {
    RteRtcStreamInfo info;
    std::string rtc_stream_id(userId);

    auto is_extract_ok =
      AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, info);

    if (is_extract_ok) {
      scene->OnRemoteVideoFrame(info.stream_id, *frame);
    }
  }
}

inline void AgoraRteRtcObserveHelper::onPublishStateChanged(
    const char* channel, rtc::STREAM_PUBLISH_STATE oldState,
    rtc::STREAM_PUBLISH_STATE newState, int elapseSinceLastState,
    MediaType type,
    const std::shared_ptr<AgoraRteScene>& scene,
    const std::shared_ptr<AgoraRteStream>& stream) {
  assert(type == MediaType::kAudio || type == MediaType::kVideo);

  bool notify_callback = false;
  PublishState state = PublishState::kFailed;

  if (oldState == rtc::STREAM_PUBLISH_STATE::PUB_STATE_PUBLISHING &&
      newState == rtc::STREAM_PUBLISH_STATE::PUB_STATE_PUBLISHED) {
    state = PublishState::kPublished;
    notify_callback = true;
  }

  if (newState == rtc::STREAM_PUBLISH_STATE::PUB_STATE_NO_PUBLISHED &&
      oldState == rtc::STREAM_PUBLISH_STATE::PUB_STATE_PUBLISHED) {
    state = PublishState::kUnpublished;
    notify_callback = true;
  }

  if (notify_callback) {
    StreamInfo info;
    info.user_id = scene->GetLocalUserInfo().user_id;
    info.stream_id = stream->GetStreamId();

    if (state == PublishState::kPublished) {
      switch (type) {
        case MediaType::kAudio:
          stream->OnAudioConnected();
          break;
        case MediaType::kVideo:
          stream->OnVideoConnected();
          break;
        default:
          break;
      }

      scene->OnLocalStreamStateChanged(info, type, StreamMediaState::kIdle,
                                       StreamMediaState::kStreaming,
                                       StreamStateChangedReason::kPublished);
    } else {
      switch (type) {
        case MediaType::kAudio:
          stream->OnAudioDisconnected();
          break;
        case MediaType::kVideo:
          stream->OnVideoDisconnected();
          break;
        default:
          break;
      }

      scene->OnLocalStreamStateChanged(info, type, StreamMediaState::kStreaming,
                                       StreamMediaState::kIdle,
                                       StreamStateChangedReason::kUnpublished);
    }
  }
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteScene.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteScene::AgoraRteScene(
    std::shared_ptr<AgoraRteStreamFactory> stream_factory,
    const std::string& scene_id, const SceneConfig& config)
    : scene_id_(scene_id), config_(config), stream_factory_(stream_factory) {}

inline int AgoraRteScene::Join(const std::string& user_id,
                                   const std::string& token,
                                   const JoinOptions& option) {
  // Sync user calls
  //
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);

  {
    // Sync between callbacks to change state
    //
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    if (scene_state_ != SceneState::kDisconnected) return -ERR_INVALID_STATE;

    scene_state_ = SceneState::kConnecting;
  }

  user_id_ = user_id;

  auto major_stream = GetMajorStream();

  if (!major_stream) {
    major_stream =
        stream_factory_->CreateMajorStream(shared_from_this(), token, option);
    SetMajorStream(major_stream);
  }

  if (!major_stream || major_stream->Connect() != ERR_OK) {
    // If we failed to connect, no callback will be triggered
    //
    {
      std::lock_guard<std::recursive_mutex> lock(callback_lock_);
      major_stream_object_.reset();
    }
    user_id_ = "";

    {
      std::lock_guard<std::recursive_mutex> lock(callback_lock_);
      if (scene_state_ == SceneState::kConnecting)
        scene_state_ = SceneState::kDisconnected;
    }

    return -ERR_FAILED;
  }

  return ERR_OK;
}

inline void AgoraRteScene::Leave() {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);

  // Leave should never fail
  //

  // Only change the state when we could "disconnect", e.g. we don't want to
  // turn disconneted state to disconnecting
  //
  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    if (scene_state_ == SceneState::kConnecting ||
        scene_state_ == SceneState::kConnected ||
        scene_state_ == SceneState::kReconnecting) {
      scene_state_ = SceneState::kDisconnecting;
      remote_stream_objects_.clear();
    }
  }

  // local_stream_objects_ could only be changed with operation_lock_
  //
  std::for_each(
      local_stream_objects_.begin(), local_stream_objects_.end(),
      [this](const auto& con_pair) { DestroyStream(con_pair.second); });

  local_stream_objects_.clear();

  // Here should be the last reference on major stream
  //
  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    major_stream_object_.reset();
  }
  user_id_ = "";

  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    if (scene_state_ == SceneState::kDisconnecting) {
      scene_state_ = SceneState::kDisconnected;
    }
  }

  // If scene object is deleted after Leave(), no callback will trigged to tell
  // the state change from kDisconnecting to kDisconnected, however, as soon as
  // Leave returned, customer could expect all stream should be disconnected in
  // soon time
  //
}

inline SceneInfo AgoraRteScene::GetSceneInfo() const {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);

  SceneInfo info;
  info.scene_id = scene_id_;
  info.state = scene_state_;
  info.scene_type = config_.scene_type;
  return info;
}

inline UserInfo AgoraRteScene::GetLocalUserInfo() const {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  UserInfo info;
  info.user_id = user_id_;
  info.user_name = "";
  return info;
}

inline std::vector<UserInfo> AgoraRteScene::GetRemoteUsers() const {
  // Don't need operation_lock_ , callback_lock_ is enough
  //
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);

  std::vector<UserInfo> result;
  for (auto& user : remote_users_) {
    result.push_back({user, ""});
  }
  return result;
}

inline std::vector<StreamInfo> AgoraRteScene::GetLocalStreams() const {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);

  std::vector<StreamInfo> result;
  for (auto& stream_pair : local_stream_objects_) {
    auto& stream = stream_pair.second;
    StreamInfo info;
    info.stream_id = stream->GetStreamId();
    info.user_id = user_id_;
    result.push_back(info);
  }

  return result;
}

inline std::vector<StreamInfo> AgoraRteScene::GetRemoteStreams() const {
  // Don't need operation_lock_ , callback_lock_ is enough
  //
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);

  std::vector<StreamInfo> result;
  for (auto& stream_pair : remote_stream_objects_) {
    auto& stream = stream_pair.second;

    result.push_back({stream->GetStreamId(), stream->GetUserId()});
  }

  return result;
}

inline std::vector<StreamInfo> AgoraRteScene::GetRemoteStreamsByUserId(
    const std::string& user_id) const {
  // Don't need operation_lock_ , callback_lock_ is enough
  //
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);

  std::vector<StreamInfo> result;
  for (auto& stream_pair : remote_stream_objects_) {
    auto& stream = stream_pair.second;

    if (stream->GetUserId() == user_id)
      result.push_back({stream->GetStreamId(), stream->GetUserId()});
  }

  return result;
}

inline int AgoraRteScene::CreateOrUpdateStreamCommon(
    const std::string& local_stream_id, const StreamOption& option) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteLocalStream> local_stream =
      GetLocalStream(local_stream_id);

  if (!local_stream) {
    local_stream = CreatLocalStream(local_stream_id, option);
    if (local_stream) return ERR_OK;
  } else {
    return local_stream->UpdateOption(option);
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::CreateOrUpdateRTCStream(
    const std::string& local_stream_id, const RtcStreamOptions& option) {
  return CreateOrUpdateStreamCommon(local_stream_id, option);
}

inline int AgoraRteScene::CreateOrUpdateDirectCDNStream(
    const std::string& local_stream_id, const DirectCdnStreamOptions& option) {
  return CreateOrUpdateStreamCommon(local_stream_id, option);
}

inline int AgoraRteScene::SetAudioEncoderConfiguration(
    const std::string& local_stream_id,
    const AudioEncoderConfiguration& config) {
  std::shared_ptr<AgoraRteLocalStream> stream = GetLocalStream(local_stream_id);

  if (stream) {
    return stream->SetAudioEncoderConfiguration(config);
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::SetVideoEncoderConfiguration(
    const std::string& local_stream_id,
    const VideoEncoderConfiguration& config) {
  std::shared_ptr<AgoraRteLocalStream> stream = GetLocalStream(local_stream_id);

  if (stream) {
    return stream->SetVideoEncoderConfiguration(config);
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::SetBypassCdnTranscoding(
    const std::string& local_stream_id,
    const agora::rtc::LiveTranscoding& transcoding) {
  std::shared_ptr<AgoraRteLocalStream> stream = GetLocalStream(local_stream_id);

  if (stream) {
    return stream->SetBypassCdnTranscoding(transcoding);
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::AddBypassCdnUrl(
    const std::string& local_stream_id, const std::string& target_cdn_url,
    bool transcoding_enabled) {
  std::shared_ptr<AgoraRteLocalStream> stream = GetLocalStream(local_stream_id);

  if (stream) {
    return stream->AddBypassCdnUrl(target_cdn_url, transcoding_enabled);
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::RemoveBypassCdnUrl(
    const std::string& local_stream_id, const std::string& target_cdn_url) {
  std::shared_ptr<AgoraRteLocalStream> stream = GetLocalStream(local_stream_id);

  if (stream) {
    return stream->RemoveBypassCdnUrl(target_cdn_url);
  }

  return -ERR_FAILED;
}

inline bool AgoraRteScene::IsSceneActive() {
  if (scene_state_ == SceneState::kDisconnecting ||
      scene_state_ == SceneState::kDisconnected ||
      scene_state_ == SceneState::kFailed) {
    return false;
  }

  return true;
}

inline std::shared_ptr<AgoraRteMajorStream>
AgoraRteScene::GetMajorStream() {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  return major_stream_object_;
}

inline void AgoraRteScene::SetMajorStream(
    const std::shared_ptr<AgoraRteMajorStream>& stream) {
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  major_stream_object_ = stream;
}

inline void AgoraRteScene::OnRtcStats(const std::string& stream_id,
                                          const rtc::RtcStats& stats) {
  const std::string major_stream_id = GetMajorStream()->GetStreamId();
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);

  rtc_stats_map_[stream_id].push_back(stats);
  if (major_stream_id != stream_id) {
    return;
  }

  SceneStats scene_stats = {};
  scene_stats.duration = stats.duration;
  scene_stats.cpuTotalUsage = stats.cpuTotalUsage;
  scene_stats.cpuAppUsage = stats.cpuAppUsage;
  scene_stats.memoryAppUsageRatio = stats.memoryAppUsageRatio;
  scene_stats.memoryTotalUsageRatio = stats.memoryTotalUsageRatio;
  scene_stats.memoryAppUsageInKbytes = stats.memoryAppUsageInKbytes;

  for (auto& rtc_stats_vec : rtc_stats_map_) {
    for (auto& rtc_stats : rtc_stats_vec.second) {
      scene_stats.txBytes += static_cast<int>(rtc_stats.txBytes);
      scene_stats.rxBytes += static_cast<int>(rtc_stats.rxBytes);
      scene_stats.txAudioBytes += static_cast<int>(rtc_stats.txAudioBytes);
      scene_stats.txVideoBytes += static_cast<int>(rtc_stats.txVideoBytes);
      scene_stats.rxAudioBytes += static_cast<int>(rtc_stats.rxAudioBytes);
      scene_stats.rxVideoBytes += static_cast<int>(rtc_stats.rxVideoBytes);
      scene_stats.txKBitRate += static_cast<int>(rtc_stats.txKBitRate);
      scene_stats.rxKBitRate += static_cast<int>(rtc_stats.rxKBitRate);
      scene_stats.txAudioKBitRate += static_cast<int>(rtc_stats.txAudioKBitRate);
      scene_stats.rxAudioKBitRate += static_cast<int>(rtc_stats.rxAudioKBitRate);
      scene_stats.txVideoKBitRate += static_cast<int>(rtc_stats.txVideoKBitRate);
      scene_stats.rxVideoKBitRate += static_cast<int>(rtc_stats.rxVideoKBitRate);
    }
  }

  rtc_stats_map_.clear();

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
    callback_lock_, event_handlers_,
    [&](const auto& handler) { handler->OnSceneStats(scene_stats); });
}

inline void AgoraRteScene::OnLocalStreamStats(const std::string& stream_id,const LocalStreamStats& stats) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
    callback_lock_, event_handlers_,
    [&](const auto& handler) {
      handler->OnLocalStreamStats(stream_id, stats);
    });
}

inline void AgoraRteScene::OnRemoteStreamStats(const std::string& stream_id, const RemoteStreamStats& stats) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
    callback_lock_, event_handlers_,
    [&](const auto& handler) {
      handler->OnRemoteStreamStats(stream_id, stats);
    });
}

inline std::shared_ptr<AgoraRteLocalStream> AgoraRteScene::CreatLocalStream(
  const std::string& local_stream_id, const StreamOption& option) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);

  auto itr = local_stream_objects_.find(local_stream_id);
  if (itr != local_stream_objects_.end()) {
    return itr->second;
  }

  RTE_LOG_INFO << "Create a new stream: " << local_stream_id;
  auto major_stream = GetMajorStream();
  if (major_stream) {
    auto stream = stream_factory_->CreateLocalStream(shared_from_this(),
                                                     option, local_stream_id);
    if (stream) {
      if (stream->Connect() == ERR_OK) {
        local_stream_objects_[local_stream_id] = stream;

        // Tell remote server our stream is created if required
        //
        major_stream->PushStreamInfo({local_stream_id, user_id_},
                                     InstanceState::kCreated);
        return stream;
      } else {
        // Tell server our stream is destoried if required
        //
        major_stream->PushStreamInfo({local_stream_id, user_id_},
                                     InstanceState::kDestroyed);
      }
    }
  }

  return nullptr;
}

inline std::shared_ptr<AgoraRteLocalStream> AgoraRteScene::GetLocalStream(
    const std::string& local_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  auto itr = local_stream_objects_.find(local_stream_id);
  if (itr != local_stream_objects_.end()) {
    return itr->second;
  }

  return nullptr;
}

inline std::shared_ptr<AgoraRteLocalStream>
AgoraRteScene::RemoveLocalStream(const std::string& local_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteLocalStream> stream;
  auto itr = local_stream_objects_.find(local_stream_id);
  if (itr != local_stream_objects_.end()) {
    stream = itr->second;
    local_stream_objects_.erase(itr);
    return stream;
  }

  return nullptr;
}

template <>
inline int AgoraRteScene::PublishSpecific<AgoraRteRtcVideoTrackBase>(
    const std::shared_ptr<AgoraRteLocalStream>& stream,
    const std::shared_ptr<AgoraRteRtcVideoTrackBase>& track) {
  return stream->PublishLocalVideoTrack(track);
}

template <>
inline int AgoraRteScene::PublishSpecific<AgoraRteRtcAudioTrackBase>(
    const std::shared_ptr<AgoraRteLocalStream>& stream,
    const std::shared_ptr<AgoraRteRtcAudioTrackBase>& track) {
  return stream->PublishLocalAudioTrack(track);
}

template <typename track_type>
inline int AgoraRteScene::PublishCommon(
    std::shared_ptr<track_type> track, const std::string& local_stream_id) {
  int result = -ERR_FAILED;

  RTE_LOG_VERBOSE << " stream id: " << local_stream_id;
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  auto stream = GetLocalStream(local_stream_id);
  if (stream) {
    auto track_impl = AgoraRteUtils::CastToImpl(track);

    assert(track_impl);

    result = track_impl->BeforePublish(stream);

    if (result == ERR_OK) {
      result = PublishSpecific(stream, track_impl);
    }

    track_impl->AfterPublish(result, stream);
  }

  return result;
}

inline int AgoraRteScene::PublishLocalAudioTrack(
    const std::string& local_stream_id,
    std::shared_ptr<IAgoraRteAudioTrack> track) {
  return PublishCommon(track, local_stream_id);
}

inline int AgoraRteScene::PublishLocalVideoTrack(
    const std::string& local_stream_id,
    std::shared_ptr<IAgoraRteVideoTrack> track) {
  return PublishCommon(track, local_stream_id);
}

template <>
inline int AgoraRteScene::UnpublishSpecific<AgoraRteRtcAudioTrackBase>(
    const std::shared_ptr<AgoraRteLocalStream>& stream,
    const std::shared_ptr<AgoraRteRtcAudioTrackBase>& track) {
  return stream->UnpublishLocalAudioTrack(track);
}

template <>
inline int AgoraRteScene::UnpublishSpecific<AgoraRteRtcVideoTrackBase>(
    const std::shared_ptr<AgoraRteLocalStream>& stream,
    const std::shared_ptr<AgoraRteRtcVideoTrackBase>& track) {
  return stream->UnpublishLocalVideoTrack(track);
}

template <typename track_type>
inline int AgoraRteScene::UnpublishCommon(
    std::shared_ptr<track_type> track) {
  int result = -ERR_FAILED;
  std::string local_stream_id = track->GetAttachedStreamId();
  if (!local_stream_id.empty()) {
    RTE_LOG_VERBOSE << " stream id: " << local_stream_id;
    std::lock_guard<std::recursive_mutex> lock(operation_lock_);
    auto stream = GetLocalStream(local_stream_id);

    if (stream) {
      auto track_impl = AgoraRteUtils::CastToImpl(track);

      assert(track_impl);

      result = track_impl->BeforeUnPublish(stream);

      if (result == ERR_OK) {
        result = UnpublishSpecific(stream, track_impl);
      }

      track_impl->AfterUnPublish(result, stream);
    }
  }

  return result;
}

inline int AgoraRteScene::UnpublishLocalAudioTrack(
    std::shared_ptr<IAgoraRteAudioTrack> track) {
  return UnpublishCommon(track);
}

inline int AgoraRteScene::UnpublishLocalVideoTrack(
    std::shared_ptr<IAgoraRteVideoTrack> track) {
  return UnpublishCommon(track);
}

inline int AgoraRteScene::PublishMediaPlayer(
    const std::string& local_stream_id,
    std::shared_ptr<IAgoraRteMediaPlayer> player) {
  int result = -ERR_NOT_READY;
  // Publish video first since one stream only allow one video
  //
  auto video_track =
      std::static_pointer_cast<AgoraRteMediaPlayer>(player)->GetVideoTrack();
  if (video_track) {
    result = PublishLocalVideoTrack(local_stream_id, video_track);
  }

  if (result == ERR_OK) {
    auto audio_track =
        std::static_pointer_cast<AgoraRteMediaPlayer>(player)->GetAudioTrack();
    if (audio_track) {
      result = PublishLocalAudioTrack(local_stream_id, audio_track);
    }

    if (result != ERR_OK && video_track) {
      UnpublishLocalVideoTrack(video_track);
    }
  }

  return result;
}

inline int AgoraRteScene::UnpublishMediaPlayer(
    std::shared_ptr<IAgoraRteMediaPlayer> player) {
  int result1 = -ERR_NOT_READY;
  int result2 = -ERR_NOT_READY;
  auto video_track =
      std::static_pointer_cast<AgoraRteMediaPlayer>(player)->GetVideoTrack();
  if (video_track) {
    result1 = UnpublishLocalVideoTrack(video_track);
  }

  auto audio_track =
      std::static_pointer_cast<AgoraRteMediaPlayer>(player)->GetAudioTrack();
  if (audio_track) {
    result2 = UnpublishLocalAudioTrack(audio_track);
  }

  return result1 == ERR_OK ? result2 : result1;
}

inline std::shared_ptr<AgoraRteRemoteStream> AgoraRteScene::GetRemoteStream(
    const std::string& remote_stream_id) {
  // Don't need operation_lock_ , callback_lock_ is enough
  //
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  auto itr = remote_stream_objects_.find(remote_stream_id);
  if (itr != remote_stream_objects_.end()) {
    return itr->second;
  }
  return nullptr;
}

inline int AgoraRteScene::SubscribeRemoteAudio(
    const std::string& remote_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteRemoteStream> stream =
      GetRemoteStream(remote_stream_id);

  if (stream) {
    return stream->SubscribeRemoteAudio();
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::UnsubscribeRemoteAudio(
    const std::string& remote_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteRemoteStream> stream =
      GetRemoteStream(remote_stream_id);

  if (stream) {
    return stream->UnsubscribeRemoteAudio();
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::SubscribeRemoteVideo(
    const std::string& remote_stream_id, const VideoSubscribeOptions& options) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteRemoteStream> stream =
      GetRemoteStream(remote_stream_id);

  if (stream) {
    return stream->SubscribeRemoteVideo(options);
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::UnsubscribeRemoteVideo(
    const std::string& remote_stream_id) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteRemoteStream> stream =
      GetRemoteStream(remote_stream_id);

  if (stream) {
    return stream->UnsubscribeRemoteVideo();
  }

  return -ERR_FAILED;
}

inline int AgoraRteScene::SetRemoteVideoCanvas(
    const std::string& remote_stream_id, const VideoCanvas& canvas) {
  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  std::shared_ptr<AgoraRteRemoteStream> stream =
      GetRemoteStream(remote_stream_id);

  if (stream) {
    return stream->SetRemoteVideoCanvas(canvas);
  }

  return -ERR_FAILED;
}

inline void AgoraRteScene::RegisterEventHandler(
    std::shared_ptr<IAgoraRteSceneEventHandler> event_handler) {
  UnregisterEventHandler(event_handler);
  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    event_handlers_.push_back(event_handler);
  }
}

inline void AgoraRteScene::UnregisterEventHandler(
    std::shared_ptr<IAgoraRteSceneEventHandler> event_handler) {
  AgoraRteUtils::UnregisterSharedPtrFromContainer(
      callback_lock_, event_handlers_, event_handler);
}

inline void AgoraRteScene::RegisterRemoteVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  UnregisterRemoteVideoFrameObserver(observer);
  std::vector<std::shared_ptr<AgoraRteRemoteStream>> remote_streams;

  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    remote_video_frame_observers_.push_back(observer);
    for (auto& stream_pair : remote_stream_objects_) {
      remote_streams.push_back(stream_pair.second);
    }
  }

  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  for (auto& stream : remote_streams) {
    stream->EnableRemoteVideoObserver();
  }
}

inline void AgoraRteScene::UnregisterRemoteVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  AgoraRteUtils::UnregisterSharedPtrFromContainer(
      callback_lock_, remote_video_frame_observers_, observer);

  std::vector<std::shared_ptr<AgoraRteRemoteStream>> remote_streams;

  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);

    if (remote_video_frame_observers_.empty()) {
      for (auto& stream_pair : remote_stream_objects_) {
        remote_streams.push_back(stream_pair.second);
      }
    }
  }

  std::lock_guard<std::recursive_mutex> lock(operation_lock_);
  for (auto& stream : remote_streams) {
    stream->DisableRemoveVideoObserver();
  }
}

inline void AgoraRteScene::RegisterAudioFrameObserver(
    std::shared_ptr<IAgoraRteAudioFrameObserver> observer,
    const AudioObserverOptions& option) {
  UnregisterAudioFrameObserver(observer);
  {
    {
      std::lock_guard<std::recursive_mutex> lock(callback_lock_);

      audio_observer_option_ = option;

      audio_frame_observers_.push_back(observer);
    }

    auto major_stream = GetMajorStream();

    if (option.local_option.after_mix || option.local_option.after_record ||
        option.local_option.before_playback) {
      std::lock_guard<std::recursive_mutex> lock(operation_lock_);
      if (config_.scene_type == SceneType::kAdhoc) {
        auto stream =
            std::static_pointer_cast<AgoraRteRtcMajorStream>(major_stream);
        stream->UpdateLocalAudioObserver(true, option.local_option);
      }
      if (config_.scene_type == SceneType::kCompatible) {
        auto stream =
            std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                major_stream);
        stream->UpdateLocalAudioObserver(true, option.local_option);
      } else {
        // Add Observer when we can register observers to audio device manager
        //
      }
    }

    if (option.remote_option.after_playback_before_mix) {
      std::lock_guard<std::recursive_mutex> lock(operation_lock_);
      if (config_.scene_type == SceneType::kAdhoc) {
        auto stream =
            std::static_pointer_cast<AgoraRteRtcMajorStream>(major_stream);
        stream->UpdateRemoteAudioObserver(true, option.remote_option);
      }
      if (config_.scene_type == SceneType::kCompatible) {
        auto stream =
            std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                major_stream);
        stream->UpdateRemoteAudioObserver(true, option.remote_option);
      } else {
        // Add Observer when we can register observers to audio device manager
        //
      }
    }
  }
}

inline void AgoraRteScene::UnregisterAudioFrameObserver(
    std::shared_ptr<IAgoraRteAudioFrameObserver> observer) {
  AgoraRteUtils::UnregisterSharedPtrFromContainer(
      callback_lock_, audio_frame_observers_, observer);

  auto major_stream = GetMajorStream();
  std::lock_guard<std::recursive_mutex> lock(callback_lock_);
  {
    if (audio_frame_observers_.empty()) {
      constexpr bool enable = true;
      std::lock_guard<std::recursive_mutex> lock(operation_lock_);
      if (config_.scene_type == SceneType::kAdhoc) {
        auto stream =
            std::static_pointer_cast<AgoraRteRtcMajorStream>(major_stream);
        stream->UpdateLocalAudioObserver(!enable);
        stream->UpdateRemoteAudioObserver(!enable);
      }
      if (config_.scene_type == SceneType::kCompatible) {
        auto stream =
            std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                major_stream);
        stream->UpdateLocalAudioObserver(!enable);
        stream->UpdateRemoteAudioObserver(!enable);
      } else {
        // Add Observer when we can register observers to audio device manager
        //
      }
    }
  }
}

inline int AgoraRteScene::AdjustUserPlaybackSignalVolume(const std::string& remote_stream_id, int volume){
  std::shared_ptr<AgoraRteRemoteStream> stream = GetRemoteStream(remote_stream_id);
  if(stream == nullptr){
    return -ERR_FAILED;
  }

  return stream->AdjustRemoteVolume(volume);
}

inline int AgoraRteScene::GetUserPlaybackSignalVolume(const std::string& remote_stream_id, int* volume){
  std::shared_ptr<AgoraRteRemoteStream> stream = GetRemoteStream(remote_stream_id);
  if(stream == nullptr){
    return -ERR_FAILED;
  }

  return stream->GetRemoteVolume(volume);
}

inline void AgoraRteScene::AddRemoteUser(const std::string& user_id) {
  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);

    // Don't need to add remote user if scene is already closed
    //
    if (!IsSceneActive()) {
      return;
    }

    if (remote_users_.find(user_id) != remote_users_.end()) {
      return;
    }

    remote_users_.insert(user_id);
  }

  std::vector<UserInfo> users;
  users.push_back({user_id, ""});

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_,
      [&users](const auto& handler) { handler->OnRemoteUserJoined(users); });
}

inline void AgoraRteScene::RemoveRemoteUser(const std::string& user_id) {
  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);

    auto itr = remote_users_.find(user_id);

    if (itr == remote_users_.end()) {
      return;
    }

    remote_users_.erase(itr);
  }

  std::vector<UserInfo> users;
  users.push_back({user_id, ""});

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_,
      [&users](const auto& handler) { handler->OnRemoteUserLeft(users); });
}

inline void AgoraRteScene::ChangeSceneState(
    SceneState state, ConnectionChangedReason reason) {
  SceneState old_state = SceneState::kFailed;
  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);

    // Customer could fire disconnect any time, so we need to check state first,
    // If we are disconnecting the connection, we could ignore all states except
    // disconnected
    //
    if (scene_state_ == SceneState::kDisconnecting) {
      switch (state) {
        case SceneState::kDisconnected:
        case SceneState::kFailed:
          break;
        default:
          return;
          break;
      }
    }

    old_state = scene_state_;
    scene_state_ = state;
  }

  auto major_stream = GetMajorStream();

  if (major_stream) {
    switch (state) {
      case SceneState::kConnected:
        major_stream->PushUserInfo(GetLocalUserInfo(), InstanceState::kOnline);
        break;
      case SceneState::kDisconnected:
        major_stream->PushUserInfo(GetLocalUserInfo(), InstanceState::kOffline);
        break;
      default:
        break;
    }
  }

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_,
      [old_state, state, reason](const auto& handler) {
        handler->OnConnectionStateChanged(old_state, state, reason);
      });
}

inline void AgoraRteScene::AddRemoteStream(
    const std::string& remote_stream_id,
    const std::shared_ptr<AgoraRteRemoteStream>& stream) {
  bool notify_event = false;

  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);

    // Don't need to add remote stream if scene is already closed
    //
    if (!IsSceneActive()) {
      return;
    }

    auto itr = remote_stream_objects_.find(remote_stream_id);
    if (itr == remote_stream_objects_.end()) {
      remote_stream_objects_[remote_stream_id] = stream;
      if (!remote_video_frame_observers_.empty()) {
        stream->EnableRemoteVideoObserver();
      }

      if (audio_observer_option_.remote_option.after_playback_before_mix) {
        stream->EnableRemoteAudioObserver(audio_observer_option_.remote_option);
      }

      notify_event = true;
    }
  }

  if (notify_event) {
    std::vector<StreamInfo> infos;
    infos.push_back({stream->GetStreamId(), stream->GetUserId()});

    AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
        callback_lock_, event_handlers_,
        [&infos](const auto& handler) { handler->OnRemoteStreamAdded(infos); });
  }
}

inline void AgoraRteScene::RemoveRemoteStream(
    const std::string& remote_stream_id) {
  bool notify_event = false;
  std::vector<StreamInfo> infos;

  {
    std::lock_guard<std::recursive_mutex> lock(callback_lock_);
    auto itr = remote_stream_objects_.find(remote_stream_id);
    if (itr != remote_stream_objects_.end()) {
      infos.push_back({itr->second->GetStreamId(), itr->second->GetUserId()});
      remote_stream_objects_.erase(itr);
      notify_event = true;
    }
  }

  if (notify_event) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
        callback_lock_, event_handlers_, [&infos](const auto& handler) {
          handler->OnRemoteStreamRemoved(infos);
        });
  }
}

inline void AgoraRteScene::OnLocalStreamStateChanged(
    const StreamInfo& stream, MediaType media_type, StreamMediaState old_state,
    StreamMediaState new_state, StreamStateChangedReason reason) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        handler->OnLocalStreamStateChanged(stream, media_type, old_state,
                                           new_state, reason);
      });

  auto major_stream = GetMajorStream();
  if (major_stream) {
    switch (new_state) {
      case StreamMediaState::kStreaming:
        major_stream->PushMediaInfo(stream, media_type, InstanceState::kOnline);
        break;
      case StreamMediaState::kIdle:
        major_stream->PushMediaInfo(stream, media_type,
                                    InstanceState::kOffline);
        break;
      default:
        break;
    }
  }
}

inline void AgoraRteScene::OnRemoteStreamStateChanged(
    const StreamInfo& stream, MediaType media_type, StreamMediaState old_state,
    StreamMediaState new_state, StreamStateChangedReason reason) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        handler->OnRemoteStreamStateChanged(stream, media_type, old_state,
                                            new_state, reason);
      });
}

inline void AgoraRteScene::OnAudioVolumeIndication(
    const std::vector<AudioVolumeInfo>& speakers, int totalVolume) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        handler->OnAudioVolumeIndication(speakers, totalVolume);
      });
}

inline void AgoraRteScene::OnSceneTokenWillExpired(
    const std::string& token) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        handler->OnSceneTokenWillExpired(scene_id_, token);
      });
}

inline void AgoraRteScene::OnSceneTokenExpired() {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_,
      [&](const auto& handler) { handler->OnSceneTokenExpired(scene_id_); });
}

inline void AgoraRteScene::OnStreamTokenWillExpire(
    const std::string& stream_id, const std::string& token) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        handler->OnStreamTokenWillExpire(stream_id, token);
      });
}

inline void AgoraRteScene::OnStreamTokenExpired(
    const std::string& stream_id) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_,
      [&](const auto& handler) { handler->OnStreamTokenExpired(stream_id); });
}

inline void AgoraRteScene::OnCdnStateChanged(
    const std::string& stream_id, const char* url,
    rtc::RTMP_STREAM_PUBLISH_STATE state,
    rtc::RTMP_STREAM_PUBLISH_ERROR errCode) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        std::string url_str(url);
        int state_value = static_cast<int>(state);
        int err_value = static_cast<int>(errCode);
        handler->OnBypassCdnStateChanged(
            stream_id, url_str,
            static_cast<CDNBYPASS_STREAM_PUBLISH_STATE>(state_value),
            static_cast<CDNBYPASS_STREAM_PUBLISH_ERROR>(err_value));
      });
}

inline void AgoraRteScene::OnCdnPublished(const std::string& stream_id,
                                              const char* url, int error) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        std::string url_str(url);
        handler->OnBypassCdnPublished(
            stream_id, url, static_cast<CDNBYPASS_STREAM_PUBLISH_ERROR>(error));
      });
}

inline void AgoraRteScene::OnCdnUnpublished(const std::string& stream_id,
                                                const char* url) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        std::string url_str(url);
        handler->OnBypassCdnUnpublished(stream_id, url);
      });
}

inline void AgoraRteScene::OnCdnTranscodingUpdated(
    const std::string& stream_id) {
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteSceneEventHandler>(
      callback_lock_, event_handlers_, [&](const auto& handler) {
        handler->OnBypassTranscodingUpdated(stream_id);
      });
}

inline void AgoraRteScene::OnRemoteVideoFrame(
    const std::string stream_id, const media::base::VideoFrame& frame) {
  // We could miss the obsever in race condition, but it hurts nothing
  //
  if (!remote_video_frame_observers_.empty()) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteVideoFrameObserver>(
        callback_lock_, remote_video_frame_observers_,
        [&](const auto& handler) { handler->OnFrame(stream_id, frame); });
  }
}

inline void AgoraRteScene::OnRecordAudioFrame(AudioFrame& audioFrame) {
  if (!audio_frame_observers_.empty()) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteAudioFrameObserver>(
        callback_lock_, audio_frame_observers_,
        [&](const auto& handler) { handler->OnRecordAudioFrame(audioFrame); });
  }
}

inline void AgoraRteScene::OnPlaybackAudioFrame(AudioFrame& audioFrame) {
  if (!audio_frame_observers_.empty()) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteAudioFrameObserver>(
        callback_lock_, audio_frame_observers_, [&](const auto& handler) {
          handler->OnPlaybackAudioFrame(audioFrame);
        });
  }
}

inline void AgoraRteScene::OnMixedAudioFrame(AudioFrame& audioFrame) {
  if (!audio_frame_observers_.empty()) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteAudioFrameObserver>(
        callback_lock_, audio_frame_observers_,
        [&](const auto& handler) { handler->OnMixedAudioFrame(audioFrame); });
  }
}

inline void AgoraRteScene::OnPlaybackAudioFrameBeforeMixing(
    const std::string& stream_id, AudioFrame& audioFrame) {
  if (!audio_frame_observers_.empty()) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteAudioFrameObserver>(
        callback_lock_, audio_frame_observers_, [&](const auto& handler) {
          handler->OnPlaybackAudioFrameBeforeMixing(stream_id, audioFrame);
        });
  }
}

inline int AgoraRteScene::DestroyStream(
    const std::string& local_stream_id) {
  std::shared_ptr<AgoraRteLocalStream> stream;
  stream = RemoveLocalStream(local_stream_id);
  if (stream) {
    DestroyStream(stream);
  }

  return ERR_OK;
}

inline void AgoraRteScene::DestroyStream(
    const std::shared_ptr<AgoraRteLocalStream>& local_stream) {
  const std::string& stream_id = local_stream->GetStreamId();

  auto major_stream = GetMajorStream();
  if (major_stream) {
    major_stream->PushStreamInfo({stream_id, user_id_},
                                 InstanceState::kDestroyed);
  }

  local_stream->Disconnect();
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteScreenVideoTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteScreenVideoTrack::AgoraRteScreenVideoTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  track_impl_ = std::make_shared<AgoraRteVideoTrackImpl>(rtc_service);

  auto media_node_factory = track_impl_->GetMediaNodeFactory();
  capture_ = media_node_factory->createScreenCapturer();

  auto track = rtc_service->createScreenVideoTrack(capture_);

  track_impl_->SetTrack(
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalVideoTrack>(
          track));
}

inline int AgoraRteScreenVideoTrack::SetPreviewCanvas(
    const VideoCanvas& canvas) {
  return track_impl_->SetPreviewCanvas(canvas);
}

inline SourceType AgoraRteScreenVideoTrack::GetSourceType() {
  return SourceType::kVideo_Screen;
}

inline void AgoraRteScreenVideoTrack::RegisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->RegisterLocalVideoFrameObserver(observer);
}

inline void AgoraRteScreenVideoTrack::UnregisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->UnregisterLocalVideoFrameObserver(observer);
}

inline int AgoraRteScreenVideoTrack::EnableVideoFilter(
    const std::string& id, bool enable) {
  return track_impl_->GetVideoTrack()->enableVideoFilter(id.c_str(), enable);
}

inline int AgoraRteScreenVideoTrack::SetFilterProperty(
    const std::string& id, const std::string& key,
    const std::string& json_value) {
  return track_impl_->GetVideoTrack()->setFilterProperty(
      id.c_str(), key.c_str(), json_value.c_str());
}

inline std::string AgoraRteScreenVideoTrack::GetFilterProperty(
    const std::string& id, const std::string& key) {
  char buffer[1024];

  if (track_impl_->GetVideoTrack()->getFilterProperty(id.c_str(), key.c_str(),
                                                      buffer, 1024) == ERR_OK) {
    return buffer;
  }

  return "";
}

inline void AgoraRteScreenVideoTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

inline const std::string& AgoraRteScreenVideoTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

#if RTE_WIN
inline int AgoraRteScreenVideoTrack::StartCaptureScreen(
    const Rectangle& screenRect, const Rectangle& regionRect) {
  int result = capture_->initWithScreenRect(screenRect, regionRect);
  if (result == ERR_OK) {
    return track_impl_->Start();
  }

  return result;
}

inline int AgoraRteScreenVideoTrack::StartCaptureWindow(
    view_t windowId, const Rectangle& regionRect) {
  int result = capture_->initWithWindowId(windowId, regionRect);
  if (result == ERR_OK) {
    return track_impl_->Start();
  }

  return result;
}
#elif RTE_MAC
inline int AgoraRteScreenVideoTrack::StartCaptureScreen(
    view_t displayId, const Rectangle& regionRect) {
  int result = capture_->initWithDisplayId(displayId, regionRect);
  if (result == ERR_OK) {
    return track_impl_->Start();
  }

  return result;
}

inline int AgoraRteScreenVideoTrack::StartCaptureWindow(
    view_t windowId, const Rectangle& regionRect) {
  int result = capture_->initWithWindowId(windowId, regionRect);
  if (result == ERR_OK) {
    return track_impl_->Start();
  }

  return result;
}
#elif RTE_ANDROID
inline int AgoraRteScreenVideoTrack::StartCaptureScreen(
    void* data, const VideoDimensions& dimensions) {
  int result =
      capture_->initWithMediaProjectionPermissionResultData(data, dimensions);
  if (result == ERR_OK) {
    return track_impl_->Start();
  }

  return result;
}
#elif RTE_IPHONE
inline int AgoraRteScreenVideoTrack::StartCaptureScreen() {
  return track_impl_->Start();
}
#endif

inline void AgoraRteScreenVideoTrack::StopCapture() { track_impl_->Stop(); }

#if RTE_WIN || RTE_MAC

inline int AgoraRteScreenVideoTrack::SetContentHint(
    VIDEO_CONTENT_HINT contentHint) {
  return capture_->setContentHint(contentHint);
}

inline int AgoraRteScreenVideoTrack::UpdateScreenCaptureRegion(
    const Rectangle& regionRect) {
  return capture_->updateScreenCaptureRegion(regionRect);
}
#endif

inline std::shared_ptr<agora::rtc::ILocalVideoTrack>
AgoraRteScreenVideoTrack::GetRtcVideoTrack() const {
  return track_impl_->GetVideoTrack();
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteSdk.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include <memory>
#include <mutex>

#include "IAgoraRteScene.h"

namespace agora {
namespace rte {

inline int AgoraRteSDK::Init(const SdkProfile& profile,
                                 bool enable_rtc_compatible_mode) {
  std::lock_guard<std::mutex> lock(GetLock());

  return GetSdkInstance()->SetProfileInternal(profile,
                                              enable_rtc_compatible_mode);
}

inline void AgoraRteSDK::Deinit() {
  std::lock_guard<std::mutex> lock(GetLock());

  return GetSdkInstance()->DeinitInternal();
}

inline std::shared_ptr<IAgoraRteScene> AgoraRteSDK::CreateRteScene(
    const std::string& scene_id, const SceneConfig& config) {
  std::lock_guard<std::mutex> lock(GetLock());

  auto factory = GetSdkInstance()->GetStreamFactory();
  if (GetSdkInstance()->IsCompatibleModeEnabled() !=
      (config.scene_type == SceneType::kCompatible)) {
    return nullptr;
  }

  return factory ? std::make_shared<AgoraRteScene>(factory, scene_id, config)
                 : nullptr;
}

inline void AgoraRteSDK::SetAgoraServiceCreator(
    AgoraServiceCreatorFunctionPtr func) {
  std::lock_guard<std::mutex> lock(GetLock());

  GetSdkInstance()->SetServiceCreator(func);
}

inline std::shared_ptr<IAgoraRteMediaFactory>
AgoraRteSDK::GetRteMediaFactory() {
  std::lock_guard<std::mutex> lock(GetLock());

  return GetSdkInstance()->GetRteMediaFactoryInternal();
}

inline std::shared_ptr<agora::base::IAgoraService>
AgoraRteSDK::GetRtcService() {
  std::lock_guard<std::mutex> lock(GetLock());

  return GetSdkInstance()->GetRtcServiceInternal();
}

inline int AgoraRteSDK::GetProfile(SdkProfile& out_profile) {
  std::lock_guard<std::mutex> lock(GetLock());
  return GetSdkInstance()->GetProfileInternal(out_profile);
}

#if RTE_WIN || RTE_MAC
inline std::shared_ptr<IAgoraRteAudioDeviceManager>
AgoraRteSDK::GetAudioDeviceManager() {
  std::lock_guard<std::mutex> lock(GetLock());

  return GetSdkInstance()->GetRteAudioDeviceManager();
}

inline std::shared_ptr<IAgoraRteVideoDeviceManager>
AgoraRteSDK::GetVideoDeviceManager() {
  std::lock_guard<std::mutex> lock(GetLock());

  return GetSdkInstance()->GetRteVideoDeviceManager();
}

inline std::shared_ptr<IAgoraRteAudioDeviceManager>
AgoraRteSDK::GetRteAudioDeviceManager() {
  if (audio_device_mgr_) {
    return audio_device_mgr_;
  }

  auto rtc_service = GetRtcServiceInternal();
  audio_device_mgr_ =
      std::make_shared<AgoraRteAudioDeviceManager>(rtc_service.get(), nullptr);
  return audio_device_mgr_;
}

inline std::shared_ptr<IAgoraRteVideoDeviceManager>
AgoraRteSDK::GetRteVideoDeviceManager() {
  if (video_device_mgr_) {
    return video_device_mgr_;
  }

  auto rtc_service = GetRtcServiceInternal();
  video_device_mgr_ = std::make_shared<AgoraRteVideoDeviceManager>(rtc_service);
  return video_device_mgr_;
}
#endif

inline std::shared_ptr<AgoraRteSDK> AgoraRteSDK::GetSdkInstance() {
  static auto internal_sdk = std::shared_ptr<AgoraRteSDK>(new AgoraRteSDK());
  return internal_sdk;
}

inline std::mutex& AgoraRteSDK::GetLock() {
  static std::mutex lock;
  return lock;
}

inline std::shared_ptr<IAgoraRteMediaFactory>
AgoraRteSDK::GetRteMediaFactoryInternal() {
  if (media_factory_) {
    return media_factory_;
  }

  auto rtc_service = GetRtcServiceInternal();
  media_factory_ = std::make_shared<AgoraRteMediaFactory>(rtc_service);
  return media_factory_;
}

inline void AgoraRteSDK::SetServiceCreator(
    AgoraServiceCreatorFunctionPtr func) {
  creator_ = func;
}

inline std::shared_ptr<AgoraRteStreamFactory>
AgoraRteSDK::GetStreamFactory() {
  if (stream_factory_) {
    return stream_factory_;
  }

  auto rtc_service = GetRtcServiceInternal();
  if (!profile_.appid.empty() && rtc_service_) {
    return stream_factory_ = std::make_shared<AgoraRteStreamFactory>(
               profile_.appid, rtc_service);
  }

  return nullptr;
}

inline int AgoraRteSDK::SetProfileInternal(
    const SdkProfile& profile, bool enable_rtc_compatible_mode) {
  if (profile.appid.empty()) {
    return -ERR_INVALID_APP_ID;
  }
  profile_ = profile;
  enable_rtc_compatible_mode_ = enable_rtc_compatible_mode;
  return ERR_OK;
}

inline int AgoraRteSDK::GetProfileInternal(SdkProfile& out_profile) {
  out_profile = profile_;
  return ERR_OK;
}

inline std::shared_ptr<agora::base::IAgoraService>
AgoraRteSDK::GetRtcServiceInternal() {
  if (rtc_service_) {
    return rtc_service_;
  }

  agora::base::IAgoraService* created_service =
      creator_ ? creator_() : createAgoraService();
  if (!created_service) {
    return nullptr;
  }

  rtc_service_ = std::shared_ptr<agora::base::IAgoraService>(
      static_cast<agora::base::IAgoraService*>(created_service),
      [](agora::base::IAgoraService* p) {
        if (p) {
          p->release();
        }
      });

  agora::base::AgoraServiceConfiguration svc_cfg_ex;
  svc_cfg_ex.appId = profile_.appid.c_str();

#if RTE_ANDROID
  svc_cfg_ex.context = profile_.app_context;
#else
  svc_cfg_ex.context = nullptr;
#endif

  svc_cfg_ex.logConfig.filePath = profile_.log_config.file_path.c_str();
  svc_cfg_ex.logConfig.fileSizeInKB = profile_.log_config.file_size_in_kb;
  svc_cfg_ex.logConfig.level = profile_.log_config.level;
  svc_cfg_ex.useStringUid = !enable_rtc_compatible_mode_;
  svc_cfg_ex.enableVideo = true;
  svc_cfg_ex.enableAudioDevice = true;

  if (rtc_service_->initialize(svc_cfg_ex) != agora::ERR_OK) {
    rtc_service_ = nullptr;
  }

  return rtc_service_;
}

inline void AgoraRteSDK::DeinitInternal() {
  rtc_service_ = nullptr;
  media_factory_ = nullptr;
  stream_factory_ = nullptr;

#if RTE_WIN || RTE_MAC
  video_device_mgr_ = nullptr;
  audio_device_mgr_ = nullptr;
#endif
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteStreamFactory.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {
inline AgoraRteStreamFactory::AgoraRteStreamFactory(
    const std::string& appid,
    std::shared_ptr<agora::base::IAgoraService> rtc_service)
    : appid_(appid), rtc_service_(rtc_service) {}

inline std::shared_ptr<AgoraRteMajorStream>
AgoraRteStreamFactory::CreateMajorStream(std::shared_ptr<AgoraRteScene> scene,
                                         const std::string& token,
                                         const JoinOptions& option) {
  // Here we don't check permission from remote server, as we assume everyone
  // could login
  //
  switch (scene->GetSceneInfo().scene_type) {
    case SceneType::kAdhoc: {
      auto stream = std::make_shared<AgoraRteRtcMajorStream>(
          scene, rtc_service_, token, option);
      return stream;
    }
    case SceneType::kCompatible: {
      auto stream = std::make_shared<AgoraRteRtcCompatibleMajorStream>(
          scene, rtc_service_, token, option);
      return stream;
    }
    default:
      break;
  }
  return nullptr;
}

inline std::shared_ptr<AgoraRteLocalStream>
AgoraRteStreamFactory::CreateLocalStream(std::shared_ptr<AgoraRteScene> scene,
                                         const StreamOption& option,
                                         const std::string& stream_id) {
  auto major_stream = scene->GetMajorStream();

  if (!major_stream) return nullptr;

  // Check whether we are allowed to create the stream from remote server (if
  // there is)
  //
  int result = major_stream->PushStreamInfo(
      {stream_id, scene->GetLocalUserInfo().user_id}, InstanceState::kCreating);
  if (result == ERR_OK) {
    // We are allowed to create stream
    //
    switch (option.type) {
      case StreamType::kRtcStream: {
        auto& rtc_option = static_cast<const RtcStreamOptions&>(option);

        switch (scene->GetSceneInfo().scene_type) {
          case SceneType::kAdhoc: {
            auto stream = std::make_shared<AgoraRteRtcLocalStream>(
                scene, rtc_service_, stream_id, rtc_option.token);
            return stream;
          }
          case SceneType::kCompatible: {
            auto compatible_major =
                std::static_pointer_cast<AgoraRteRtcCompatibleMajorStream>(
                    major_stream);
            auto stream = std::make_shared<AgoraRteRtcCompatibleLocalStream>(
                scene, compatible_major);
            return stream;
          }
          default:
            return nullptr;
        }
      }
      case StreamType::kCdnStream: {
        auto& cnd_option = static_cast<const DirectCdnStreamOptions&>(option);
        auto stream = std::make_shared<AgoraRteCdnLocalStream>(
            scene, rtc_service_, stream_id, cnd_option.url);
        return stream;
      }
      default:
        break;
    }
  }

  return nullptr;
}
}  // namespace rte
}  // namespace agora
// ======================= AgoraRteStreamingSource.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteStreamingSource::AgoraRteStreamingSource(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  rtc_service_ = rtc_service;

  //
  // Create rtc object
  //
  rtc_node_factory_ = rtc_service_->createMediaNodeFactory();
  rtc_streaming_source_ = rtc_node_factory_->createMediaStreamingSource();
  if (rtc_streaming_source_ == nullptr) {
    RTE_LOG_VERBOSE << "<AgoraRteStreamingSource> streaming_source is NULL";
    return;
  }
  auto rtc_audio_track =
      rtc_service_->createMediaStreamingAudioTrack(rtc_streaming_source_);
  auto rtc_video_track =
      rtc_service_->createMediaStreamingVideoTrack(rtc_streaming_source_);

  //
  // Create rte object
  //
  rte_audio_track_ = std::make_shared<AgoraRteWrapperAudioTrack>(
      rtc_service,
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalAudioTrack>(
          rtc_audio_track));
  rte_video_track_ = std::make_shared<AgoraRteWrapperVideoTrack>(
      rtc_service,
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::ILocalVideoTrack>(
          rtc_video_track));

  rtc_streaming_source_->registerObserver(this);
}

inline AgoraRteStreamingSource::~AgoraRteStreamingSource() {
  if (rtc_streaming_source_.get() != nullptr) {
    rtc_streaming_source_->unregisterObserver(this);
    rtc_streaming_source_.reset();
  }
  rte_audio_track_.reset();
  rte_audio_track_.reset();
  observer_list_.clear();

  if (play_list_ != nullptr) {
    play_list_->SetCurrFileById(INVALID_RTE_FILE_ID);
    play_list_ = nullptr;
  }
}

//////////////////////////////////////////////////////////////////////////
///////////////// Override Methods of IAgoraRteStreamingSource ////////////
///////////////////////////////////////////////////////////////////////////

inline std::shared_ptr<IAgoraRtePlayList>
AgoraRteStreamingSource::CreatePlayList() {
  return std::make_shared<AgoraRtePlayList>();
}

inline std::shared_ptr<IAgoraRteVideoTrack>
AgoraRteStreamingSource::GetRteVideoTrack() {
  return static_cast<std::shared_ptr<IAgoraRteVideoTrack>>(rte_video_track_);
}

inline std::shared_ptr<IAgoraRteAudioTrack>
AgoraRteStreamingSource::GetRteAudioTrack() {
  return static_cast<std::shared_ptr<IAgoraRteAudioTrack>>(rte_audio_track_);
}

inline int AgoraRteStreamingSource::Open(const char* url, int64_t start_pos,
                                             bool auto_play /* = true */) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  if (play_list_ != nullptr) {  // previous list have not closed
    RTE_LOG_ERROR
        << "<AgoraRteStreamingSource.Open> there is a play list exist";
    return -agora::ERR_INVALID_STATE;
  }

  current_file_.Reset();
  current_file_.file_id = 1;
  current_file_.file_url = url;
  current_file_.index = 0;
  first_file_id_ = current_file_.file_id;

  auto_play_for_open_ = auto_play;
  int ret = rtc_streaming_source_->open(url, start_pos, auto_play);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::Open(
    std::shared_ptr<IAgoraRtePlayList> in_play_list, int64_t start_pos,
    bool auto_play /* = true */) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  if (play_list_ != nullptr) {  // previous list have not closed
    RTE_LOG_ERROR
        << "<AgoraRteStreamingSource.Open> there is a play list exist";
    return -agora::ERR_INVALID_STATE;
  }
  if (in_play_list == nullptr) {
    return -agora::ERR_INVALID_ARGUMENT;
  }
  if (in_play_list->GetFileCount() <= 0) {
    return -agora::ERR_INVALID_ARGUMENT;
  }

  play_list_ = std::static_pointer_cast<AgoraRtePlayList>(in_play_list);

  current_file_.Reset();
  play_list_->GetFirstFileInfo(current_file_);
  play_list_->SetCurrFileById(current_file_.file_id);
  assert(current_file_.IsValid());
  first_file_id_ = current_file_.file_id;
  list_played_count_ = 0;

  auto_play_for_open_ = auto_play;
  int ret = rtc_streaming_source_->open(current_file_.file_url.c_str(),
                                        start_pos, auto_play);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::Close() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->close();

  if (play_list_ != nullptr) {
    play_list_->SetCurrFileById(INVALID_RTE_FILE_ID);
    play_list_ = nullptr;
  }

  return ConvertErrCode(ret);
}

inline bool AgoraRteStreamingSource::isVideoValid() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  bool video_valid = rtc_streaming_source_->isVideoValid();
  return video_valid;
}

inline bool AgoraRteStreamingSource::isAudioValid() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  bool audio_valid = rtc_streaming_source_->isAudioValid();
  return audio_valid;
}

inline int AgoraRteStreamingSource::GetDuration(int64_t& duration) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->getDuration(duration);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::GetStreamCount(int64_t& count) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->getStreamCount(count);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::GetStreamInfo(
    int64_t index, media::base::PlayerStreamInfo* out_info) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->getStreamInfo(index, out_info);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::SetLoopCount(int64_t loop_count) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
  setting_loop_count = loop_count;
  list_played_count_ = 0;

  if (play_list_ == nullptr) {  // Open only one file
    return rtc_streaming_source_->setLoopCount(loop_count);
  }

  return agora::ERR_OK;
}

inline int AgoraRteStreamingSource::Play() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->play();
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::Pause() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->pause();
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::Stop() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->stop();
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::Seek(int64_t new_pos) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  if (play_list_ != nullptr) {
    RteFileInfo curr_file_info;
    play_list_->GetCurrentFileInfo(curr_file_info);
    if ((curr_file_info.duration > 0) && (new_pos >= curr_file_info.duration)) {
      RTE_LOG_ERROR
          << "<AgoraRteStreamingSource.Seek> pos is more than duration";
      return -agora::ERR_INVALID_ARGUMENT;
    }
  }

  int ret = rtc_streaming_source_->seek(new_pos);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::SeekToPrev(int64_t pos) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
  if (play_list_ == nullptr) {  // It don't support for only one file
    return -agora::ERR_INVALID_STATE;
  }
  if (play_list_->CurrentIsFirstFile()) {
    RTE_LOG_ERROR
        << "<AgoraRteStreamingSource.SeekToPrev> current already is first file";
    return -agora::ERR_INVALID_STATE;
  }
  RteFileInfoSharePtr prev_file_ptr = play_list_->FindPrevFile(false);
  assert(prev_file_ptr != nullptr);
  if ((prev_file_ptr->duration > 0) && (pos >= prev_file_ptr->duration)) {
    RTE_LOG_ERROR
        << "<AgoraRteMediaPlayer.SeekToPrev> pos is more than duration";
    return -agora::ERR_INVALID_ARGUMENT;
  }

  // close current file
  int ret = rtc_streaming_source_->close();

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to close source, error : " << ret;
    return ret;
  }

  // switch to previous file
  ret = play_list_->MoveCurrentToPrev(false);

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to move to previous file, error : " << ret;
    return ret;
  }

  current_file_.Reset();
  play_list_->GetCurrentFileInfo(current_file_);
  assert(current_file_.IsValid());

  // start play previous file
  auto_play_for_open_ = true;
  ret = rtc_streaming_source_->open(current_file_.file_url.c_str(), pos, true);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::SeekToNext(int64_t pos) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
  if (play_list_ == nullptr) {
    return -agora::ERR_INVALID_STATE;
  }
  if (play_list_->CurrentIsLastFile()) {
    RTE_LOG_ERROR
        << "<AgoraRteStreamingSource.SeekToNext> current already is last file";
    return -agora::ERR_INVALID_STATE;
  }
  RteFileInfoSharePtr next_file_ptr = play_list_->FindNextFile(false);
  assert(next_file_ptr != nullptr);
  if ((next_file_ptr->duration > 0) && (pos >= next_file_ptr->duration)) {
    RTE_LOG_ERROR
        << "<AgoraRteStreamingSource.SeekToNext> pos is more than duration";
    return -agora::ERR_INVALID_ARGUMENT;
  }

  // close current file
  int ret = rtc_streaming_source_->close();
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to close source, error : " << ret;
    return ret;
  }

  // switch to next file
  ret = play_list_->MoveCurrentToNext(false);

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to move to next file, error : " << ret;
    return ret;
  }

  current_file_.Reset();
  play_list_->GetCurrentFileInfo(current_file_);
  assert(current_file_.IsValid());

  // start play next file
  auto_play_for_open_ = true;
  ret = rtc_streaming_source_->open(current_file_.file_url.c_str(), pos, true);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::SeekToFile(int32_t file_id,
                                                   int64_t pos) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
  if (play_list_ == nullptr) {
    return -agora::ERR_INVALID_STATE;
  }

  if (file_id == current_file_.file_id) {
    // Seek in current file
    int ret_inner_seek = rtc_streaming_source_->seek(pos);
    return ConvertErrCode(ret_inner_seek);
  }

  // close current file
  int ret = rtc_streaming_source_->close();

  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to close source, error : " << ret;
    return ret;
  }

  // switch to new file
  ret = play_list_->SetCurrFileById(file_id);
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "<AgoraRteMediaPlayer.SeekToFile> failed, file_id= "
                  << file_id << ", pos= " << pos;
    return -agora::ERR_INVALID_ARGUMENT;
  }
  current_file_.Reset();
  play_list_->GetCurrentFileInfo(current_file_);
  assert(current_file_.IsValid());

  // start play new file
  auto_play_for_open_ = true;
  ret = rtc_streaming_source_->open(current_file_.file_url.c_str(), pos, true);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::GetStreamingSourceStatus(
    RteStreamingSourceStatus& out_source_status) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  out_source_status.curr_file_id = current_file_.file_id;
  out_source_status.curr_file_url = current_file_.file_url;
  out_source_status.curr_file_index = current_file_.index;
  out_source_status.curr_file_duration = current_file_.duration;
  out_source_status.curr_file_begin_time = current_file_.begin_time;
  out_source_status.source_state = rtc_streaming_source_->getCurrState();
  rtc_streaming_source_->getCurrPosition(out_source_status.progress_pos);
  return agora::ERR_OK;
}

inline int AgoraRteStreamingSource::GetCurrPosition(int64_t& pos) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->getCurrPosition(pos);
  return ConvertErrCode(ret);
}

inline agora::rtc::STREAMING_SRC_STATE
AgoraRteStreamingSource::GetCurrState() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  agora::rtc::STREAMING_SRC_STATE state = rtc_streaming_source_->getCurrState();
  return state;
}

inline int AgoraRteStreamingSource::AppendSeiData(
    const agora::rtc::InputSeiData& inSeiData) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  int ret = rtc_streaming_source_->appendSeiData(inSeiData);
  return ConvertErrCode(ret);
}

inline int AgoraRteStreamingSource::RegisterObserver(
    std::shared_ptr<IAgoraRteStreamingSourceObserver> observer) {
  UnregisterObserver(observer);

  {
    std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
    observer_list_.push_back(observer);
  }
  return agora::rtc::STREAMING_SRC_ERR_NONE;
}

inline int AgoraRteStreamingSource::UnregisterObserver(
    std::shared_ptr<IAgoraRteStreamingSourceObserver> observer) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);

  AgoraRteUtils::UnregisterSharedPtrFromContainer(streaming_source_lock_,
                                                  observer_list_, observer);
  return agora::rtc::STREAMING_SRC_ERR_NONE;
}

inline void AgoraRteStreamingSource::GetCurrFileInfo(
    RteFileInfo& out_current_file) {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
  current_file_.CloneTo(out_current_file);
}

inline int AgoraRteStreamingSource::ProcessOneMediaCompleted() {
  std::lock_guard<std::recursive_mutex> _(streaming_source_lock_);
  int ret = agora::ERR_OK;

  if (play_list_ == nullptr) {  // Only play one file, Ignore this handle
    return agora::ERR_OK;
  }

  // Close current playing file
  ret = rtc_streaming_source_->close();
  if (ret != agora::ERR_OK) {
    RTE_LOG_ERROR << "Failed to close source, error : " << ret;
    return ret;
  }

  if (!play_list_->CurrentIsLastFile()) {
    // switch to next file
    RTE_LOG_ERROR << "<AgoraRteStreamingSource.ProcessOneMediaCompleted> "
                     "switch to next file...";
    ret = play_list_->MoveCurrentToNext(false);
    if (ret != agora::ERR_OK) {
      RTE_LOG_ERROR
          << "<AgoraRteStreamingSource.ProcessOneMediaCompleted> failed, ret= "
          << ret;
      return -agora::ERR_INVALID_STATE;
    }

    // start play next file
    play_list_->GetCurrentFileInfo(current_file_);
    auto_play_for_open_ = true;
    ret = rtc_streaming_source_->open(current_file_.file_url.c_str(), 0, true);
    return ConvertErrCode(ret);
  }

  list_played_count_++;
  if (list_played_count_ >=
      setting_loop_count)  // all file loops are finished finished
  {
    play_list_->SetCurrFileById(INVALID_RTE_FILE_ID);  // reset current file
    current_file_.Reset();
    RTE_LOG_ERROR << "<AgoraRteStreamingSource.ProcessOneMediaCompleted> all "
                     "loop finished";
    return -agora::ERR_CANCELED;

  } else  // Repeat play first file
  {
    RTE_LOG_ERROR << "<AgoraRteStreamingSource.ProcessOneMediaCompleted> "
                     "switch to first file...";

    // Switch to first file
    play_list_->GetFirstFileInfo(current_file_);
    play_list_->SetCurrFileById(current_file_.file_id);

    // start play first file
    auto_play_for_open_ = true;
    ret = rtc_streaming_source_->open(current_file_.file_url.c_str(), 0, true);
    return ConvertErrCode(ret);
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
/////////////// Override Methods of IMediaStreamingSourceObserver ////////////
///////////////////////////////////////////////////////////////////////////////
inline void AgoraRteStreamingSource::onStateChanged(
    agora::rtc::STREAMING_SRC_STATE state,
    agora::rtc::STREAMING_SRC_ERR err_code) {
  RTE_LOG_VERBOSE << "<StreamingSrcObserver.onStateChanged> state=" << state
                  << ", err_code=" << err_code;
  RteFileInfo current_file;
  GetCurrFileInfo(current_file);

  // switch (state) {
  //  case agora::rtc::STREAMING_SRC_STATE_CLOSED:
  //    break;

  //  case agora::rtc::STREAMING_SRC_STATE_OPENING:
  //    break;

  //  case agora::rtc::STREAMING_SRC_STATE_IDLE:
  //    break;

  //  case agora::rtc::STREAMING_SRC_STATE_PLAYING:
  //    break;

  //  case agora::rtc::STREAMING_SRC_STATE_SEEKING:
  //    break;

  //  case agora::rtc::STREAMING_SRC_STATE_EOF:
  //    break;

  //  case agora::rtc::STREAMING_SRC_STATE_ERROR:
  //    break;

  //  default:
  //    break;
  //}

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
      streaming_source_lock_, observer_list_,
      [&current_file, state, err_code](const auto& ob) {
        ob->OnStateChanged(current_file, state, err_code);
      });
}

inline void AgoraRteStreamingSource::onOpenDone(
    agora::rtc::STREAMING_SRC_ERR err_code) {
  RTE_LOG_VERBOSE << "<StreamingSrcObserver.onOpenDone> err_code=" << err_code;

  RteFileInfo current_file;
  GetCurrFileInfo(current_file);

  int32_t rte_err_code = ConvertErrCode(err_code);
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
      streaming_source_lock_, observer_list_,
      [&current_file, rte_err_code](const auto& handler) {
        handler->OnOpenDone(current_file, rte_err_code);
      });
}

inline void AgoraRteStreamingSource::onSeekDone(
    agora::rtc::STREAMING_SRC_ERR err_code) {
  RTE_LOG_VERBOSE << "<StreamingSrcObserver.onSeekDone> err_code=" << err_code;

  RteFileInfo current_file;
  GetCurrFileInfo(current_file);

  int32_t rte_err_code = ConvertErrCode(err_code);
  AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
      streaming_source_lock_, observer_list_,
      [&current_file, rte_err_code](const auto& handler) {
        handler->OnSeekDone(current_file, rte_err_code);
      });
}

inline void AgoraRteStreamingSource::onEofOnce(int64_t progress_ms,
                                                   int64_t repeat_count) {
  RTE_LOG_VERBOSE << "<StreamingSrcObserver.onProgress> position_ms="
                  << progress_ms << ", repeat_count=" << repeat_count;
  RteFileInfo current_file;
  GetCurrFileInfo(current_file);

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
      streaming_source_lock_, observer_list_,
      [&current_file, progress_ms, repeat_count](const auto& handler) {
        handler->OnEofOnce(current_file, progress_ms, repeat_count);
      });

  int ret = ProcessOneMediaCompleted();
  if (-agora::ERR_CANCELED == ret) {  // All file and loop are finished
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
        streaming_source_lock_, observer_list_,
        [](const auto& ob) { ob->OnAllMediasCompleted(agora::ERR_OK); });
  }
}

inline void AgoraRteStreamingSource::onProgress(int64_t position_ms) {
  RteFileInfo current_file;
  GetCurrFileInfo(current_file);

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
      streaming_source_lock_, observer_list_,
      [&current_file, position_ms](const auto& handler) {
        handler->OnProgress(current_file, position_ms);
      });
}

inline void AgoraRteStreamingSource::onMetaData(const void* data,
                                                    int length) {
  RteFileInfo current_file;
  GetCurrFileInfo(current_file);

  AgoraRteUtils::NotifyEventHandlers<IAgoraRteStreamingSourceObserver>(
      streaming_source_lock_, observer_list_,
      [&current_file, data, length](const auto& handler) {
        handler->OnMetaData(current_file, data, length);
      });
}

inline int32_t
AgoraRteStreamingSource::ConvertErrCode(int32_t src_err_code) {
  struct ErrCodeMap {
    agora::rtc::STREAMING_SRC_ERR src_err;
    agora::ERROR_CODE_TYPE common_err;
  };

  const ErrCodeMap error_code_map[] = {
      {agora::rtc::STREAMING_SRC_ERR_NONE, agora::ERR_OK},
      {agora::rtc::STREAMING_SRC_ERR_UNKNOWN, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_INVALID_PARAM,
       agora::ERR_INVALID_ARGUMENT},
      {agora::rtc::STREAMING_SRC_ERR_BAD_STATE, agora::ERR_INVALID_STATE},
      {agora::rtc::STREAMING_SRC_ERR_NO_MEM, agora::ERR_RESOURCE_LIMITED},
      {agora::rtc::STREAMING_SRC_ERR_BUFFER_OVERFLOW,
       agora::ERR_BUFFER_TOO_SMALL},
      {agora::rtc::STREAMING_SRC_ERR_BUFFER_UNDERFLOW,
       agora::ERR_BUFFER_TOO_SMALL},
      {agora::rtc::STREAMING_SRC_ERR_NOT_FOUND, agora::ERR_NOT_READY},
      {agora::rtc::STREAMING_SRC_ERR_TIMEOUT, agora::ERR_TIMEDOUT},
      {agora::rtc::STREAMING_SRC_ERR_EXPIRED, agora::ERR_NOT_READY},
      {agora::rtc::STREAMING_SRC_ERR_UNSUPPORTED, agora::ERR_NOT_SUPPORTED},
      {agora::rtc::STREAMING_SRC_ERR_NOT_EXIST, agora::ERR_NOT_READY},
      {agora::rtc::STREAMING_SRC_ERR_EXIST, agora::ERR_ALREADY_IN_USE},
      {agora::rtc::STREAMING_SRC_ERR_OPEN, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_CLOSE, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_READ, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_WRITE, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_SEEK, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_EOF, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_CODECOPEN, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_CODECCLOSE, agora::ERR_FAILED},
      {agora::rtc::STREAMING_SRC_ERR_CODECPROC, agora::ERR_FAILED}};
  int err_cnt = sizeof(error_code_map) / sizeof(error_code_map[0]);
  int32_t src_err = -1 * src_err_code;

  for (int i = 0; i < err_cnt; i++) {
    if (error_code_map[i].src_err == src_err) {
      return (-1 * error_code_map[i].common_err);
    }
  }

  return (-agora::ERR_FAILED);
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteTrackBase.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteTrackBase ::AgoraRteTrackBase() {
  auto current_point =
      std::chrono::system_clock::now().time_since_epoch().count();
  track_id_ += std::to_string(current_point);
  track_id_ += "_";
  track_id_ += std::to_string(GenerateTrackTicket());
}

inline void AgoraRteTrackBase::OnTrackPublished() {
  std::lock_guard<std::mutex> _(track_pub_state_lock_);
  // Customer could unpublish in anytime
  //
  if (track_pub_stat_ == PublishState::kPublishing) {
    track_pub_stat_ = PublishState::kPublished;
  }
}

inline void AgoraRteTrackBase::OnTrackUnpublished() {
  std::lock_guard<std::mutex> _(track_pub_state_lock_);
  if (track_pub_stat_ == PublishState::kUnpublishing) {
    SetStreamId("");
    track_pub_stat_ = PublishState::kUnpublished;
  }
}

inline int AgoraRteTrackBase::CheckAndChangePublishState() {
  {
    std::lock_guard<std::mutex> _(track_pub_state_lock_);

    if (track_pub_stat_ != PublishState::kUnpublished)
      return -ERR_INVALID_STATE;
    track_pub_stat_ = PublishState::kPublishing;

    return ERR_OK;
  }
}

inline int AgoraRteTrackBase::CheckAndChangeUnpublishState() {
  std::lock_guard<std::mutex> _(track_pub_state_lock_);
  if (track_pub_stat_ == PublishState::kUnpublished) return ERR_OK;

  track_state_before_unpub = track_pub_stat_;
  track_pub_stat_ = PublishState::kUnpublishing;
  return ERR_OK;
}

inline int AgoraRteTrackBase::BeforePublish(
    const std::shared_ptr<AgoraRteLocalStream>& stream) {
  int result = CheckAndChangePublishState();
  return result;
}

inline void AgoraRteTrackBase::AfterPublish(
    int result, const std::shared_ptr<AgoraRteLocalStream>& stream) {
  if (result == ERR_OK) {
    SetStreamId(stream->GetStreamId());
  } else {
    track_pub_stat_ = PublishState::kUnpublished;
  }
}

inline int AgoraRteTrackBase::BeforeUnPublish(
    const std::shared_ptr<AgoraRteLocalStream>& stream) {
  int result = CheckAndChangeUnpublishState();

  return result;
}

inline void AgoraRteTrackBase::AfterUnPublish(
    int result, const std::shared_ptr<AgoraRteLocalStream>& stream) {
  if (result == ERR_OK) {
    OnTrackUnpublished();
  } else {
    track_pub_stat_ = track_state_before_unpub;
  }
}

inline std::shared_ptr<AgoraRteTrackImplBase>
AgoraRteRtcVideoTrackBase::GetTackImpl() {
  return track_impl_;
}

inline std::shared_ptr<AgoraRteTrackImplBase>
AgoraRteRtcAudioTrackBase::GetTackImpl() {
  return track_impl_;
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteUtils.cpp ======================= 
#pragma once

#include "AgoraRteSdkImpl.hpp" 

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <vector>

#include "AgoraRefPtr.h"
#include "IAgoraRteMediaTrack.h"

#if defined(__clang__)
#if __has_feature(cxx_rtti)
#define RTTI_ENABLED
#endif
#elif defined(__GNUG__)
#if defined(__GXX_RTTI)
#define RTTI_ENABLED
#endif
#elif defined(_MSC_VER)
#if defined(_CPPRTTI)
#define RTTI_ENABLED
#endif
#endif

namespace agora {
namespace rte {

static const std::string g_rte_user_format = "u=%s";
static const std::string g_rte_stream_format = "v=%s&u=%s&s=%s";
static const std::string g_rte_token_version = "1";

inline std::shared_ptr<AgoraRteRtcVideoTrackBase> AgoraRteUtils::CastToImpl(
  std::shared_ptr<IAgoraRteVideoTrack> track) {
  std::shared_ptr<AgoraRteRtcVideoTrackBase> result;
  if (track) {
    switch (track->GetSourceType()) {
      case agora::rte::SourceType::kVideo_Camera: {
        result = std::static_pointer_cast<AgoraRteCameraVideoTrack>(track);
        break;
      }
      case agora::rte::SourceType::kVideo_Custom: {
        result = std::static_pointer_cast<AgoraRteCustomVideoTrack>(track);
        break;
      }
      case agora::rte::SourceType::kVideo_Mix: {
        result = std::static_pointer_cast<AgoraRteMixedVideoTrack>(track);
        break;
      }
      case agora::rte::SourceType::kVideo_Screen: {
        result = std::static_pointer_cast<AgoraRteScreenVideoTrack>(track);
        break;
      }
      case agora::rte::SourceType::kVideo_Wrapper: {
        result = std::static_pointer_cast<AgoraRteWrapperVideoTrack>(track);
        break;
      }
      default:
        assert(false);
        break;
    }
  }
  return result;
};

inline std::shared_ptr<AgoraRteRtcAudioTrackBase> AgoraRteUtils::CastToImpl(
  std::shared_ptr<IAgoraRteAudioTrack> track) {
  std::shared_ptr<AgoraRteRtcAudioTrackBase> result;
  if (track) {
    switch (track->GetSourceType()) {
      case agora::rte::SourceType::kAudio_Microphone: {
        result = std::static_pointer_cast<AgoraRteMicrophoneAudioTrack>(track);
        break;
      }
      case agora::rte::SourceType::kAudio_Custom: {
        result = std::static_pointer_cast<AgoraRteCustomAudioTrack>(track);
        break;
      }
      case agora::rte::SourceType::kAudio_Wrapper: {
        result = std::static_pointer_cast<AgoraRteWrapperAudioTrack>(track);
        break;
      }
      default:
        assert(false);
        break;
    }
  }
  return result;
}

inline std::string AgoraRteUtils::GenerateRtcStreamId(
    bool is_major_stream, const std::string& rtc_user_id,
    const std::string& stream_id) {
  std::string result;

  if (is_major_stream) {
    const std::string empty;
    result = GeneratorJsonUserId(g_rte_user_format,rtc_user_id.c_str());
  } else {
    result =
        GeneratorJsonUserId(g_rte_stream_format, g_rte_token_version.c_str(),
                            rtc_user_id.c_str(), stream_id.c_str());
  }

  return result;
}

inline bool AgoraRteUtils::SplitKeyPairInRtcStream(
    const std::string& key_pair, std::map<std::string, std::string>& store) {
  auto start = 0;
  auto end = key_pair.find('=');
  if (end != std::string::npos) {
    std::string key = key_pair.substr(start, end - start);
    std::string value = key_pair.substr(end + 1);
    if (store[key].empty() && !value.empty()) {
      store[key] = value;
      return true;
    }
  }

  return false;
}

inline bool AgoraRteUtils::ExtractRtcStreamInfo(
    const std::string& rtc_stream_id, RteRtcStreamInfo& info) {
  std::smatch sm;

  if (rtc_stream_id.find("u=") == 0) {
    info.user_id = rtc_stream_id.substr(2);
    info.stream_id = "";
    info.is_major_stream = true;
    return !info.user_id.empty();
  }
  else {
    std::map<std::string, std::string> key_pairs;
    auto start = 0;
    auto end = rtc_stream_id.find('&');
    while (end != std::string::npos) {
      std::string key_pair = rtc_stream_id.substr(start, end - start);
      start = end + 1U;
      end = rtc_stream_id.find('&', start);
      if (!SplitKeyPairInRtcStream(key_pair, key_pairs)) return false;
    }

    std::string last_pair = rtc_stream_id.substr(start, end);
    if (!SplitKeyPairInRtcStream(last_pair, key_pairs)) return false;

    info.user_id = key_pairs["u"];
    info.stream_id = key_pairs["s"];
    info.is_major_stream = false;
    return !info.user_id.empty() && !info.stream_id.empty();
  }

  return false;
}

inline ConnectionState
AgoraRteUtils::GetConnStateFromRtcState(rtc::CONNECTION_STATE_TYPE state) {
  switch (state) {
    case agora::rtc::CONNECTION_STATE_DISCONNECTED:
      return ConnectionState::kDisconnected;
      break;
    case agora::rtc::CONNECTION_STATE_CONNECTING:
      return ConnectionState::kConnecting;
      break;
    case agora::rtc::CONNECTION_STATE_CONNECTED:
      return ConnectionState::kConnected;
      break;
    case agora::rtc::CONNECTION_STATE_RECONNECTING:
      return ConnectionState::kReconnecting;
      break;
    case agora::rtc::CONNECTION_STATE_FAILED:
    default:
      return ConnectionState::kFailed;
  }
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteVideoDeviceManager.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

namespace agora {
namespace rte {

#if RTE_WIN || RTE_MAC

inline AgoraRteVideoDeviceManager::AgoraRteVideoDeviceManager(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  auto media_node_factory = rtc_service->createMediaNodeFactory();
  camera_capturer_ = media_node_factory->createCameraCapturer();
}

inline std::vector<VideoDeviceInfo>
AgoraRteVideoDeviceManager::EnumerateVideoDevices() {
  std::vector<VideoDeviceInfo> result;
  std::unique_ptr<rtc::ICameraCapturer::IDeviceInfo> device_info(
      camera_capturer_->createDeviceInfo());
  if (device_info) {
    int number = device_info->NumberOfDevices();
    constexpr int max_string_len = 260;
    for (int i = 0; i < number; i++) {
      char device_name[max_string_len] = {0};
      char device_id[max_string_len] = {0};
      char product_id[max_string_len] = {0};
      if (device_info->GetDeviceName(i, device_name, max_string_len, device_id,
                                     max_string_len, product_id,
                                     max_string_len) == ERR_OK) {
        std::string device_name_str(device_name);
        std::string device_id_str(device_id);
        result.push_back(
            {std::move(device_name_str), std::move(device_id_str)});
      }
    }
  }

  return result;
}

#endif
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteVideoTrackImpl.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 

#include "AgoraAtomicOps.h"
namespace agora {
namespace rte {

inline int AgoraRteRawVideoFrameRender::onFrame(
    const media::base::VideoFrame& videoFrame) {
  auto track = video_track_.lock();

  if (track) {
    AgoraRteUtils::NotifyEventHandlers<IAgoraRteVideoFrameObserver>(
        track->track_lock_, track->local_video_frame_observers_,
        [&](const auto& handler) {
          handler->OnFrame(track->GetStreamId(), videoFrame);
        });
  }

  return ERR_OK;
}

inline AgoraRteVideoTrackImpl::AgoraRteVideoTrackImpl(
    std::shared_ptr<agora::base::IAgoraService> rtc_service) {
  rtc_service_ = rtc_service;

  auto media_node_factory = rtc_service->createMediaNodeFactory();

  media_node_factory_ =
      AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IMediaNodeFactory>(
          media_node_factory);
}

inline AgoraRteVideoTrackImpl::~AgoraRteVideoTrackImpl() { Reset(); }

inline void AgoraRteVideoTrackImpl::SetTrack(
    std::shared_ptr<rtc::ILocalVideoTrack> track) {
  video_track_ = track;
}

inline std::shared_ptr<rtc::IVideoRenderer>
AgoraRteVideoTrackImpl::GetVideoRender(bool create_if_not_exist) {
  {
    std::lock_guard<std::mutex> _(render_lock_);
    if (!video_render_) {
      if (!create_if_not_exist) {
        return nullptr;
      }

      auto video_render = media_node_factory_->createVideoRenderer();

      if (video_render) {
        video_render_ =
            AgoraRteUtils::AgoraRefObjectToSharedObject<rtc::IVideoRenderer>(
                video_render);
      } else {
        return nullptr;
      }
    }
  }

  return video_render_;
}

inline int AgoraRteVideoTrackImpl::SetView(View view) {
  auto render = GetVideoRender();

  if (!render) return -ERR_FAILED;
  return render->setView(view);
}

inline int AgoraRteVideoTrackImpl::SetVideoEncoderConfiguration(
    const VideoEncoderConfiguration& config) {
  return video_track_->setVideoEncoderConfiguration(config);
}

inline int AgoraRteVideoTrackImpl::SetPreviewCanvas(
    const VideoCanvas& canvas) {
  int result = -ERR_FAILED;
  auto render = GetVideoRender();

  if (render) {
    result = render->setRenderMode(canvas.renderMode);

    if (result != ERR_OK) return result;

    switch (canvas.mirrorMode) {
      case rtc::VIDEO_MIRROR_MODE_AUTO:
      case rtc::VIDEO_MIRROR_MODE_ENABLED:
        result = render->setMirror(true);
        break;
      case rtc::VIDEO_MIRROR_MODE_DISABLED:
      default:
        result = render->setMirror(false);
        break;
    }

    if (result != ERR_OK) return result;

    if (canvas.view) {
      result = render->setView(canvas.view);
    }
  }

  return result;
}

inline int AgoraRteVideoTrackImpl::RegisterLocalVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  UnregisterLocalVideoFrameObserver(observer);

  bool result = false;
  {
    std::lock_guard<std::mutex> _(track_lock_);
    local_video_frame_observers_.push_back(observer);

    if (!raw_video_frame_renders_) {
      raw_video_frame_renders_ =
          make_refptr<AgoraRteRawVideoFrameRender>(shared_from_this());

      result = video_track_->addRenderer(raw_video_frame_renders_);
    }
  }

  return result ? ERR_OK : -ERR_FAILED;
}

inline void AgoraRteVideoTrackImpl::UnregisterLocalVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  AgoraRteUtils::UnregisterSharedPtrFromContainer(
      track_lock_, local_video_frame_observers_, observer);
  {
    std::lock_guard<std::mutex> _(track_lock_);
    if (local_video_frame_observers_.empty() && raw_video_frame_renders_) {
      video_track_->removeRenderer(raw_video_frame_renders_);
      raw_video_frame_renders_ = nullptr;
    }
  }
}

inline std::shared_ptr<agora::rtc::ILocalVideoTrack>
AgoraRteVideoTrackImpl::GetVideoTrack() const {
  return video_track_;
}

inline int AgoraRteVideoTrackImpl::Start() {
  int result = ERR_OK;
  {
    // Lock to synchronize between start and stop
    //
    std::lock_guard<std::mutex> _(track_lock_);

    if (is_started_) {
      return ERR_OK;
    }

    if (is_render_enabled_) {
      bool create_if_not_exist = false;
      auto render = GetVideoRender(create_if_not_exist);

      if (render) {
        agora_refptr<rtc::IVideoRenderer> agora_render(render.get());
        if (!video_track_->addRenderer(agora_render)) {
          result = -ERR_FAILED;
        }
      }
    }

    if (result == ERR_OK) {
      video_track_->setEnabled(true);
      is_started_ = true;

      result = ERR_OK;
    } else {
      Reset();
    }
  }

  return result;
}

inline void AgoraRteVideoTrackImpl::Reset() {
  bool enable_track = false;
  bool create_if_not_exist = false;

  auto render = GetVideoRender(create_if_not_exist);
  if (render) render->setView(nullptr);

  if (video_track_) {
    video_track_->setEnabled(enable_track);
  }

  if (raw_video_frame_renders_) {
    agora_refptr<rtc::IVideoSinkBase> sink(raw_video_frame_renders_);
    video_track_->removeRenderer(sink);
  }

  is_started_ = false;
}

inline void AgoraRteVideoTrackImpl::Stop() {
  std::lock_guard<std::mutex> _(track_lock_);

  Reset();
}
}  // namespace rte
}  // namespace agora

// ======================= AgoraRteWrapperAudioTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteWrapperAudioTrack::AgoraRteWrapperAudioTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service,
    std::shared_ptr<rtc::ILocalAudioTrack> rtc_audio_track) {
  track_impl_ = std::make_shared<AgoraRteAudioTrackImpl>(rtc_service);
  track_impl_->SetTrack(rtc_audio_track);
}

//////////////////////////////////////////////////////////////////////
///////////////// Override Methods of IAgoraRteAudioTrack ////////////
///////////////////////////////////////////////////////////////////////
inline int AgoraRteWrapperAudioTrack::EnableLocalPlayback() {
  return track_impl_->EnableLocalPlayback();
}

inline SourceType AgoraRteWrapperAudioTrack::GetSourceType() {
  return SourceType::kAudio_Wrapper;
}

inline int AgoraRteWrapperAudioTrack::AdjustPublishVolume(int volume) {
  return track_impl_->AdjustPublishVolume(volume);
}

inline int AgoraRteWrapperAudioTrack::AdjustPlayoutVolume(int volume) {
  return track_impl_->AdjustPlayoutVolume(volume);
}

inline const std::string& AgoraRteWrapperAudioTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

////////////////////////////////////////////////////////////////////////////////
///////////////// Override Methods of AgoraRteRtcAudioTrackBase ////////////////
////////////////////////////////////////////////////////////////////////////////
inline std::shared_ptr<agora::rtc::ILocalAudioTrack>
AgoraRteWrapperAudioTrack::GetRtcAudioTrack() const {
  return track_impl_->GetAudioTrack();
}

////////////////////////////////////////////////////////////////////
///////////////// Override Methods of AgoraRteTrackBase ////////////
////////////////////////////////////////////////////////////////////
inline void AgoraRteWrapperAudioTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

}  // namespace rte
}  // namespace agora

// ======================= AgoraRteWrapperVideoTrack.cpp ======================= 
#include "AgoraRteSdkImpl.hpp" 


namespace agora {
namespace rte {

inline AgoraRteWrapperVideoTrack::AgoraRteWrapperVideoTrack(
    std::shared_ptr<agora::base::IAgoraService> rtc_service,
    std::shared_ptr<rtc::ILocalVideoTrack> rtc_video_track) {
  track_impl_ = std::make_shared<AgoraRteVideoTrackImpl>(rtc_service);
  track_impl_->SetTrack(rtc_video_track);
}

//////////////////////////////////////////////////////////////////////
///////////////// Override Methods of IAgoraRteVideoTrack ////////////
///////////////////////////////////////////////////////////////////////
inline int AgoraRteWrapperVideoTrack::SetPreviewCanvas(
    const VideoCanvas& canvas) {
  return track_impl_->SetPreviewCanvas(canvas);
}

inline SourceType AgoraRteWrapperVideoTrack::GetSourceType() {
  return SourceType::kVideo_Wrapper;
}

inline void AgoraRteWrapperVideoTrack::RegisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->RegisterLocalVideoFrameObserver(observer);
}
inline void AgoraRteWrapperVideoTrack::UnregisterVideoFrameObserver(
    std::shared_ptr<IAgoraRteVideoFrameObserver> observer) {
  track_impl_->UnregisterLocalVideoFrameObserver(observer);
}

inline int AgoraRteWrapperVideoTrack::EnableVideoFilter(
    const std::string& id, bool enable) {
  return track_impl_->GetVideoTrack()->enableVideoFilter(id.c_str(), enable);
}

inline int AgoraRteWrapperVideoTrack::SetFilterProperty(
    const std::string& id, const std::string& key,
    const std::string& json_value) {
  return track_impl_->GetVideoTrack()->setFilterProperty(
      id.c_str(), key.c_str(), json_value.c_str());
}

inline std::string AgoraRteWrapperVideoTrack::GetFilterProperty(
    const std::string& id, const std::string& key) {
  char buffer[1024];

  if (track_impl_->GetVideoTrack()->getFilterProperty(id.c_str(), key.c_str(),
                                                      buffer, 1024) == ERR_OK) {
    return buffer;
  }

  return "";
}

inline const std::string& AgoraRteWrapperVideoTrack::GetAttachedStreamId() {
  return track_impl_->GetStreamId();
}

///////////////////////////////////////////////////////////////////////////////
///////////////// Override Methods of AgoraRteRtcVideoTrackBase ///////////////
///////////////////////////////////////////////////////////////////////////////
inline std::shared_ptr<agora::rtc::ILocalVideoTrack>
AgoraRteWrapperVideoTrack::GetRtcVideoTrack() const {
  return track_impl_->GetVideoTrack();
}

////////////////////////////////////////////////////////////////////
///////////////// Override Methods of AgoraRteTrackBase ////////////
////////////////////////////////////////////////////////////////////
inline void AgoraRteWrapperVideoTrack::SetStreamId(
    const std::string& stream_id) {
  track_impl_->SetStreamId(stream_id);
}

}  // namespace rte
}  // namespace agora

