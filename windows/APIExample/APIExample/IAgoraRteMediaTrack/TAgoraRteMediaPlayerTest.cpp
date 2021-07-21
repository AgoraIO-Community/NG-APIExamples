// ======================= TAgoraRteMediaPlayerTest.cpp ======================= 
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
#include "TestUitls.h"
#include "gtest/gtest.h"

using namespace agora::rte;

#define AGO_LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////
/////////////////// Smoke Testcase for Rte Streaming Source //////////////////
//////////////////////////////////////////////////////////////////////////////

void MpkThread_Sleep(int64_t milili_seconds) {
  std::chrono::milliseconds du(milili_seconds);
  std::this_thread::sleep_for(du);
}

class MpkSceneEventHandler : public IAgoraRteSceneEventHandler {
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
      case ConnectionState::kConnecting:
      case ConnectionState::kReconnecting:
        break;
      case ConnectionState::kConnected:
        connect_success_sempahore.Notify();
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
      CDNBYPASS_STREAM_PUBLISH_ERROR err_code) override {}

  void OnBypassCdnPublished(const std::string& stream_id,
                            const std::string& target_cdn_url,
                            CDNBYPASS_STREAM_PUBLISH_ERROR error) override{};

  void OnBypassCdnUnpublished(const std::string& stream_id,
                              const std::string& target_cdn_url) override {}

  void OnBypassTranscodingUpdated(const std::string& stream_id) override{};

  void OnSceneStats(const SceneStats& stats) override {}

  void OnLocalStreamStats(const std::string& stream_id,
                          const LocalStreamStats& stats) override {}

  void OnRemoteStreamStats(const std::string& stream_id,
                           const RemoteStreamStats& stats) override {}

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

class MediaPlayerObserver final : public IAgoraRteMediaPlayerObserver {
 public:
  MediaPlayerObserver(std::shared_ptr<IAgoraRteMediaPlayer> player) {
    player_ = player;
  }

  ~MediaPlayerObserver() = default;

  bool WaitFileOpened(int32_t timeout_seconds) {
    int wait_ret = opened_semaphore_.Wait(timeout_seconds);
    return (wait_ret == 1) ? true : false;
  }

  bool WaitSeeked(int32_t timeout_seconds) {
    int wait_ret = seeked_semaphore_.Wait(timeout_seconds);
    return (wait_ret == 1) ? true : false;
  }

  bool WaitPlaybackCompleted(int32_t timeout_seconds) {
    int wait_ret = play_completed_semaphore_.Wait(timeout_seconds);
    return (wait_ret == 1) ? true : false;
  }

  bool WaitAllFilesCompleted(int32_t timeout_seconds) {
    int wait_ret = allfiles_completed_semaphore_.Wait(timeout_seconds);
    return (wait_ret == 1) ? true : false;
  }

 public:
  void OnPlayerStateChanged(const RteFileInfo& current_file_info,
                            MEDIA_PLAYER_STATE state,
                            MEDIA_PLAYER_ERROR ec) override {
    AGO_LOG("<OnPlayerStateChanged> state=%d, err_code=%d\n",
            static_cast<int>(state), static_cast<int>(ec));

    switch (state) {
      case MEDIA_PLAYER_STATE::PLAYER_STATE_OPEN_COMPLETED:
        opened_semaphore_.Notify();
        break;
      case MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYBACK_COMPLETED:
        play_completed_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED:
        play_completed_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_IDLE:
      case MEDIA_PLAYER_STATE::PLAYER_STATE_OPENING:
      case MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING:
      case MEDIA_PLAYER_STATE::PLAYER_STATE_PAUSED:
      case MEDIA_PLAYER_STATE::PLAYER_STATE_STOPPED:
      case MEDIA_PLAYER_STATE::PLAYER_STATE_FAILED:
      default:
        break;
    }
  }

  void OnPositionChanged(const RteFileInfo& current_file_info,
                         int64_t position) override {
    // AGO_LOG("<OnPositionChanged> position=%ld\n",
    // static_cast<long>(position));
  }

