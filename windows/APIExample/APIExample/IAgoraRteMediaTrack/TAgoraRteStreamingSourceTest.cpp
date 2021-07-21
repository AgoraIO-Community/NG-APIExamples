// ======================= TAgoraRteStreamingSourceTest.cpp ======================= 
// AgoraRte.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//
#include <stdexcept>
#include <thread>

#include "AgoraBase.h"
#include "AgoraRteSdkImpl.hpp" 
#include "IAgoraRteMediaObserver.h"
#include "IAgoraRteMediaPlayer.h"
#include "IAgoraRteMediaTrack.h"
#include "IAgoraRteScene.h"
#include "IAgoraRteStreamingSource.h"
#include "TestUitls.h"
#include "gtest/gtest.h"

using namespace agora::rte;

#define AGO_LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////
/////////////////// Smoke Testcase for Rte Streaming Source //////////////////
//////////////////////////////////////////////////////////////////////////////

void Thread_Sleep(int64_t milili_seconds) {
  std::chrono::milliseconds du(milili_seconds);
  std::this_thread::sleep_for(du);
}

class StreamingSrcSceneEventHandler : public IAgoraRteSceneEventHandler {
 public:
  void OnConnectionStateChanged(ConnectionState old_state,
                                ConnectionState new_state,
                                ConnectionChangedReason reason) override {
    AGO_LOG(
        "<OnConnectionStateChanged> old_state=%d, new_state=%d, reason=%d\n",
        static_cast<int>(old_state), static_cast<int>(new_state),
        static_cast<int>(reason));

    connection_stat_changed_semaphore.Notify();

    switch (new_state) {
      case ConnectionState::kDisconnecting:
        break;
      case ConnectionState::kConnecting:
        break;

      case ConnectionState::kConnected:
        connect_success_sempahore.Notify();
        break;

      case ConnectionState::kReconnecting:
        break;
      case ConnectionState::kFailed:
      case ConnectionState::kDisconnected:
        connect_gone_sempahore.Notify();
        break;
      default:
        break;
    }
  }

  void OnRemoteUserJoined(const std::vector<UserInfo>& users) override {
    remote_user_joined_semaphore.Notify();
  }

  void OnRemoteUserLeft(const std::vector<UserInfo>& users) override {
    remote_user_left_semaphore.Notify();
  }

  void OnRemoteStreamAdded(const std::vector<StreamInfo>& streams) override {
    remote_stream_added_semaphore.Notify();
  }

  void OnRemoteStreamRemoved(const std::vector<StreamInfo>& streams) override {
    remote_stream_removed_semaphore.Notify();
  }

  void OnLocalStreamStateChanged(const StreamInfo& streams,
                                 MediaType media_type,
                                 StreamMediaState old_state,
                                 StreamMediaState new_state,
                                 StreamStateChangedReason reason) override {}

  void OnRemoteStreamStateChanged(const StreamInfo& streams,
                                  MediaType media_type,
                                  StreamMediaState old_state,
                                  StreamMediaState new_state,
                                  StreamStateChangedReason reason) override {}

  void OnAudioVolumeIndication(const std::vector<AudioVolumeInfo>& speakers,
                               int totalVolume) override {}

  void OnSceneTokenWillExpired(const std::string& scene_id,
                               const std::string& token) override {}

  void OnSceneTokenExpired(const std::string& scene_id) override {}

  void OnStreamTokenWillExpire(const std::string& stream_id,
                               const std::string& token) override {}

  void OnStreamTokenExpired(const std::string& stream_id) override {}

  void OnBypassCdnStateChanged(
      const std::string& stream_id, const std::string& target_cdn_url,
      CDNBYPASS_STREAM_PUBLISH_STATE state,
      CDNBYPASS_STREAM_PUBLISH_ERROR err_code) override {
    AGO_LOG(
        "<OnBypassCdnStateChanged> target_cdn_url=%s, state=%d, err_code=%d\n",
        target_cdn_url.c_str(), static_cast<int>(state),
        static_cast<int>(err_code));
  }

  void OnBypassCdnPublished(const std::string& stream_id,
                            const std::string& target_cdn_url,
                            CDNBYPASS_STREAM_PUBLISH_ERROR error) override {
    AGO_LOG("<OnBypassCdnPublished> target_cdn_url=%s, error=%d\n",
            target_cdn_url.c_str(), error);
  };

  void OnBypassCdnUnpublished(const std::string& stream_id,
                              const std::string& target_cdn_url) override {
    AGO_LOG("<OnBypassCdnUnpublished> target_cdn_url=%s\n",
            target_cdn_url.c_str());
  }

  void OnBypassTranscodingUpdated(const std::string& stream_id) override {
    AGO_LOG("<OnBypassTranscodingUpdated>\n");
  };

  void OnSceneStats(const SceneStats& stats) override {
    AGO_LOG("<OnSceneStats>\n");
  }

  void OnLocalStreamStats(const std::string& stream_id,
                          const LocalStreamStats& stats) override {
    AGO_LOG("<OnLocalStreamStats %s>\n", stream_id.c_str());
  }

  void OnRemoteStreamStats(const std::string& stream_id,
                           const RemoteStreamStats& stats) override {
    AGO_LOG("<OnRemoteStreamStats %s>\n", stream_id.c_str());
  }

 public:
  agora_test_semaphore connection_stat_changed_semaphore;
  agora_test_semaphore remote_user_joined_semaphore;
  agora_test_semaphore remote_user_left_semaphore;
  agora_test_semaphore remote_stream_added_semaphore;
  agora_test_semaphore remote_stream_removed_semaphore;
  agora_test_semaphore connect_success_sempahore;
  agora_test_semaphore connect_gone_sempahore;

  std::set<std::string> remote_streams;
  std::set<std::string> remote_users;
};

class StreamingSrcObserver
    : public agora::rte::IAgoraRteStreamingSourceObserver {
 public:
  StreamingSrcObserver() {
    eof_flag_ = false;
    opening_flag_ = false;
    seeking_flag_ = false;
  }

  ~StreamingSrcObserver() = default;

 public:
  void ResetEof() { eof_flag_ = false; }
  bool isEof() { return eof_flag_; }

  void ResetOpening() { opening_flag_ = false; }
  bool isOpening() { return opening_flag_; }
  bool WaitOpenDone(int32_t seconds_timeout) {
    int wait_result = opened_semaphore_.Wait(seconds_timeout);
    return (wait_result < 0) ? false : true;
  }

  void ResetSeeking() { seeking_flag_ = false; }
  bool isSeeking() { return seeking_flag_; }
  bool WaitSeekDone(int32_t seconds_timeout) {
    int wait_result = seeked_semaphore_.Wait(seconds_timeout);
    return (wait_result < 0) ? false : true;
  }

  bool WaitAllFilesCompleted(int32_t seconds_timeout) {
    int wait_result = allfiles_completed_semaphore_.Wait(seconds_timeout);
    return (wait_result < 0) ? false : true;
  }

  void OnStateChanged(const RteFileInfo& current_file_info,
                      agora::rtc::STREAMING_SRC_STATE state,
                      agora::rtc::STREAMING_SRC_ERR err_code) override {
    AGO_LOG("<onStateChanged> state=%d, err_code=%d\n", static_cast<int>(state),
            static_cast<int>(err_code));

    switch (state) {
      case agora::rtc::STREAMING_SRC_STATE_CLOSED: {
      } break;

      case agora::rtc::STREAMING_SRC_STATE_OPENING: {
        opening_flag_ = true;
      } break;

      case agora::rtc::STREAMING_SRC_STATE_IDLE: {
      } break;

      case agora::rtc::STREAMING_SRC_STATE_PLAYING: {
      } break;

      case agora::rtc::STREAMING_SRC_STATE_SEEKING: {
      } break;

      case agora::rtc::STREAMING_SRC_STATE_EOF: {
        eof_flag_ = true;
      } break;

      case agora::rtc::STREAMING_SRC_STATE_ERROR: {
      } break;

      default:
        break;
    }
  }

  void OnProgress(const RteFileInfo& current_file_info,
                  int64_t position_ms) override {
    AGO_LOG("<StreamingSrcObserver.onProgress> position_ms=%ld\n",
            static_cast<long>(position_ms));
  }

  void OnOpenDone(const RteFileInfo& current_file_info,
                  int32_t err_code) override {
    AGO_LOG("<StreamingSrcObserver.OnOpenDone> err_code=%d\n", err_code);
    opened_semaphore_.Notify();
  }

  void OnSeekDone(const RteFileInfo& current_file_info,
                  int32_t err_code) override {
    AGO_LOG("<StreamingSrcObserver.OnSeekDone> err_code=%d\n", err_code);
    seeked_semaphore_.Notify();
  }

  void OnEofOnce(const RteFileInfo& current_file_info, int64_t progress_ms,
                 int64_t repeat_count) override {
    AGO_LOG(
        "<StreamingSrcObserver.onEofOnce> progress_ms=%ld, repeat_count=%ld\n",
        static_cast<long>(progress_ms), static_cast<long>(repeat_count));
  }

  void OnAllMediasCompleted(int32_t err_code) override {
    AGO_LOG("<StreamingSrcObserver.OnAllMediasCompleted>\n");
    allfiles_completed_semaphore_.Notify();
  }

  void OnMetaData(const RteFileInfo& current_file_info, const void* data,
                  int length) override {
    AGO_LOG("<StreamingSrcObserver.onMetaData> length=%d\n", length);
  }

 private:
  agora_test_semaphore opened_semaphore_;
  agora_test_semaphore seeked_semaphore_;
  agora_test_semaphore allfiles_completed_semaphore_;

  std::atomic<bool> eof_flag_;
  std::atomic<bool> opening_flag_;
  std::atomic<bool> seeking_flag_;
};