  void OnPlayerEvent(const RteFileInfo& current_file_info,
                     MEDIA_PLAYER_EVENT event) override {
    AGO_LOG("<OnPlayerEvent> event=%d\n", static_cast<int>(event));

    switch (event) {
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_COMPLETE:
        seeked_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_ERROR:
        seeked_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_BEGIN:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_VIDEO_PUBLISHED:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_AUDIO_PUBLISHED:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_AUDIO_TRACK_CHANGED:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_BUFFER_LOW:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_BUFFER_RECOVER:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_FREEZE_START:
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_FREEZE_STOP:
      default:
        break;
    }
  }

  void OnAllMediasCompleted(int32_t err_code) override {
    AGO_LOG("<OnAllMediasCompleted> err_code=%d\n", err_code);
    allfiles_completed_semaphore_.Notify();
  }

  void OnMetadata(const RteFileInfo& current_file_info,
                  MEDIA_PLAYER_METADATA_TYPE type, const uint8_t* data,
                  uint32_t length) override {}

  void OnPlayerBufferUpdated(const RteFileInfo& current_file_info,
                             int64_t playCachedBuffer) override {}

  void OnAudioFrame(const RteFileInfo& current_file_info,
                    const AudioPcmFrame& audio_frame) override {
    // AGO_LOG("<OnAudioFrame>\n");
  }

  void OnVideoFrame(const RteFileInfo& current_file_info,
                    const VideoFrame& video_frame) override {
    // AGO_LOG("<OnVideoFrame>\n");
  }

 private:
  agora_test_semaphore opened_semaphore_;
  agora_test_semaphore seeked_semaphore_;
  agora_test_semaphore play_completed_semaphore_;
  agora_test_semaphore allfiles_completed_semaphore_;

  std::weak_ptr<IAgoraRteMediaPlayer> player_;
};

class TestMediaPlayerImpl {
 public:
  bool Init() {
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
    scene_event_handler_ = std::make_shared<MpkSceneEventHandler>();
    EXPECT_TRUE(scene_event_handler_ != nullptr);
    SceneConfig config;
    rte_scene_ = AgoraRteSDK::CreateRteScene(scene_id_, config);
    EXPECT_TRUE(rte_scene_ != nullptr);
    rte_scene_->RegisterEventHandler(scene_event_handler_);

    media_factory_ = AgoraRteSDK::GetRteMediaFactory();
    EXPECT_TRUE(media_factory_ != nullptr);

    media_player_ = media_factory_->CreateMediaPlayer();
    EXPECT_TRUE(media_player_ != nullptr);

    player_observer_ = std::make_shared<MediaPlayerObserver>(media_player_);
    EXPECT_TRUE(player_observer_ != nullptr);
    int ret = media_player_->RegisterMediaPlayerObserver(player_observer_);
    EXPECT_EQ(ret, 0);

    ret = media_player_->Mute(true);
    EXPECT_EQ(ret, 0);

    //
    // Join scene and sync waiting for join successful
    //
    JoinOptions option;
    option.is_user_visible_to_remote = true;
    ret = rte_scene_->Join(local_user_id_, token_, option);
    EXPECT_EQ(ret, 0);
    ret = scene_event_handler_->connect_success_sempahore.Wait(8);
    EXPECT_EQ(ret, 1);

    // Config the publish options
    agora::rte::RtcStreamOptions pub_option(token_);
    pub_option.type = StreamType::kRtcStream;
    ret = rte_scene_->CreateOrUpdateRTCStream(stream_id_, pub_option);
    EXPECT_EQ(ret, 0);

    // publish video track & audio track
    ret = rte_scene_->PublishMediaPlayer(stream_id_, media_player_);
    EXPECT_EQ(ret, 0);

    is_ready = true;
    AGO_LOG("<TestMediaPlayerImpl.Init> finished, Test is ready\n\n");
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
    if (media_player_ != nullptr) {
      media_player_->UnregisterMediaPlayerObserver(player_observer_);
      player_observer_.reset();
      rte_scene_->UnpublishMediaPlayer(media_player_);
      media_player_.reset();
    }

    media_factory_.reset();
    rte_scene_.reset();
    AgoraRteSDK::Deinit();

    is_ready = false;
    AGO_LOG("<TestMediaPlayerImpl.Uninit> finished, Test is Done\n\n");
  }