class TestFileSourceImpl {
 public:
  bool Init() {
    int ret = 0;

#if defined(WIN32)
    {
      char szAppName[300] = {0};
      char* pFileName = NULL;
      int path_len = 0;
      GetModuleFileNameA(NULL, szAppName, 300);
      pFileName =
          (pFileName = strrchr(szAppName, '\\')) ? pFileName : szAppName;
      path_len = strlen(szAppName) - strlen(pFileName);
      char szAppPath[300] = {0};
      strncpy(szAppPath, szAppName, path_len);

      std::string str_app_path = szAppPath;
      test_file_path1_ = str_app_path + "\\test_data\\h264_opus_01.mp4";
      test_file_path2_ = str_app_path + "\\test_data\\h264_opus_02.mp4";
      test_file_path3_ = str_app_path + "\\test_data\\h264_opus_03.mp4";
      test_file_path4_ = str_app_path + "\\test_data\\h264_opus_04.mp4";
      test_file_path5_ = str_app_path + "\\test_data\\h264_opus_05.mp4";
    }
#endif

    //
    // Initialize sdk
    //
    AgoraRteLogger::EnableLogging(true);
    AgoraRteLogger::SetLevel(LogLevel::Verbose);
    AgoraRteLogger::SetListener(
        [](const std::string& message) { std::cout << message; });

    agora::rte::SdkProfile profile;
    profile.appid = app_id_;
    AgoraRteSDK::Init(profile);

    //
    // Create rte scene and register event handler
    //
    scene_event_handler_ = std::make_shared<StreamingSrcSceneEventHandler>();
    EXPECT_TRUE(scene_event_handler_ != nullptr);
    SceneConfig config;
    rte_scene_ = AgoraRteSDK::CreateRteScene(scene_id_, config);
    EXPECT_TRUE(rte_scene_ != nullptr);
    rte_scene_->RegisterEventHandler(scene_event_handler_);

    media_factory_ = AgoraRteSDK::GetRteMediaFactory();
    EXPECT_TRUE(media_factory_ != nullptr);

    streaming_source_ = media_factory_->CreateStreamingSource();
    EXPECT_TRUE(streaming_source_ != nullptr);
    streaming_src_observer_ = std::make_shared<StreamingSrcObserver>();
    EXPECT_TRUE(streaming_src_observer_ != nullptr);
    ret = streaming_source_->RegisterObserver(streaming_src_observer_);
    EXPECT_EQ(ret, 0);

    video_track_ = streaming_source_->GetRteVideoTrack();
    EXPECT_TRUE(video_track_ != nullptr);
    audio_track_ = streaming_source_->GetRteAudioTrack();
    EXPECT_TRUE(audio_track_ != nullptr);

    //
    // Join scene and sync waiting for join successful
    //
    JoinOptions option;
    option.is_user_visible_to_remote = false;
    ret = rte_scene_->Join(local_user_id_, token_, option);
    EXPECT_EQ(ret, 0);
    ret = scene_event_handler_->connect_success_sempahore.Wait(8);
    EXPECT_EQ(ret, 1);

    // Config the publish options
    agora::rte::RtcStreamOptions pub_option(token_);
    pub_option.type = StreamType::kRtcStream;
    ret = rte_scene_->CreateOrUpdateRTCStream(stream_id_, pub_option);
    EXPECT_EQ(ret, 0);

    is_ready = true;
    AGO_LOG("<TestFileSourceImpl.Init> finished, Test is ready\n\n");
    return true;
  }