  void PlaySingleFile() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestMediaPlayerImpl.PlaySingleFile> ==>Enter\n");
    int ret = 0;
    bool result;

    // Open file
    ret = media_player_->Open(test_file_path1_, 0);
    EXPECT_TRUE(ret == 0);
    result = player_observer_->WaitFileOpened(10);
    EXPECT_TRUE(result);

    // Mute the local playing
    ret = media_player_->Mute(true);
    EXPECT_TRUE(ret == 0);

    ret = media_player_->Play();
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(2000);
    int64_t progress = 0;
    media_player_->GetPlayPosition(progress);
    EXPECT_TRUE(progress > 1000);
    MEDIA_PLAYER_STATE state = media_player_->GetState();
    EXPECT_EQ(state, MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);

    ret = media_player_->Pause();
    EXPECT_TRUE(ret == 0);
    state = media_player_->GetState();
    EXPECT_EQ(state, MEDIA_PLAYER_STATE::PLAYER_STATE_PAUSED);

    ret = media_player_->Resume();
    EXPECT_TRUE(ret == 0);
    ret = media_player_->MuteAudio(true);
    EXPECT_TRUE(ret == 0);
    ret = media_player_->MuteVideo(true);
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(1000);
    state = media_player_->GetState();
    EXPECT_EQ(state, MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);

    ret = media_player_->MuteAudio(false);
    EXPECT_TRUE(ret == 0);
    ret = media_player_->MuteVideo(false);
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(1000);
    ret = media_player_->Stop();
    EXPECT_TRUE(ret == 0);
    state = media_player_->GetState();
    EXPECT_EQ(state, MEDIA_PLAYER_STATE::PLAYER_STATE_IDLE);

    AGO_LOG("<TestMediaPlayerImpl.PlaySingleFile> <==Exit\n\n");
  }

  void PlayList() {
    if (!is_ready) {
      return;
    }

    AGO_LOG("<TestMediaPlayerImpl.PlayList> ==>Enter\n");
    int ret = 0;
    bool result;

    //
    // Test Case: open play list wit different parameters
    //
    std::shared_ptr<IAgoraRtePlayList> play_list;
    ret = media_player_->Open(play_list, 0);  // nullptr
    EXPECT_TRUE(ret == (-agora::ERR_INVALID_ARGUMENT));
    play_list = media_player_->CreatePlayList();
    EXPECT_TRUE(play_list != nullptr);
    ret = media_player_->Open(play_list, 0);  // no files in list
    EXPECT_TRUE(ret == (-agora::ERR_INVALID_ARGUMENT));

    //
    // Test Case: loop_count
    //
    RteFileInfo file_info[5];
    ret = play_list->AppendFile(test_file_path1_.c_str(), file_info[0]);
    EXPECT_TRUE(ret == 0);
    ret = play_list->AppendFile(test_file_path2_.c_str(), file_info[1]);
    EXPECT_TRUE(ret == 0);
    ret = media_player_->Open(play_list, 0);
    EXPECT_TRUE(ret == 0);
    result = player_observer_->WaitFileOpened(10);
    EXPECT_TRUE(result);
    ret = media_player_->SetLoopCount(2);
    EXPECT_TRUE(ret == 0);
    ret = media_player_->Play();
    EXPECT_TRUE(ret == 0);
    result = player_observer_->WaitAllFilesCompleted(60000);
    EXPECT_TRUE(result);
    ret = media_player_->Stop();
    EXPECT_TRUE(ret == 0);

    AGO_LOG("<TestMediaPlayerImpl.PlayList> <==Exit\n\n");
  }

  void PlaySeek() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> ==>Enter\n");

    std::shared_ptr<IAgoraRtePlayList> play_list;
    play_list = media_player_->CreatePlayList();
    EXPECT_TRUE(play_list != nullptr);

    //
    // Test Case: append files
    //
    RteFileInfo file_info[3];
    int ret = play_list->ClearFileList();
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
    ret = media_player_->Open(play_list, 0);
    EXPECT_TRUE(ret == 0);
    bool result = player_observer_->WaitFileOpened(10);
    EXPECT_TRUE(result);
    ret = media_player_->Play();
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(2000);
    RtePlayerStatus player_status;
    ret = media_player_->GetPlayerStatus(player_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(player_status.player_state,
              MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);
    EXPECT_EQ(player_status.curr_file_id, file_info[0].file_id);
    EXPECT_TRUE(player_status.progress_pos > 1000);

    ret = media_player_->Pause();
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(500);
    ret = media_player_->GetPlayerStatus(player_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(player_status.player_state,
              MEDIA_PLAYER_STATE::PLAYER_STATE_PAUSED);
    EXPECT_EQ(player_status.curr_file_id, file_info[0].file_id);

    ret = media_player_->Resume();
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(500);
    ret = media_player_->GetPlayerStatus(player_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(player_status.player_state,
              MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);
    EXPECT_EQ(player_status.curr_file_id, file_info[0].file_id);

    //
    // Test Case: Seek()
    //
    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> seek to 6000\n");
    ret = media_player_->Seek(6000);
    EXPECT_TRUE(ret == 0);
    MpkThread_Sleep(1500);
    int64_t progress = 0;
    ret = media_player_->GetPlayPosition(progress);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(progress > 6000);

    //
    // Test Case: SeekToNext()
    //
    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> seek to next 3000\n");
    ret = media_player_->SeekToNext(3000);
    EXPECT_TRUE(ret == 0);
    result = player_observer_->WaitFileOpened(10);
    EXPECT_TRUE(result);
    MpkThread_Sleep(1500);
    ret = media_player_->GetPlayerStatus(player_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(player_status.player_state,
              MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);
    EXPECT_EQ(player_status.curr_file_id, file_info[1].file_id);
    EXPECT_TRUE(player_status.progress_pos > 3000);

    //
    // Test Case: SeekToPrev()
    //
    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> seek to prev 5000\n");
    ret = media_player_->SeekToPrev(5000);
    EXPECT_TRUE(ret == 0);
    result = player_observer_->WaitFileOpened(10);
    EXPECT_TRUE(result);
    MpkThread_Sleep(1500);
    ret = media_player_->GetPlayerStatus(player_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(player_status.player_state,
              MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);
    EXPECT_EQ(player_status.curr_file_id, file_info[0].file_id);
    EXPECT_TRUE(player_status.progress_pos > 5000);

    //
    // Test Case: SeekToFile()
    //
    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> seek to file 0\n");
    ret = media_player_->SeekToFile(file_info[2].file_id, 0);
    EXPECT_TRUE(ret == 0);
    result = player_observer_->WaitFileOpened(10);
    EXPECT_TRUE(result);
    MpkThread_Sleep(3000);
    ret = media_player_->GetPlayerStatus(player_status);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(player_status.player_state,
              MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING);
    EXPECT_EQ(player_status.curr_file_id, file_info[2].file_id);
    EXPECT_TRUE(player_status.progress_pos > 1000);

    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> stopping...\n");
    ret = media_player_->Stop();
    EXPECT_EQ(ret, 0);

    AGO_LOG("<TestMediaPlayerImpl.PlaySeek> <==Exit\n\n");
  }

  void PlayListMgr() {
    if (!is_ready) {
      return;
    }
    AGO_LOG("<TestMediaPlayerImpl.PlayListMgr> ==>Enter\n");

    std::shared_ptr<IAgoraRtePlayList> list_interface =
        media_player_->CreatePlayList();
    std::shared_ptr<AgoraRtePlayList> play_list =
        std::static_pointer_cast<AgoraRtePlayList>(list_interface);
    EXPECT_TRUE(play_list != nullptr);

    //
    // Test Case: invalid file url
    //
    RteFileInfo file_info_none;
    int ret = play_list->AppendFile(nullptr, file_info_none);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->AppendFile("", file_info_none);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->InsertFile(nullptr, 2, file_info_none);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->InsertFile("", 3, file_info_none);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);

    //
    // Test Case: insert into list, the result file order is:
    //

    // Result: file_1;
    RteFileInfo file_1;
    ret = play_list->AppendFile(test_file_path1_.c_str(), file_1);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_1.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_1.index == 0);

    // Result: file_1; file_2;
    RteFileInfo file_2;
    ret = play_list->AppendFile(test_file_path2_.c_str(), file_2);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_2.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_2.index == 1);

    // Result: file_3; file_1; file_2;
    RteFileInfo file_3;
    ret = play_list->InsertFile(test_file_path3_.c_str(), -2, file_3);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_3.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_3.index == 0);

    // Result: file_3; file_1; file_2; file_4;
    RteFileInfo file_4;
    ret = play_list->InsertFile(test_file_path4_.c_str(), 100, file_4);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_4.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_4.index == 3);

    // Result: file_3; file_1; file_5; file_2; file_4;
    RteFileInfo file_5;
    ret = play_list->InsertFile(test_file_path5_.c_str(), 2, file_5);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_5.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_5.index == 2);

    // Result: file_3; file_1; file_5; file_2; file_1B; file_4;
    RteFileInfo file_1B;
    ret = play_list->InsertFile(test_file_path1_.c_str(), 4, file_1B);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_1B.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_1B.index == 4);

    // Result: file_3; file_2B; file_1; file_5; file_2; file_1B; file_4;
    RteFileInfo file_2B;
    ret = play_list->InsertFile(test_file_path2_.c_str(), 1, file_2B);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_2B.file_id != INVALID_RTE_FILE_ID);
    EXPECT_TRUE(file_2B.index == 1);
    int32_t file_count = play_list->GetFileCount();
    EXPECT_TRUE(file_count == 7);

    //
    // Test Case: remove from list
    //
    std::vector<RteFileInfo> file_list;

    //  Result: file_3; file_2B; file_1; file_5; file_2; file_1B;
    ret = play_list->RemoveFileById(999);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->RemoveFileById(file_4.file_id);
    EXPECT_EQ(ret, 0);
    file_list.clear();
    ret = play_list->GetFileList(file_list);
    EXPECT_TRUE(ret == 0);
    EXPECT_TRUE(file_list.size() == 6);
    EXPECT_TRUE(file_list[0].file_id == file_3.file_id);
    EXPECT_TRUE(file_list[5].file_id == file_1B.file_id);

    //  Result: file_3; file_2B; file_1; file_2; file_1B;
    ret = play_list->RemoveFileByIndex(-3);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->RemoveFileByIndex(6);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->RemoveFileByIndex(3);
    EXPECT_EQ(ret, 0);
    file_list.clear();
    ret = play_list->GetFileList(file_list);
    EXPECT_TRUE(ret == 0);
    EXPECT_TRUE(file_list.size() == 5);
    EXPECT_TRUE(file_list[3].file_id == file_2.file_id);
    EXPECT_TRUE(file_list[4].file_id == file_1B.file_id);

    //  Result: file_3; file_1; file_1B;
    ret = play_list->RemoveFileByUrl(nullptr);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->RemoveFileByUrl("");
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->RemoveFileByUrl("./test_data/empty.mp4");
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->RemoveFileByUrl(test_file_path2_.c_str());
    EXPECT_EQ(ret, 0);
    file_list.clear();
    ret = play_list->GetFileList(file_list);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(file_list.size() == 3);
    EXPECT_TRUE(file_list[0].file_id == file_3.file_id);
    EXPECT_TRUE(file_list[2].file_id == file_1B.file_id);
    ret = play_list->ClearFileList();
    EXPECT_TRUE(ret == 0);

    //
    // TestCase: set current item for list
    //
    RteFileInfo file_info[5];
    ret = play_list->AppendFile(test_file_path1_.c_str(), file_info[0]);
    EXPECT_EQ(ret, 0);
    ret = play_list->AppendFile(test_file_path2_.c_str(), file_info[1]);
    EXPECT_EQ(ret, 0);
    ret = play_list->AppendFile(test_file_path3_.c_str(), file_info[2]);
    EXPECT_EQ(ret, 0);

    RteFileInfo current_file_info;
    ret = play_list->GetCurrentFileInfo(current_file_info);
    EXPECT_EQ(ret, -agora::ERR_INVALID_STATE);
    ret = play_list->SetCurrFileById(9999);
    EXPECT_EQ(ret, -agora::ERR_INVALID_ARGUMENT);
    ret = play_list->SetCurrFileById(file_info[0].file_id);
    EXPECT_EQ(ret, 0);
    ret = play_list->GetCurrentFileInfo(current_file_info);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(current_file_info.file_id, file_info[0].file_id);
    bool is_first = play_list->CurrentIsFirstFile();
    EXPECT_EQ(is_first, true);

    ret = play_list->MoveCurrentToNext(false);
    EXPECT_EQ(ret, 0);
    play_list->GetCurrentFileInfo(current_file_info);
    EXPECT_EQ(current_file_info.file_id, file_info[1].file_id);
    ret = play_list->MoveCurrentToPrev(false);
    EXPECT_EQ(ret, 0);
    ret = play_list->MoveCurrentToPrev(false);
    EXPECT_EQ(ret, -agora::ERR_INVALID_STATE);
    ret = play_list->MoveCurrentToPrev(true);
    EXPECT_EQ(ret, 0);
    play_list->GetCurrentFileInfo(current_file_info);
    EXPECT_EQ(current_file_info.file_id, file_info[2].file_id);
    bool is_last = play_list->CurrentIsLastFile();
    EXPECT_EQ(is_last, true);

    AGO_LOG("<TestMediaPlayerImpl.PlayListMgr> <==Exit\n\n");
  }

 private:
  std::shared_ptr<IAgoraRteScene> rte_scene_ = nullptr;
  std::shared_ptr<MpkSceneEventHandler> scene_event_handler_ = nullptr;
  std::shared_ptr<IAgoraRteMediaFactory> media_factory_ = nullptr;
  std::shared_ptr<IAgoraRteMediaPlayer> media_player_ = nullptr;
  std::shared_ptr<MediaPlayerObserver> player_observer_ = nullptr;

  std::string app_id_ = RteTestGloablSettings::GetAppId();
  std::string token_ = "";
  std::string scene_id_ = "lxh4";
  std::string local_user_id_ = "local_user_4";
  std::string stream_id_ = "local_media_player";

  std::string test_file_path1_ = "./test_data/h264_opus_01.mp4";
  std::string test_file_path2_ = "./test_data/h264_opus_02.mp4";
  std::string test_file_path3_ = "./test_data/h264_opus_03.mp4";
  std::string test_file_path4_ = "./test_data/h264_opus_04.mp4";
  std::string test_file_path5_ = "./test_data/h264_opus_05.mp4";

  bool is_ready = false;
};

class MediaPlayerTestSmoke : public testing::Test {
 public:
  void SetUp() override { mpk_tester_.Init(); }

  void TearDown() override { mpk_tester_.Uninit(); }

 protected:
  TestMediaPlayerImpl mpk_tester_;
};

TEST_F(MediaPlayerTestSmoke, PlayListMgr) { mpk_tester_.PlayListMgr(); }

TEST_F(MediaPlayerTestSmoke, PlaySingleFile) { mpk_tester_.PlaySingleFile(); }

TEST_F(MediaPlayerTestSmoke, PlayList) { mpk_tester_.PlayList(); }

TEST_F(MediaPlayerTestSmoke, PlaySeek) { mpk_tester_.PlaySeek(); }