  void Uninit() {
    int ret = 0;

    //
    // Leave scene and sync waiting for leave successful
    //
    if ((rte_scene_ != nullptr) && (scene_event_handler_ != nullptr)) {
      rte_scene_->Leave();
      ret = scene_event_handler_->connect_gone_sempahore.Wait(8);
      EXPECT_EQ(ret, 1);
    }

    //
    // Unregister event handler
    //
    if (scene_event_handler_ != nullptr) {
      rte_scene_->UnregisterEventHandler(scene_event_handler_);
      scene_event_handler_.reset();
    }

    //
    // Unregister observer of streaming source
    //
    if (streaming_source_ != nullptr) {
      streaming_source_->UnregisterObserver(streaming_src_observer_);
      streaming_src_observer_.reset();
      streaming_source_.reset();
    }

    video_track_.reset();
    audio_track_.reset();
    media_factory_.reset();
    rte_scene_.reset();
    AgoraRteSDK::Deinit();

    is_ready = false;
    AGO_LOG("<TestFileSourceImpl.Uninit> finished, Test is Done\n\n");
  }

  void PlayList() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestFileSourceImpl.PlayMultipleFiles> ==>Enter\n");

    std::string file_path_array[] = {
        test_file_path1_,
        test_file_path2_,
    };
    int32_t file_count = sizeof(file_path_array) / sizeof(file_path_array[0]);
    int32_t file_index = 0;
    int ret;

    while (file_index < file_count) {
      // set repeat count
      streaming_source_->SetLoopCount(1);

      // open media streaming
      ret = streaming_source_->Open(file_path_array[file_index].c_str(), 0,
                                    false);
      EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
      bool result = streaming_src_observer_->WaitOpenDone(10);
      EXPECT_TRUE(result);
      AGO_LOG("<TestFileSourceImpl.PlayMultipleFiles> file is opened\n");

      // publish/unpublish video track
      if (streaming_source_->isVideoValid()) {
        rte_scene_->PublishLocalVideoTrack(stream_id_, video_track_);
      } else {
        rte_scene_->UnpublishLocalVideoTrack(video_track_);
      }

      // publish/unpublish audio track
      if (streaming_source_->isAudioValid()) {
        rte_scene_->PublishLocalAudioTrack(stream_id_, audio_track_);
      } else {
        rte_scene_->UnpublishLocalAudioTrack(audio_track_);
      }

      // Start playing...
      AGO_LOG("<TestFileSourceImpl.PlayMultipleFiles> Start playing...\n");
      streaming_src_observer_->ResetEof();
      ret = streaming_source_->Play();
      EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);

      // Waiting for playing is EOF
      for (;;) {
        if (streaming_src_observer_->isEof()) {
          break;
        }
        Thread_Sleep(200);
      }

      ret = streaming_source_->Close();
      EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
      AGO_LOG("<TestFileSourceImpl.PlayList> file is closed!\n");

      // Unpublish audio & video track
      rte_scene_->UnpublishLocalAudioTrack(audio_track_);
      rte_scene_->UnpublishLocalVideoTrack(video_track_);

      file_index++;
    }

    AGO_LOG("<TestFileSourceImpl.PlayMultipleFiles> <==Exit\n\n");
  }

  void PlayCtrl() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestFileSourceImpl.PlayCtrl> ==>Enter\n");

    // set repeat count
    streaming_source_->SetLoopCount(1);

    // open media streaming
    int ret = streaming_source_->Open(test_file_path1_.c_str(), 0, false);
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    bool result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    AGO_LOG("<TestFileSourceImpl.PlayCtrl> file is opened.\n");

    // publish/unpublish video track
    if (streaming_source_->isVideoValid()) {
      rte_scene_->PublishLocalVideoTrack(stream_id_, video_track_);
    } else {
      rte_scene_->UnpublishLocalVideoTrack(video_track_);
    }

    // publish/unpublish audio track
    if (streaming_source_->isAudioValid()) {
      rte_scene_->PublishLocalAudioTrack(stream_id_, audio_track_);
    } else {
      rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    }

    // Play 3s
    AGO_LOG("<TestFileSourceImpl.PlayCtrl> Start playing...\n");
    streaming_src_observer_->ResetEof();
    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    Thread_Sleep(3000);

    // Pause 5s
    ret = streaming_source_->Pause();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    Thread_Sleep(5000);

    // Play 3s
    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    Thread_Sleep(3000);

    // Pause 4s
    ret = streaming_source_->Pause();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    Thread_Sleep(4000);

    // Play 3s
    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    Thread_Sleep(3000);

    // Close directly
    ret = streaming_source_->Close();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    AGO_LOG("<StreamSrcTest_PlayCtrl> file is closed!\n");

    // Unpublish audio & video track
    rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    rte_scene_->UnpublishLocalVideoTrack(video_track_);

    AGO_LOG("<TestFileSourceImpl.PlayCtrl> <==Exit\n\n");
  }

  void PlaySeek() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestFileSourceImpl.PlaySeek> ==>Enter\n");

    // set repeat count
    streaming_source_->SetLoopCount(1);

    // open media streaming
    int ret = streaming_source_->Open(test_file_path1_.c_str(), 0, false);
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    bool result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    AGO_LOG("<TestFileSourceImpl.PlaySeek> file is opened.\n");

    // publish/unpublish video track
    if (streaming_source_->isVideoValid()) {
      rte_scene_->PublishLocalVideoTrack(stream_id_, video_track_);
    } else {
      rte_scene_->UnpublishLocalVideoTrack(video_track_);
    }

    // publish/unpublish audio track
    if (streaming_source_->isAudioValid()) {
      rte_scene_->PublishLocalAudioTrack(stream_id_, audio_track_);
    } else {
      rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    }

    // Play 3s
    AGO_LOG("<TestFileSourceImpl.PlaySeek> Start playing...\n");
    streaming_src_observer_->ResetEof();
    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    Thread_Sleep(3000);

    // Seek to 2s and play 3s
    ret = streaming_source_->Seek(2000);
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    result = streaming_src_observer_->WaitSeekDone(
        10);  // waiting for seeking finished
    EXPECT_TRUE(result);
    Thread_Sleep(3000);

    // Seek to 5s and play 2s
    ret = streaming_source_->Seek(5000);
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    result = streaming_src_observer_->WaitSeekDone(
        10);  // waiting for seeking finished
    EXPECT_TRUE(result);
    Thread_Sleep(2000);

    // Close directly
    ret = streaming_source_->Close();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    AGO_LOG("<TestFileSourceImpl.PlaySeek> file is closed!\n");

    // Unpublish audio & video track
    rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    rte_scene_->UnpublishLocalVideoTrack(video_track_);

    AGO_LOG("<TestFileSourceImpl.PlaySeek> <==Exit\n\n");
  }

  void PlayLoop() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestFileSourceImpl.PlayLoop> ==>Enter\n");

    // set repeat count
    streaming_source_->SetLoopCount(2);

    // open media streaming
    int ret = streaming_source_->Open(test_file_path1_.c_str(), 0, false);
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    bool result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    AGO_LOG("<TestFileSourceImpl.PlayLoop> file is opened.\n");

    // publish/unpublish video track
    if (streaming_source_->isVideoValid()) {
      rte_scene_->PublishLocalVideoTrack(stream_id_, video_track_);
    } else {
      rte_scene_->UnpublishLocalVideoTrack(video_track_);
    }

    // publish/unpublish audio track
    if (streaming_source_->isAudioValid()) {
      rte_scene_->PublishLocalAudioTrack(stream_id_, audio_track_);
    } else {
      rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    }

    // Playing
    AGO_LOG("<TestFileSourceImpl.PlayLoop> Start playing...\n");
    streaming_src_observer_->ResetEof();
    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);

    // Waiting for playing is EOF
    for (;;) {
      if (streaming_src_observer_->isEof()) {
        break;
      }
      Thread_Sleep(200);
    }

    // Close directly
    ret = streaming_source_->Close();
    EXPECT_TRUE(ret == agora::rtc::STREAMING_SRC_ERR_NONE);
    AGO_LOG("<TestFileSourceImpl.PlayLoop> file is closed!\n");

    // Unpublish audio & video track
    rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    rte_scene_->UnpublishLocalVideoTrack(video_track_);

    AGO_LOG("<TestFileSourceImpl.PlayLoop> <==Exit\n\n");
  }

  void ListPublish() {
    if (!is_ready) {
      return;
    }

    AGO_LOG("<TestMediaPlayerImpl.List_Publish> ==>Enter\n");
    int ret = 0;
    bool result;

    //
    // Test Case: open play list wit different parameters
    //
    std::shared_ptr<IAgoraRtePlayList> play_list;
    ret = streaming_source_->Open(play_list, 0);  // nullptr
    EXPECT_TRUE(ret == (-agora::ERR_INVALID_ARGUMENT));
    play_list = streaming_source_->CreatePlayList();
    EXPECT_TRUE(play_list != nullptr);
    ret = streaming_source_->Open(play_list, 0);  // no files in list
    EXPECT_TRUE(ret == (-agora::ERR_INVALID_ARGUMENT));

    //
    // publish video track & audio track
    //
    ret = rte_scene_->PublishLocalVideoTrack(stream_id_, video_track_);
    EXPECT_TRUE(ret == agora::ERR_OK);
    ret = rte_scene_->PublishLocalAudioTrack(stream_id_, audio_track_);
    EXPECT_TRUE(ret == agora::ERR_OK);

    //
    // Test Case: loop_count
    //
    RteFileInfo file_info[2];
    ret = play_list->AppendFile(test_file_path1_.c_str(), file_info[0]);
    EXPECT_TRUE(ret == agora::ERR_OK);
    ret = play_list->AppendFile(test_file_path2_.c_str(), file_info[1]);
    EXPECT_TRUE(ret == agora::ERR_OK);
    ret = streaming_source_->SetLoopCount(2);
    EXPECT_TRUE(ret == agora::ERR_OK);
    ret = streaming_source_->Open(play_list, 0, true);
    EXPECT_TRUE(ret == agora::ERR_OK);
    result = streaming_src_observer_->WaitOpenDone(5);
    EXPECT_TRUE(result);
    result = streaming_src_observer_->WaitAllFilesCompleted(60000);
    EXPECT_TRUE(result);
    ret = streaming_source_->Close();
    EXPECT_TRUE(ret == 0);

    //
    // unpublish video track & audio track
    //
    rte_scene_->UnpublishLocalVideoTrack(video_track_);
    rte_scene_->UnpublishLocalAudioTrack(audio_track_);

    AGO_LOG("<TestMediaPlayerImpl.List_Publish> <==Exit\n\n");
  }

  void ListSeek() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestMediaPlayerImpl.ListSeek> ==>Enter\n");
    int ret;

    std::shared_ptr<IAgoraRtePlayList> play_list;
    play_list = streaming_source_->CreatePlayList();
    EXPECT_TRUE(play_list != nullptr);

    //
    // publish video track & audio track
    //
    ret = rte_scene_->PublishLocalVideoTrack(stream_id_, video_track_);
    EXPECT_TRUE(ret == agora::ERR_OK);
    ret = rte_scene_->PublishLocalAudioTrack(stream_id_, audio_track_);
    EXPECT_TRUE(ret == agora::ERR_OK);

    //
    // Test Case: append files
    //
    RteFileInfo file_info[3];
    ret = play_list->ClearFileList();
    EXPECT_TRUE(ret == 0);
    ret = play_list->AppendFile(test_file_path1_.c_str(), file_info[0]);
    EXPECT_TRUE(ret == 0);
    ret = play_list->AppendFile(test_file_path2_.c_str(), file_info[1]);
    EXPECT_TRUE(ret == 0);
    ret = play_list->AppendFile(test_file_path3_.c_str(), file_info[2]);
    EXPECT_TRUE(ret == 0);

    //
    // Test Case: play & pause & resume
    //
    ret = streaming_source_->Open(play_list, 0, false);
    EXPECT_TRUE(ret == 0);
    bool result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == 0);
    Thread_Sleep(2000);
    RteStreamingSourceStatus source_status;
    ret = streaming_source_->GetStreamingSourceStatus(source_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(source_status.source_state,
              agora::rtc::STREAMING_SRC_STATE::STREAMING_SRC_STATE_PLAYING);
    EXPECT_EQ(source_status.curr_file_id, file_info[0].file_id);
    EXPECT_TRUE(source_status.progress_pos > 1000);

    ret = streaming_source_->Pause();
    EXPECT_TRUE(ret == 0);
    Thread_Sleep(500);
    ret = streaming_source_->GetStreamingSourceStatus(source_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(source_status.source_state,
              agora::rtc::STREAMING_SRC_STATE::STREAMING_SRC_STATE_IDLE);
    EXPECT_EQ(source_status.curr_file_id, file_info[0].file_id);

    ret = streaming_source_->Play();
    EXPECT_TRUE(ret == 0);
    Thread_Sleep(500);
    ret = streaming_source_->GetStreamingSourceStatus(source_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(source_status.source_state,
              agora::rtc::STREAMING_SRC_STATE::STREAMING_SRC_STATE_PLAYING);
    EXPECT_EQ(source_status.curr_file_id, file_info[0].file_id);

    //
    // Test Case: Seek()
    //
    ret = streaming_source_->Seek(6000);
    EXPECT_TRUE(ret == 0);
    result = streaming_src_observer_->WaitSeekDone(
        10);  // waiting for seeking finished
    EXPECT_TRUE(result);
    Thread_Sleep(1500);
    int64_t progress = 0;
    ret = streaming_source_->GetCurrPosition(progress);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(progress > 6000);

    //
    // Test Case: SeekToNext()
    //
    ret = streaming_source_->SeekToNext(3000);
    EXPECT_TRUE(ret == 0);
    result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    Thread_Sleep(1500);
    ret = streaming_source_->GetStreamingSourceStatus(source_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(source_status.source_state,
              agora::rtc::STREAMING_SRC_STATE::STREAMING_SRC_STATE_PLAYING);
    EXPECT_EQ(source_status.curr_file_id, file_info[1].file_id);
    EXPECT_TRUE(source_status.progress_pos > 3000);

    //
    // Test Case: SeekToPrev()
    //
    ret = streaming_source_->SeekToPrev(5000);
    EXPECT_TRUE(ret == 0);
    result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    Thread_Sleep(1500);
    ret = streaming_source_->GetStreamingSourceStatus(source_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(source_status.source_state,
              agora::rtc::STREAMING_SRC_STATE::STREAMING_SRC_STATE_PLAYING);
    EXPECT_EQ(source_status.curr_file_id, file_info[0].file_id);
    EXPECT_TRUE(source_status.progress_pos > 5000);

    //
    // Test Case: SeekToPrev()
    //
    ret = streaming_source_->SeekToFile(file_info[2].file_id, 0);
    EXPECT_TRUE(ret == 0);
    result = streaming_src_observer_->WaitOpenDone(10);
    EXPECT_TRUE(result);
    Thread_Sleep(3000);
    ret = streaming_source_->GetStreamingSourceStatus(source_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(source_status.source_state,
              agora::rtc::STREAMING_SRC_STATE::STREAMING_SRC_STATE_PLAYING);
    EXPECT_EQ(source_status.curr_file_id, file_info[2].file_id);
    EXPECT_TRUE(source_status.progress_pos > 1000);

    ret = streaming_source_->Close();
    EXPECT_EQ(ret, 0);

    //
    // unpublish video track & audio track
    //
    rte_scene_->UnpublishLocalVideoTrack(video_track_);
    rte_scene_->UnpublishLocalAudioTrack(audio_track_);
    AGO_LOG("<TestMediaPlayerImpl.ListSeek> <==Exit\n\n");
  }

 private:
  std::shared_ptr<IAgoraRteScene> rte_scene_ = nullptr;
  std::shared_ptr<StreamingSrcObserver> streaming_src_observer_ = nullptr;
  std::shared_ptr<IAgoraRteMediaFactory> media_factory_ = nullptr;
  std::shared_ptr<IAgoraRteStreamingSource> streaming_source_ = nullptr;
  std::shared_ptr<StreamingSrcSceneEventHandler> scene_event_handler_ = nullptr;
  std::shared_ptr<IAgoraRteVideoTrack> video_track_ = nullptr;
  std::shared_ptr<IAgoraRteAudioTrack> audio_track_ = nullptr;

  std::string app_id_ = RteTestGloablSettings::GetAppId();
  std::string token_ = "";
  std::string scene_id_ = "lxh2";
  std::string local_user_id_ = "local_user_2";
  std::string stream_id_ = "local_streaming_source";

  std::string test_file_path1_ = "./test_data/h264_opus_01.mp4";
  std::string test_file_path2_ = "./test_data/h264_opus_02.mp4";
  std::string test_file_path3_ = "./test_data/h264_opus_03.mp4";
  std::string test_file_path4_ = "./test_data/h264_opus_04.mp4";
  std::string test_file_path5_ = "./test_data/h264_opus_05.mp4";

  bool is_ready = false;
};

class RteStreamingSrcTestSmoke : public testing::Test {
 public:
  void SetUp() override { agora_server_.Init(); }

  void TearDown() override { agora_server_.Uninit(); }

 protected:
  TestFileSourceImpl agora_server_;
};

TEST_F(RteStreamingSrcTestSmoke, PlayList) { agora_server_.PlayList(); }

TEST_F(RteStreamingSrcTestSmoke, PlayCtrl) { agora_server_.PlayCtrl(); }

TEST_F(RteStreamingSrcTestSmoke, PlaySeek) { agora_server_.PlaySeek(); }

TEST_F(RteStreamingSrcTestSmoke, PlayLoop) { agora_server_.PlayLoop(); }

TEST_F(RteStreamingSrcTestSmoke, ListPublish) { agora_server_.ListPublish(); }

TEST_F(RteStreamingSrcTestSmoke, ListSeek) { agora_server_.ListSeek(); }

