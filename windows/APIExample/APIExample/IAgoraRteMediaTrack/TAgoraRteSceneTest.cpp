// ======================= TAgoraRteSceneTest.cpp ======================= 
// AgoraRte.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//
#include <algorithm>
#include <random>
#include <stdexcept>
#include <thread>

#include "AgoraBase.h"
#include "AgoraRteSdkImpl.hpp" 
#include "IAgoraRteDeviceManager.h"
#include "IAgoraRteMediaFactory.h"
#include "IAgoraRteMediaObserver.h"
#include "IAgoraRteMediaPlayer.h"
#include "IAgoraRteMediaTrack.h"
#include "IAgoraRteScene.h"
#include "TestUitls.h"
#include "gtest/gtest.h"

using namespace agora::rte;

struct RteFakeVideoFrame : public agora::rte::ExternalVideoFrame {
 public:
  RteFakeVideoFrame(int w, int h,
                    agora::media::base::VIDEO_PIXEL_FORMAT formatType) {
    format = static_cast<agora::media::base::VIDEO_PIXEL_FORMAT>(formatType);
    stride = w;
    height = h;
    cropLeft = 0;
    cropRight = 0;
    cropTop = 0;
    cropBottom = 0;
    rotation = 0;
    timestamp = 0;
    double m = 0.0;
    switch (formatType) {
      case agora::media::base::VIDEO_PIXEL_I420: {
        m = 1.5;
        break;
      }
      case agora::media::base::VIDEO_PIXEL_I422: {
        m = 2;
        break;
      }
      case agora::media::base::VIDEO_PIXEL_RGBA: {
        m = 4;
        break;
        default:
          break;
      }
    }
    unsigned int len = static_cast<unsigned int>(w * h * m);

    video_buffer.resize(len);
    buffer = static_cast<void*>(&video_buffer[0]);
    memset(buffer, 0, len);
  }

  ~RteFakeVideoFrame() {}

  std::vector<uint8_t> video_buffer;
};

struct RteFakeAudioFrame : public agora::rte::AudioFrame {
 public:
  RteFakeAudioFrame() {
    int sample_rate = 48000;
    int channel_num = 1;

    int lengthInByte = sample_rate / 1000 * sizeof(int16_t) * channel_num * 10;
    int samplesPerChannel = lengthInByte / sizeof(int16_t) / channel_num;

    audio_buffer.resize(samplesPerChannel * channel_num);
    agora::media::IAudioFrameObserver::AudioFrame audio_frame;
    this->type = ::agora::media::IAudioFrameObserver::FRAME_TYPE_PCM16;
    this->samplesPerChannel = samplesPerChannel;
    this->bytesPerSample = agora::rtc::TWO_BYTES_PER_SAMPLE;
    this->channels = channel_num;
    this->samplesPerSec = sample_rate;
    this->buffer = reinterpret_cast<void*>(&audio_buffer[0]);
    this->renderTimeMs = 0;
    this->avsync_type = 0;
  }

  ~RteFakeAudioFrame() {}

  std::vector<int16_t> audio_buffer;
};

class MediaPlayerObserverSimple final : public IAgoraRteMediaPlayerObserver {
 public:
  MediaPlayerObserverSimple(std::shared_ptr<IAgoraRteMediaPlayer> player) {
    player_ = player;
  }

  ~MediaPlayerObserverSimple() = default;

  bool WaitFileOpened(int32_t timeout_seconds) {
    int wait_ret = opened_semaphore_.Wait(timeout_seconds);
    return (wait_ret == 1) ? true : false;
  }

 public:
  void OnPlayerStateChanged(const RteFileInfo& current_file_info,
                            MEDIA_PLAYER_STATE state,
                            MEDIA_PLAYER_ERROR ec) override {
    switch (state) {
      case MEDIA_PLAYER_STATE::PLAYER_STATE_IDLE:
        RTE_LOG_VERBOSE << "PLAYER_STATE_IDLE";
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_OPENING:
        RTE_LOG_VERBOSE << "PLAYER_STATE_OPENING";
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_OPEN_COMPLETED:
        RTE_LOG_VERBOSE << "PLAYER_STATE_OPEN_COMPLETED";
        opened_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYING:
        RTE_LOG_VERBOSE << "PLAYER_STATE_PLAYING";
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_PAUSED:
        RTE_LOG_VERBOSE << "PLAYER_STATE_PAUSED";
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYBACK_COMPLETED:
        RTE_LOG_VERBOSE << "PLAYER_STATE_PLAYBACK_COMPLETED";
        play_completed_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED:
        RTE_LOG_VERBOSE << "PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED";
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_STOPPED:
        RTE_LOG_VERBOSE << "PLAYER_STATE_STOPPED";
        break;

      case MEDIA_PLAYER_STATE::PLAYER_STATE_FAILED:
        RTE_LOG_VERBOSE << "PLAYER_STATE_FAILED";
        break;

      default:
        break;
    }
  }

  void OnPositionChanged(const RteFileInfo& current_file_info,
                         int64_t position) override {
    RTE_LOG_VERBOSE << "position: " << position;
  }

  void OnPlayerEvent(const RteFileInfo& current_file_info,
                     MEDIA_PLAYER_EVENT event) override {
    switch (event) {
      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_BEGIN:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_SEEK_BEGIN";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_COMPLETE:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_SEEK_COMPLETE";
        seeked_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_ERROR:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_SEEK_ERROR";
        seeked_semaphore_.Notify();
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_VIDEO_PUBLISHED:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_VIDEO_PUBLISHED";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_AUDIO_PUBLISHED:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_AUDIO_PUBLISHED";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_AUDIO_TRACK_CHANGED:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_AUDIO_TRACK_CHANGED";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_BUFFER_LOW:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_BUFFER_LOW";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_BUFFER_RECOVER:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_BUFFER_RECOVER";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_FREEZE_START:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_FREEZE_START";
        break;

      case MEDIA_PLAYER_EVENT::PLAYER_EVENT_FREEZE_STOP:
        RTE_LOG_VERBOSE << "PLAYER_EVENT_FREEZE_STOP";
        break;

      default:
        break;
    }
  }

  void OnAllMediasCompleted(int32_t err_code) override {
    RTE_LOG_VERBOSE << "OnAllMediasCompleted";
  }

  void OnMetadata(const RteFileInfo& current_file_info,
                  MEDIA_PLAYER_METADATA_TYPE type, const uint8_t* data,
                  uint32_t length) override {
    RTE_LOG_VERBOSE << "OnMetadata";
  }

  void OnPlayerBufferUpdated(const RteFileInfo& current_file_info,
                             int64_t playCachedBuffer) override {
    RTE_LOG_VERBOSE << "OnPlayerBufferUpdated";
  }

  void OnAudioFrame(const RteFileInfo& current_file_info,
                    const AudioPcmFrame& audio_frame) override {
    // RTE_LOG_VERBOSE << "OnAudioFrame";
  }

  void OnVideoFrame(const RteFileInfo& current_file_info,
                    const VideoFrame& video_frame) override {
    // RTE_LOG_VERBOSE << "OnVideoFrame";
  }

 private:
  agora_test_semaphore opened_semaphore_;
  agora_test_semaphore seeked_semaphore_;
  agora_test_semaphore closed_semaphore_;
  agora_test_semaphore play_completed_semaphore_;

  std::weak_ptr<IAgoraRteMediaPlayer> player_;
};

template <typename track_type>
class TestTrackWithThread {
 public:
  using TrackTheadRoutine = std::function<void()>;

  TestTrackWithThread(std::shared_ptr<track_type> input_track) {
    track = input_track;
  }

  void SetRoutine(TrackTheadRoutine&& routine) {
    routine_ = std::move(routine);
  }

  virtual ~TestTrackWithThread() { Stop(); }

  int Start() {
    if (should_run) return agora::ERR_OK;

    should_run = true;
    if (track) {
      internal_thread = std::make_shared<std::thread>(routine_);
    } else {
      assert(false);
      should_run = false;
      return -agora::ERR_FAILED;
    }
    return agora::ERR_OK;
  }

  bool ShouldRun() { return should_run && track; }

  int Stop() {
    if (should_run) {
      should_run = false;

      if (track) {
        if (internal_thread) {
          internal_thread->join();
        }
      } else {
        should_run = true;
        return -agora::ERR_FAILED;
      }
    }

    return agora::ERR_OK;
  }

  auto GetTrack() { return std::static_pointer_cast<track_type>(track); }

  std::shared_ptr<track_type> track = nullptr;
  std::shared_ptr<std::thread> internal_thread = nullptr;
  std::atomic<bool> should_run = {false};
  TrackTheadRoutine routine_ = nullptr;
};

auto CreateTestCustomVideoTrack() {
  auto track = AgoraRteSDK::GetRteMediaFactory()->CreateCustomVideoTrack();
  auto custom_video_track =
    std::make_shared<TestTrackWithThread<IAgoraRteCustomVideoTrack>>(track);
  auto routine = [thiz = custom_video_track.get()]() {
    std::shared_ptr<RteFakeVideoFrame> frame =
      std::make_shared<RteFakeVideoFrame>(
        1280, 720, agora::media::base::VIDEO_PIXEL_I420);
    ExternalVideoFrame* my_frame = frame.get();
    int result = agora::ERR_OK;

    bool continue_run = thiz->ShouldRun();
    while (continue_run) {
      RTE_LOG_INFO << "Sending video for stream :"
                   << thiz->track->GetAttachedStreamId();

      result = thiz->track->PushVideoFrame(*my_frame);

      EXPECT_EQ(result, agora::ERR_OK);
      std::this_thread::sleep_for(std::chrono::seconds(1));

      continue_run = thiz->ShouldRun();

      if (!continue_run) {
        RTE_LOG_INFO << "Stop stream :" << thiz->track->GetAttachedStreamId();
      }
    }
  };

  custom_video_track->SetRoutine(routine);

  return custom_video_track;
}

auto CreateTestCustomAudioTrack() {
  auto track = AgoraRteSDK::GetRteMediaFactory()->CreateCustomAudioTrack();
  auto custom_audio_track =
    std::make_shared<TestTrackWithThread<IAgoraRteCustomAudioTrack>>(track);
  auto routine = [thiz = custom_audio_track.get()]() {
    auto frame = std::make_shared<RteFakeAudioFrame>();
    AudioFrame* my_frame = frame.get();
    int result = agora::ERR_OK;

    while (thiz->ShouldRun()) {
      RTE_LOG_INFO << "Sending audio for stream :"
                   << thiz->track->GetAttachedStreamId();
      result = thiz->track->PushAudioFrame(*my_frame);
      EXPECT_EQ(result, agora::ERR_OK);

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  };

  custom_audio_track->SetRoutine(routine);

  return custom_audio_track;
}

#if defined(WIN32)
const std::string GetExeRunningFolder() {
  char fileName[MAX_PATH];
  GetModuleFileNameA(NULL, fileName, MAX_PATH);

  std::string currentPath = fileName;
  return currentPath.substr(0, currentPath.find_last_of("\\"));
}
#endif

class RteSceneEventHandler : public agora::rte::IAgoraRteSceneEventHandler {
 public:
  virtual ~RteSceneEventHandler() = default;

  agora_test_semaphore connection_stat_changed_semaphore;
  agora_test_semaphore remote_user_joined_semaphore;
  agora_test_semaphore remote_user_left_semaphore;
  agora_test_semaphore remote_stream_added_semaphore;
  agora_test_semaphore remote_stream_removed_semaphore;
  agora_test_semaphore connect_success_semaphore;
  agora_test_semaphore connect_gone_semaphore;

  agora_test_semaphore local_stream_state_changed_semaphore;
  agora_test_semaphore remote_stream_state_changed_semaphore;

  std::set<std::string> remote_streams;
  std::set<std::string> remote_users;

  std::set<std::string> local_audio_good_streams;
  std::set<std::string> local_video_good_streams;

  std::set<std::string> remote_audio_good_streams;
  std::set<std::string> remote_video_good_streams;

  // Inherited via IAgoraRteSceneEventHandler
  void OnConnectionStateChanged(ConnectionState old_state,
                                ConnectionState new_state,
                                ConnectionChangedReason reason) override {
    RTE_LOG_INFO << "ConState: " << static_cast<int>(new_state);

    connection_stat_changed_semaphore.Notify();

    switch (new_state) {
      case agora::rte::ConnectionState::kDisconnecting:
      case agora::rte::ConnectionState::kConnecting:
      case agora::rte::ConnectionState::kReconnecting:
        break;
      case agora::rte::ConnectionState::kConnected:
        connect_success_semaphore.Notify();
        break;
      case agora::rte::ConnectionState::kFailed:
      case agora::rte::ConnectionState::kDisconnected:
        connect_gone_semaphore.Notify();
        break;
      default:
        break;
    }
  }

  void OnRemoteUserJoined(const std::vector<UserInfo>& users) override {
    RTE_LOG_INFO << "Existing user: ";

    for (auto& user : remote_users) {
      RTE_LOG_INFO << user << " , ";
    }

    RTE_LOG_INFO << ". Adding user: " << users[0].user_id;

    remote_users.emplace(users[0].user_id);

    remote_user_joined_semaphore.Notify();
  }

  void OnRemoteUserLeft(const std::vector<UserInfo>& users) override {
    RTE_LOG_INFO << "Existing user: ";
    for (auto& user : remote_users) {
      RTE_LOG_INFO << user << " , ";
    }
    RTE_LOG_INFO << ". Removing user: " << users[0].user_id;

    remote_users.erase(users[0].user_id);
    remote_user_left_semaphore.Notify();
  }

  void OnRemoteStreamAdded(const std::vector<StreamInfo>& streams) override {
    RTE_LOG_INFO << "Existing stream: ";
    for (auto& stream : remote_streams) {
      RTE_LOG_INFO << stream << " , ";
    }

    RTE_LOG_INFO << ". Adding stream: " << streams[0].stream_id;

    remote_streams.emplace(streams[0].stream_id);

    remote_stream_added_semaphore.Notify();
  }

  void OnRemoteStreamRemoved(const std::vector<StreamInfo>& streams) override {
    RTE_LOG_INFO << "Existing stream: ";
    for (auto& stream : remote_streams) {
      RTE_LOG_INFO << stream << " , ";
    }

    RTE_LOG_INFO << ". Removing stream: " << streams[0].stream_id;

    remote_streams.erase(streams[0].stream_id);

    remote_stream_removed_semaphore.Notify();
  }

  void OnLocalStreamStateChanged(const StreamInfo& streams,
                                 MediaType media_type,
                                 StreamMediaState old_state,
                                 StreamMediaState new_state,
                                 StreamStateChangedReason reason) override {
    if (new_state == StreamMediaState::kStreaming) {
      switch (media_type) {
        case MediaType::kAudio:
          if (local_audio_good_streams.find(streams.stream_id) ==
              local_audio_good_streams.end()) {
            local_audio_good_streams.emplace(streams.stream_id);
          }

          break;
        case MediaType::kVideo:
          if (local_video_good_streams.find(streams.stream_id) ==
              local_video_good_streams.end()) {
            local_video_good_streams.emplace(streams.stream_id);
          }

          break;
      }
    } else {
      switch (media_type) {
        case MediaType::kAudio:
          local_audio_good_streams.erase(streams.stream_id);
          break;
        case MediaType::kVideo:
          local_video_good_streams.erase(streams.stream_id);
          break;
      }
    }

    local_stream_state_changed_semaphore.Notify();
  }

  void OnRemoteStreamStateChanged(const StreamInfo& streams,
                                  MediaType media_type,
                                  StreamMediaState old_state,
                                  StreamMediaState new_state,
                                  StreamStateChangedReason reason) override {
    if (new_state == StreamMediaState::kStreaming) {
      switch (media_type) {
        case MediaType::kAudio:
          if (remote_audio_good_streams.find(streams.stream_id) ==
              remote_audio_good_streams.end()) {
            remote_audio_good_streams.emplace(streams.stream_id);
          }

          break;
        case MediaType::kVideo:
          if (remote_video_good_streams.find(streams.stream_id) ==
              remote_video_good_streams.end()) {
            remote_video_good_streams.emplace(streams.stream_id);
          }

          break;
      }
    } else {
      switch (media_type) {
        case MediaType::kAudio:
          remote_audio_good_streams.erase(streams.stream_id);
          break;
        case MediaType::kVideo:
          remote_video_good_streams.erase(streams.stream_id);
          break;
      }
    }

    remote_stream_state_changed_semaphore.Notify();
  }

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
                            CDNBYPASS_STREAM_PUBLISH_ERROR error) override {}

  void OnBypassCdnUnpublished(const std::string& stream_id,
                              const std::string& target_cdn_url) override {}

  void OnBypassTranscodingUpdated(const std::string& stream_id) override {}

  void OnSceneStats(const SceneStats& stats) override {}

  void OnLocalStreamStats(const std::string& stream_id,
                          const LocalStreamStats& stats) override {}

  void OnRemoteStreamStats(const std::string& stream_id,
                           const RemoteStreamStats& stats) override {}
};

class RteAudioFrameObserver : public agora::rte::IAgoraRteAudioFrameObserver {
 public:
  bool OnRecordAudioFrame(AudioFrame& audio_frame) override {
    recorded_audio_frame_count++;
    recorded_audio_frame_semaphore.Notify();
    return true;
  }

  bool OnPlaybackAudioFrame(AudioFrame& audio_frame) override {
    playback_audio_frame_count++;
    playback_audio_frame_semaphore.Notify();
    return true;
  }

  bool OnMixedAudioFrame(AudioFrame& audio_frame) override {
    mixed_audio_frame_count++;
    mixed_audio_frame_semaphore.Notify();
    return true;
  }

  bool OnPlaybackAudioFrameBeforeMixing(const std::string& stream_id,
                                        AudioFrame& audio_frame) override {
    playback_audio_frame_before_mixing_count++;
    playback_audio_frame_before_mixing_semaphore.Notify();
    return true;
  }

  agora_test_semaphore recorded_audio_frame_semaphore;
  int recorded_audio_frame_count = 0;
  agora_test_semaphore playback_audio_frame_semaphore;
  int playback_audio_frame_count = 0;
  agora_test_semaphore mixed_audio_frame_semaphore;
  int mixed_audio_frame_count = 0;
  agora_test_semaphore playback_audio_frame_before_mixing_semaphore;
  int playback_audio_frame_before_mixing_count = 0;
};

class RteVideoFrameObserver : public IAgoraRteVideoFrameObserver {
 public:
  void OnFrame(const std::string stream_id,
               const VideoFrame& video_frame) override {
    video_frame_captured_count++;
    stream_video_frame_count[stream_id]++;
    video_frame_captured_semaphore.Notify();
  }

  agora_test_semaphore video_frame_captured_semaphore;
  int video_frame_captured_count = 0;
  std::map<std::string, int> stream_video_frame_count;
};

struct TestSceneObject {
  std::shared_ptr<agora::rte::IAgoraRteScene> scene = nullptr;
  std::shared_ptr<RteSceneEventHandler> rte_scene_event_handler = nullptr;
};

class TestSceneBasic : public testing::Test {
 protected:
  void SetUp() override { CreateDefaultScene(); }

  void TearDown() override {}

 public:
  TestSceneBasic() {
    AgoraRteLogger::EnableLogging(true);
    AgoraRteLogger::SetLevel(LogLevel::Verbose);
    AgoraRteLogger::SetListener(
      [](const std::string& message) { std::cout << message; });

    agora::rte::SdkProfile profile;

    profile.appid = RteTestGloablSettings::GetAppId();

    AgoraRteSDK::Init(profile);

    default_user_id_ =
      GenerateRandomString("rte_test_user");
    default_scene_name_ =
      GenerateRandomString("default_rte_scene_name");
    dummy_stream_ = "dummy_1234";

    token_ = "";

    camera_ob_ = [](CameraState state, CameraSource src) {
      RTE_LOG_VERBOSE << "Camera Src : " << static_cast<int>(src)
                      << "  state: " << static_cast<int>(state);
    };
  }

  ~TestSceneBasic() { AgoraRteSDK::Deinit(); }

  std::string GenerateRandomString(const std::string& prefix,
                                   int random_number_appended = 10) {
    static constexpr char characters[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

    static std::uniform_int_distribution<int> dist(0, sizeof(characters) - 2);

    std::string buf(random_number_appended, 0);

    for (int i = 0; i < random_number_appended; ++i) {
      buf[i] = characters[dist(e_)];
    }

    return prefix + buf;
  }

  void CreateDefaultScene() {
    if (!test_scene_obj_.scene) {
      test_scene_obj_ = CreateScene(default_scene_name_);

      auto media_control = AgoraRteSDK::GetRteMediaFactory();
      EXPECT_TRUE(media_control);

      media_control_ = media_control;
    }
  }

  TestSceneObject CreateScene(const std::string& scene_name) {
    TestSceneObject result;

    RTE_LOG_VERBOSE << "Create Scene: " << scene_name;

    SceneConfig config;
    auto rte_scene = AgoraRteSDK::CreateRteScene(scene_name, config);
    EXPECT_TRUE(rte_scene);

    result.scene = rte_scene;

    result.rte_scene_event_handler = std::make_shared<RteSceneEventHandler>();
    EXPECT_TRUE(result.rte_scene_event_handler);

    return result;
  }

  TestSceneObject CreateScene(const std::string& scene_name,
                              SceneConfig config) {
    TestSceneObject result;

    RTE_LOG_VERBOSE << "Create Scene with config: " << scene_name;
    auto rte_scene = AgoraRteSDK::CreateRteScene(scene_name, config);
    EXPECT_TRUE(rte_scene);

    result.scene = rte_scene;

    result.rte_scene_event_handler = std::make_shared<RteSceneEventHandler>();
    EXPECT_TRUE(result.rte_scene_event_handler);

    return result;
  }

  bool JoinScene(TestSceneObject& scene, const std::string& user_id,
                 const std::string& token, bool expect_result, bool wait_event,
                 bool is_user_visible_to_remote = true) {
    RTE_LOG_VERBOSE << "Join Scene: " << scene.scene->GetSceneInfo().scene_id;

    scene.scene->RegisterEventHandler(scene.rte_scene_event_handler);
    JoinOptions option;
    option.is_user_visible_to_remote = is_user_visible_to_remote;

    EXPECT_TRUE((scene.scene->Join(user_id, token, option) == 0) ==
                expect_result);

    if (wait_event && scene.rte_scene_event_handler) {
      int ret = scene.rte_scene_event_handler->connect_success_semaphore.Wait(
        wait_timeout_in_seconds);
      EXPECT_EQ(ret, 1);
      return 1 == ret;
    }
    return true;
  }

  bool LeaveScene(TestSceneObject& scene, bool expect_result, bool wait_event) {
    RTE_LOG_VERBOSE << "Leave Scene: " << scene.scene->GetSceneInfo().scene_id;

    scene.scene->Leave();

    if (wait_event && scene.rte_scene_event_handler) {
      int ret = scene.rte_scene_event_handler->connect_gone_semaphore.Wait(
        wait_timeout_in_seconds);
      EXPECT_EQ(ret, 1);
      return 1 == ret;
    }
    return true;
  }

  template <typename track_type>
  bool PublishThreadVideoTrack(
    TestSceneObject& test_scene,
    std::shared_ptr<TestTrackWithThread<track_type>> thread_track,
    const std::string& stream_id, bool expect_result, bool wait_event,
    bool start_first = true) {
    if (start_first) {
      EXPECT_EQ(thread_track->Start(), agora::ERR_OK);
    }

    test_scene.scene->CreateOrUpdateRTCStream(stream_id, {""});
    EXPECT_TRUE((test_scene.scene->PublishLocalVideoTrack(
      stream_id, thread_track->track) == agora::ERR_OK) ==
                expect_result);

    if (!start_first) {
      EXPECT_EQ(thread_track->Start(), agora::ERR_OK);
    }

    if (!wait_event) {
      return true;
    }

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_video_good_streams.find(
        stream_id) !=
          test_scene.rte_scene_event_handler->local_video_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool PublishThreadAudioTrack(
    TestSceneObject& test_scene,
    std::shared_ptr<TestTrackWithThread<IAgoraRteCustomAudioTrack>>
    thread_track,
    const std::string& stream_id, bool expect_result, bool wait_event,
    bool start_first = true) {
    if (start_first) {
      EXPECT_EQ(thread_track->Start(), agora::ERR_OK);
    }

    auto track = thread_track->GetTrack();
    test_scene.scene->CreateOrUpdateRTCStream(stream_id, {""});
    EXPECT_TRUE((test_scene.scene->PublishLocalAudioTrack(stream_id, track) ==
                 agora::ERR_OK) == expect_result);

    if (!start_first) {
      EXPECT_EQ(thread_track->Start(), agora::ERR_OK);
    }

    if (!wait_event) {
      return true;
    }

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_audio_good_streams.find(
        stream_id) !=
          test_scene.rte_scene_event_handler->local_audio_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitUnitlVideoStreaming(TestSceneObject& test_scene,
                               const std::string& stream_id) {
    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_video_good_streams.find(
        stream_id) !=
          test_scene.rte_scene_event_handler->local_video_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitUnitlAudioStreaming(TestSceneObject& test_scene,
                               const std::string& stream_id) {
    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_audio_good_streams.find(
        stream_id) !=
          test_scene.rte_scene_event_handler->local_audio_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitUnitlVideoIdle(TestSceneObject& test_scene,
                          const std::string& stream_id) {
    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_video_good_streams.find(
        stream_id) ==
          test_scene.rte_scene_event_handler->local_video_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitUnitlAudioIdle(TestSceneObject& test_scene,
                          const std::string& stream_id) {
    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_audio_good_streams.find(
        stream_id) ==
          test_scene.rte_scene_event_handler->local_audio_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool PublishCameraTrack(TestSceneObject& test_scene,
                          std::shared_ptr<IAgoraRteCameraVideoTrack> track,
                          const std::string& stream_id, bool expect_result,
                          bool wait_event, bool start_first = true) {
    if (start_first) {
      EXPECT_EQ(track->StartCapture(camera_ob_), agora::ERR_OK);
    }

    test_scene.scene->CreateOrUpdateRTCStream(stream_id, {""});
    EXPECT_TRUE((test_scene.scene->PublishLocalVideoTrack(stream_id, track) ==
                 agora::ERR_OK) == expect_result);

    if (!start_first) {
      EXPECT_EQ(track->StartCapture(camera_ob_), agora::ERR_OK);
    }

    if (wait_event) {
      bool ret = WaitUnitlVideoStreaming(test_scene, stream_id);
      EXPECT_TRUE(ret);
      return ret;
    }
    return true;
  }

  template <typename track_type>
  bool UnpublishThreadVideoTrack(
    TestSceneObject& test_scene,
    std::shared_ptr<TestTrackWithThread<track_type>> thread_track,
    bool expect_result, bool wait_event, bool stop_first = true) {
    if (stop_first) {
      EXPECT_EQ(thread_track->Stop(), agora::ERR_OK);
    }

    std::string stream_id = thread_track->track->GetAttachedStreamId();

    EXPECT_TRUE((test_scene.scene->UnpublishLocalVideoTrack(
      thread_track->track) == agora::ERR_OK) == expect_result);

    if (!stop_first) {
      EXPECT_EQ(thread_track->Stop(), agora::ERR_OK);
    }

    if (wait_event) {
      auto ret = WaitUnitlVideoIdle(test_scene, stream_id);
      EXPECT_TRUE(ret);
      return ret;
    }
    return true;
  }

  template <typename track_type>
  bool UnpublishThreadAudioTrack(
    TestSceneObject& test_scene,
    std::shared_ptr<TestTrackWithThread<track_type>> thread_track,
    bool expect_result, bool wait_event, bool stop_first = true) {
    if (stop_first) {
      EXPECT_EQ(thread_track->Stop(), agora::ERR_OK);
    }

    std::string stream_id = thread_track->track->GetAttachedStreamId();

    EXPECT_TRUE((test_scene.scene->UnpublishLocalAudioTrack(
      thread_track->track) == agora::ERR_OK) == expect_result);

    if (!stop_first) {
      EXPECT_EQ(thread_track->Stop(), agora::ERR_OK);
    }

    if (!wait_event) {
      return true;
    }

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_audio_good_streams.find(
        stream_id) ==
          test_scene.rte_scene_event_handler->local_audio_good_streams.end())
        break;

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);

    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool UnpublishCamearTrack(TestSceneObject& test_scene,
                            std::shared_ptr<IAgoraRteCameraVideoTrack> track,
                            bool expect_result, bool wait_event,
                            bool stop_first = true) {
    if (stop_first) {
      track->StopCapture();
    }

    std::string stream_id = track->GetAttachedStreamId();

    EXPECT_TRUE((test_scene.scene->UnpublishLocalVideoTrack(track) ==
                 agora::ERR_OK) == expect_result);

    if (!stop_first) {
      track->StopCapture();
    }

    if (!wait_event) {
      return true;
    }

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->local_video_good_streams.find(
        stream_id) ==
          test_scene.rte_scene_event_handler->local_video_good_streams.end()) {
        break;
      }

      wait_result = test_scene.rte_scene_event_handler
        ->local_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);
      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitForRemoteStreamJoined(TestSceneObject& test_scene,
                                 int expected_number) {
    RTE_LOG_VERBOSE << "Wait remote stream to join in scene:  "
                    << test_scene.scene->GetSceneInfo().scene_id;

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->remote_streams.size() ==
          expected_number)
        break;
      wait_result =
        test_scene.rte_scene_event_handler->remote_stream_added_semaphore
          .Wait(wait_timeout_in_seconds);

      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitForRemoteUserJoined(TestSceneObject& test_scene,
                               int expected_number) {
    RTE_LOG_VERBOSE << "Wait remote user to join in scene:  "
                    << test_scene.scene->GetSceneInfo().scene_id;

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->remote_users.size() ==
          expected_number)
        break;
      wait_result =
        test_scene.rte_scene_event_handler->remote_user_joined_semaphore.Wait(
          wait_timeout_in_seconds);

      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitForRemoteStreamLeft(TestSceneObject& test_scene,
                               int expected_number) {
    RTE_LOG_VERBOSE << "Wait remote stream to leave in scene:  "
                    << test_scene.scene->GetSceneInfo().scene_id;

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->remote_streams.size() ==
          expected_number)
        break;
      wait_result =
        test_scene.rte_scene_event_handler->remote_stream_removed_semaphore
          .Wait(wait_timeout_in_seconds);

      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitForRemoteUserLeft(TestSceneObject& test_scene, int expected_number) {
    RTE_LOG_VERBOSE << "Wait remote user to leave in scene:  "
                    << test_scene.scene->GetSceneInfo().scene_id;

    int wait_result = 1;
    do {
      if (test_scene.rte_scene_event_handler->remote_users.size() ==
          expected_number)
        break;
      wait_result =
        test_scene.rte_scene_event_handler->remote_user_left_semaphore.Wait(
          wait_timeout_in_seconds);

      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitForSubscribeSucceeded(TestSceneObject& test_scene,
                                 int expected_number, MediaType type,
                                 std::string stream_name_to_find = "") {
    RTE_LOG_VERBOSE << "Subscribe remote stream:" << stream_name_to_find
                    << " type: " << (int)type << " in scene:  "
                    << test_scene.scene->GetSceneInfo().scene_id;

    int wait_result = 1;
    do {
      if (type == MediaType::kAudio) {
        if (static_cast<int>(test_scene.rte_scene_event_handler
          ->remote_audio_good_streams.size()) >=
            expected_number) {
          if (!stream_name_to_find.empty()) {
            if (test_scene.rte_scene_event_handler->remote_audio_good_streams
                  .find(stream_name_to_find) !=
                test_scene.rte_scene_event_handler->remote_audio_good_streams
                  .end()) {
              break;
            }
          } else {
            break;
          }
        }
      } else if (type == MediaType::kVideo) {
        if (static_cast<int>(test_scene.rte_scene_event_handler
          ->remote_video_good_streams.size()) >=
            expected_number) {
          if (!stream_name_to_find.empty()) {
            if (test_scene.rte_scene_event_handler->remote_video_good_streams
                  .find(stream_name_to_find) !=
                test_scene.rte_scene_event_handler->remote_video_good_streams
                  .end()) {
              break;
            }
          } else {
            break;
          }
        }
      }

      wait_result = test_scene.rte_scene_event_handler
        ->remote_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);

      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

  bool WaitForUnsubscribeSucceeded(TestSceneObject& test_scene,
                                   int expected_number, MediaType type,
                                   std::string stream_name_to_find = "") {
    RTE_LOG_VERBOSE << "Unsubscribe remote stream:" << stream_name_to_find
                    << "type: " << (int)type << " in scene:  "
                    << test_scene.scene->GetSceneInfo().scene_id;

    int wait_result = 1;
    do {
      if (type == MediaType::kAudio) {
        if (static_cast<int>(test_scene.rte_scene_event_handler
          ->remote_audio_good_streams.size()) <=
            expected_number) {
          if (!stream_name_to_find.empty()) {
            if (test_scene.rte_scene_event_handler->remote_audio_good_streams
                  .find(stream_name_to_find) ==
                test_scene.rte_scene_event_handler->remote_audio_good_streams
                  .end()) {
              break;
            }
          } else {
            break;
          }
        }
      } else if (type == MediaType::kVideo) {
        if (static_cast<int>(test_scene.rte_scene_event_handler
          ->remote_video_good_streams.size()) <=
            expected_number) {
          if (!stream_name_to_find.empty()) {
            if (test_scene.rte_scene_event_handler->remote_video_good_streams
                  .find(stream_name_to_find) ==
                test_scene.rte_scene_event_handler->remote_video_good_streams
                  .end()) {
              break;
            }
          } else {
            break;
          }
        }
      }

      wait_result = test_scene.rte_scene_event_handler
        ->remote_stream_state_changed_semaphore.Wait(
          wait_timeout_in_seconds);

      EXPECT_EQ(wait_result, 1);
    } while (wait_result == 1);
    return 1 == wait_result;
  }

 public:
  std::shared_ptr<agora::rte::IAgoraRteMediaFactory> media_control_ = nullptr;
  TestSceneObject test_scene_obj_;
  std::string default_user_id_;
  std::string default_scene_name_;
  int wait_timeout_in_seconds = 30;
  bool expect_ok = true;
  bool wait_event = true;
  std::string dummy_stream_;
  std::string token_;
  CameraCallbackFun camera_ob_;
  std::mt19937 e_{std::random_device{}()};
};

TEST_F(TestSceneBasic, CreateScene) {
  CreateDefaultScene();

  EXPECT_TRUE(test_scene_obj_.scene != nullptr);

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));
  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));
  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, !expect_ok,
                        !wait_event));
  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, !expect_ok,
                        !wait_event));
  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, !wait_event));
  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, !wait_event));
  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, !wait_event));
}

TEST_F(TestSceneBasic, PubAudioVideoToDifferentStream) {
  // We could publish several audio/video tracks to different streams
  //
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  // Push camera to stream camera_stream_id_1
  //
  auto camera_track_1 = media_control_->CreateCameraVideoTrack();

  EXPECT_TRUE(PublishCameraTrack(test_scene_obj_, camera_track_1,
                                 "camera_stream_id_1", expect_ok, wait_event));

  VideoEncoderConfiguration video_config;

  video_config.codecType = agora::rtc::VIDEO_CODEC_H264;
  video_config.dimensions = {1280, 720};
  video_config.bitrate = 2000;
  video_config.frameRate = 6;
  EXPECT_EQ(test_scene_obj_.scene->SetVideoEncoderConfiguration(
    "camera_stream_id_1", video_config),
            agora::ERR_OK);

  // std::this_thread::sleep_for(std::chrono::seconds(5000));

  // Push customer video to stream customer_stream_id_1
  //
  auto custom_video_track_2 = CreateTestCustomVideoTrack();

  EXPECT_TRUE(PublishThreadVideoTrack(test_scene_obj_, custom_video_track_2,
                                      "custom_video_stream_id_1", expect_ok,
                                      wait_event));

  // Push customer audio to stream customer_audio_stream_id_1
  //
  auto custom_audio_track_3 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, custom_audio_track_3,
                                      "custom_audio_stream_id_1", expect_ok,
                                      wait_event));

  // Push customer audio to stream customer_audio_stream_id_2
  //
  auto custom_audio_track_4 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, custom_audio_track_4,
                                      "custom_audio_stream_id_2", expect_ok,
                                      wait_event));

  std::this_thread::sleep_for(std::chrono::seconds(5));

  // We don't have remote streams yet since all published stream are local
  // streams
  //
  EXPECT_EQ(test_scene_obj_.scene->GetRemoteStreams().size(), 0);
  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 4);

  EXPECT_TRUE(UnpublishCamearTrack(test_scene_obj_, camera_track_1, expect_ok,
                                   wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 4);

  test_scene_obj_.scene->DestroyStream("camera_stream_id_1");

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 3);

  EXPECT_TRUE(UnpublishThreadVideoTrack(test_scene_obj_, custom_video_track_2,
                                        expect_ok, wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 3);

  test_scene_obj_.scene->DestroyStream("custom_video_stream_id_1");

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 2);

  EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, custom_audio_track_3,
                                        expect_ok, wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 2);

  test_scene_obj_.scene->DestroyStream("custom_audio_stream_id_1");

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, custom_audio_track_4,
                                        expect_ok, wait_event));

  test_scene_obj_.scene->DestroyStream("custom_audio_stream_id_2");

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 0);

  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, PubUnpubSingleVideoToSameStream) {
  // We could publish/unpublish tracks to same stream several times
  //
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  // Push camera to stream camera_stream_id_1
  //
  auto camera_track_1 = media_control_->CreateCameraVideoTrack();

  for (int i = 0; i < 10; i++) {
    EXPECT_TRUE(PublishCameraTrack(test_scene_obj_, camera_track_1,
                                   "camera_stream_id_1", expect_ok,
                                   !wait_event));
    EXPECT_TRUE(UnpublishCamearTrack(test_scene_obj_, camera_track_1, expect_ok,
                                     !wait_event));
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, PubAudioVideoToDifferentStreamInMultipleThread) {
  // We could publish several audio/video tracks to different streams in
  // multiple threads
  //
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  constexpr int thread_count = 2;
  constexpr int stream_per_thread = 4;
  constexpr int expected_published = thread_count * stream_per_thread;
  std::vector<std::shared_ptr<std::thread>> threads;
  std::vector<std::vector<
    std::shared_ptr<TestTrackWithThread<IAgoraRteCustomVideoTrack>>>>
    tracks(thread_count);

  for (int i = 0; i < thread_count; i++) {
    int cursor = i;
    threads.emplace_back(std::make_shared<std::thread>([&, cursor] {
      tracks[cursor].resize(stream_per_thread);
      for (int j = 0; j < stream_per_thread; j++) {
        std::string stream_id =
          "test_stream" + std::to_string(cursor * 100 + j);

        auto custom_video_track_2 = CreateTestCustomVideoTrack();

        custom_video_track_2->Start();

        test_scene_obj_.scene->CreateOrUpdateRTCStream(stream_id, {""});

        EXPECT_EQ(test_scene_obj_.scene->PublishLocalVideoTrack(
          stream_id, custom_video_track_2->GetTrack()),
                  agora::ERR_OK);
        tracks[cursor][j] = std::move(custom_video_track_2);
      }
    }));
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  for (int i = 0; i < (static_cast<int>(threads.size())); i++) {
    threads[i]->join();
  }

  auto& semaphore = test_scene_obj_.rte_scene_event_handler
    ->local_stream_state_changed_semaphore;

  int count = 0;
  do {
    EXPECT_EQ(semaphore.Wait(wait_timeout_in_seconds), 1);
    count++;
    RTE_LOG_VERBOSE << "Get notified " << count;
  } while (count != expected_published);

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(),
            expected_published);

  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, PubMultipleAudioAndTwoVideoToSameStream) {
  // One stream could only have multiple audios and only one video track at most
  //
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  // TODO(yejun): check why camera hang if we don't sleep here
  //
  std::this_thread::sleep_for(std::chrono::seconds(3));

  auto camera_track = media_control_->CreateCameraVideoTrack();
  EXPECT_TRUE(PublishCameraTrack(test_scene_obj_, camera_track, "my_stream_id",
                                 expect_ok, wait_event));

  auto custom_video_track_2 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(test_scene_obj_, custom_video_track_2,
                                      "my_stream_id", !expect_ok, !wait_event));

  custom_video_track_2 = nullptr;

  auto audio_track = CreateTestCustomAudioTrack();
  EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, audio_track,
                                      "my_stream_id", expect_ok, wait_event));

  auto custom_audio_track_2 = CreateTestCustomAudioTrack();
  // We don't need to wait again, as long as the publish call is succeeded, we
  // assume audio is published since previous audio publishing tells the network
  // is good.
  //
  //
  EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, custom_audio_track_2,
                                      "my_stream_id", expect_ok, !wait_event));

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // We don't have remote streams yet since all published stream are local
  // streams
  //
  EXPECT_EQ(test_scene_obj_.scene->GetRemoteStreams().size(), 0);

  EXPECT_TRUE(UnpublishCamearTrack(test_scene_obj_, camera_track, expect_ok,
                                   wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, audio_track, expect_ok,
                                        !wait_event));

  // The stream should be still there as we still have one audio track left
  //
  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, custom_audio_track_2,
                                        expect_ok, wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  test_scene_obj_.scene->DestroyStream("my_stream_id");

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 0);

  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, PubAudioVideoToSameStreamWithoutWaitingResult) {
  // One stream could only have multiple audios and only one video track at most
  //
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  auto camera_track = media_control_->CreateCameraVideoTrack();
  EXPECT_TRUE(PublishCameraTrack(test_scene_obj_, camera_track, "my_stream_id",
                                 expect_ok, !wait_event));

  auto custom_video_track_2 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(test_scene_obj_, custom_video_track_2,
                                      "my_stream_id", !expect_ok, !wait_event));

  custom_video_track_2 = nullptr;

  auto audio_track = CreateTestCustomAudioTrack();
  EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, audio_track,
                                      "my_stream_id", expect_ok, !wait_event));

  auto custom_audio_track_2 = CreateTestCustomAudioTrack();
  // We don't need to wait again, as long as the publish call is succeeded, we
  // assume audio is published since previous audio publishing tells the network
  // is good.
  //
  //
  EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, custom_audio_track_2,
                                      "my_stream_id", expect_ok, !wait_event));

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // We don't have remote streams yet since all published stream are local
  // streams
  //
  EXPECT_EQ(test_scene_obj_.scene->GetRemoteStreams().size(), 0);

  EXPECT_TRUE(UnpublishCamearTrack(test_scene_obj_, camera_track, expect_ok,
                                   !wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, audio_track, expect_ok,
                                        !wait_event));

  // The stream should be still there as we still have one audio track left
  //
  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, custom_audio_track_2,
                                        expect_ok, !wait_event));

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 1);

  test_scene_obj_.scene->DestroyStream("my_stream_id");

  EXPECT_EQ(test_scene_obj_.scene->GetLocalStreams().size(), 0);

  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, !wait_event));
}

TEST_F(TestSceneBasic, PubUnpubAudioVideoInDifferentOrders) {
  // One track could only be published to one stream, but could republished
  // again after it's unpublished
  //
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  auto camera_track = media_control_->CreateCameraVideoTrack();
  auto audio_track = CreateTestCustomAudioTrack();

  for (int i = 0; i < 5; i++) {
    RTE_LOG_INFO << "Loop : " << i;

    if (i % 2 == 0) {
      EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, audio_track,
                                          "camera_stream_id", expect_ok,
                                          wait_event, i < 3));
      EXPECT_TRUE(PublishCameraTrack(test_scene_obj_, camera_track,
                                     "camera_stream_id", expect_ok, wait_event,
                                     i < 3));
    } else {
      EXPECT_TRUE(PublishCameraTrack(test_scene_obj_, camera_track,
                                     "camera_stream_id", expect_ok, wait_event,
                                     i < 3));
      EXPECT_TRUE(PublishThreadAudioTrack(test_scene_obj_, audio_track,
                                          "camera_stream_id", expect_ok,
                                          wait_event, i < 3));
    }

    if (i % 3 == 0) {
      EXPECT_TRUE(UnpublishCamearTrack(test_scene_obj_, camera_track, expect_ok,
                                       wait_event, i < 3));
      EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, audio_track,
                                            expect_ok, wait_event, i < 3));
    } else {
      EXPECT_TRUE(UnpublishThreadAudioTrack(test_scene_obj_, audio_track,
                                            expect_ok, wait_event, i < 3));
      EXPECT_TRUE(UnpublishCamearTrack(test_scene_obj_, camera_track, expect_ok,
                                       wait_event, i < 3));
    }
  }

  EXPECT_TRUE(LeaveScene(test_scene_obj_, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, ThreeUsersPubAndOneUserSub) {
  // We could create two scenes, and publish/Unpublish tracks, then
  // subscribe/Unsubscribe each other
  //

  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");
  auto user_2 = GenerateRandomString("test_user");
  auto user_3 = GenerateRandomString("test_user");
  auto user_4 = GenerateRandomString("test_user");

  auto scene_object_1 = CreateScene(scene_name);
  auto scene_object_2 = CreateScene(scene_name);
  auto scene_object_3 = CreateScene(scene_name);
  auto scene_object_4 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Joining Scenes";
  EXPECT_TRUE(JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_2, user_2, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_3, user_3, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_4, user_4, token_, expect_ok, wait_event));

  int expected_num_of_remote_usr = 3;
  EXPECT_TRUE(
    WaitForRemoteUserJoined(scene_object_4, expected_num_of_remote_usr));

  EXPECT_EQ(scene_object_4.scene->GetRemoteUsers().size(), 3);

  auto audio_track_1 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_1, audio_track_1,
                                      "my_test_stream_1", expect_ok,
                                      !wait_event));

  auto custom_video_track_1 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_1, custom_video_track_1,
                                      "my_test_stream_1", expect_ok,
                                      !wait_event));

  auto audio_track_2 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, audio_track_2,
                                      "my_test_stream_2_1", expect_ok,
                                      !wait_event));

  auto custom_video_track_2 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_2, custom_video_track_2,
                                      "my_test_stream_2_2", expect_ok,
                                      !wait_event));

  auto audio_track_3 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_3, audio_track_3,
                                      "my_test_stream_3", expect_ok,
                                      !wait_event));

  int expected_num_of_remote_stream = 4;
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_4, expected_num_of_remote_stream));

  auto remote_streams = scene_object_4.scene->GetRemoteStreams();
  EXPECT_EQ(remote_streams.size(), 4);

  int found_stream = 0;
  for (auto& stream_name : remote_streams) {
    if (stream_name.stream_id == "my_test_stream_1") {
      found_stream++;
      EXPECT_EQ(stream_name.user_id,
                scene_object_1.scene->GetLocalUserInfo().user_id);
    }

    if (stream_name.stream_id.find("my_test_stream_2") != std::string::npos) {
      found_stream++;
      EXPECT_EQ(stream_name.user_id,
                scene_object_2.scene->GetLocalUserInfo().user_id);
    }

    if (stream_name.stream_id.find("my_test_stream_3") != std::string::npos) {
      found_stream++;
      EXPECT_EQ(stream_name.user_id,
                scene_object_3.scene->GetLocalUserInfo().user_id);
    }
  }

  EXPECT_EQ(found_stream, 4);

  EXPECT_EQ(scene_object_4.scene->GetRemoteStreamsByUserId(user_1).size(), 1);
  EXPECT_EQ(scene_object_4.scene->GetRemoteStreamsByUserId(user_2).size(), 2);
  EXPECT_EQ(scene_object_4.scene->GetRemoteStreamsByUserId(user_3).size(), 1);

  EXPECT_TRUE(LeaveScene(scene_object_1, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_2, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_3, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_4, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, TwoUserSubsribeEachOther) {
  // We could create two scenes, and publish/Unpublish tracks, then
  // subscribe/Unsubscribe each other
  //

  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");
  auto user_2 = GenerateRandomString("test_user");

  auto scene_object_1 = CreateScene(scene_name);
  auto scene_object_2 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Joining Scenes";
  EXPECT_TRUE(JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_2, user_2, token_, expect_ok, wait_event));

  int expected_num_of_remote_usr = 1;
  EXPECT_TRUE(
    WaitForRemoteUserJoined(scene_object_1, expected_num_of_remote_usr));
  EXPECT_TRUE(
    WaitForRemoteUserJoined(scene_object_2, expected_num_of_remote_usr));

  EXPECT_EQ(scene_object_1.scene->GetRemoteUsers().size(),
            expected_num_of_remote_usr);
  EXPECT_EQ(scene_object_2.scene->GetRemoteUsers().size(),
            expected_num_of_remote_usr);

  // scene_object_1 has my_test_stream_1 and my_test_stream_3
  // scene_object_2 has my_test_stream_2 and my_test_stream_4

  // publish all of them
  auto camera_track_1 = media_control_->CreateCameraVideoTrack();
  EXPECT_TRUE(PublishCameraTrack(scene_object_1, camera_track_1,
                                 "my_test_stream_1", expect_ok, wait_event));

  auto audio_track_1 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_1, audio_track_1,
                                      "my_test_stream_1", expect_ok,
                                      wait_event));

  auto custom_video_track_2 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_2, custom_video_track_2,
                                      "my_test_stream_2", expect_ok,
                                      wait_event));

  auto custom_audio_track_2 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, custom_audio_track_2,
                                      "my_test_stream_2", expect_ok,
                                      wait_event));

  auto custom_video_track_3 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_1, custom_video_track_3,
                                      "my_test_stream_3", expect_ok,
                                      wait_event));

  auto custom_audio_track_3 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_1, custom_audio_track_3,
                                      "my_test_stream_3", expect_ok,
                                      wait_event));

  auto custom_video_track_4 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_2, custom_video_track_4,
                                      "my_test_stream_4", expect_ok,
                                      wait_event));

  auto custom_audio_track_4 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, custom_audio_track_4,
                                      "my_test_stream_4", expect_ok,
                                      wait_event));

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Both scene_object_1 and scene_object_1 should have two remote streams
  // scene_object_1  remote stream = my_test_stream_2 and my_test_stream_4
  // scene_object_2  remote stream = my_test_stream_1 and my_test_stream_3
  //
  int expected_num_of_remote_stream = 2;
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_1, expected_num_of_remote_stream));
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_2, expected_num_of_remote_stream));

  auto remote_stream_1 = scene_object_1.scene->GetRemoteStreams();
  EXPECT_EQ(remote_stream_1.size(), 2);
  EXPECT_EQ(remote_stream_1[0].stream_id, "my_test_stream_2");
  EXPECT_EQ(remote_stream_1[1].stream_id, "my_test_stream_4");

  auto remote_stream_2 = scene_object_2.scene->GetRemoteStreams();
  EXPECT_EQ(remote_stream_2.size(), 2);
  EXPECT_EQ(remote_stream_2[0].stream_id, "my_test_stream_1");
  EXPECT_EQ(remote_stream_2[1].stream_id, "my_test_stream_3");

  // scene_object_1  subscribe audio and video from my_test_stream_2
  //
  scene_object_1.scene->SubscribeRemoteAudio("my_test_stream_2");

  VideoSubscribeOptions option;
  scene_object_1.scene->SubscribeRemoteVideo("my_test_stream_2", option);

  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_1, 1, MediaType::kAudio,
                                        "my_test_stream_2"));
  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_1, 1, MediaType::kVideo,
                                        "my_test_stream_2"));

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // scene_object_1  unsubscribe audio and video from my_test_stream_2
  //
  scene_object_1.scene->UnsubscribeRemoteAudio("my_test_stream_2");
  scene_object_1.scene->UnsubscribeRemoteVideo("my_test_stream_2");

  EXPECT_TRUE(WaitForUnsubscribeSucceeded(scene_object_1, 0, MediaType::kAudio,
                                          "my_test_stream_2"));
  EXPECT_TRUE(WaitForUnsubscribeSucceeded(scene_object_1, 0, MediaType::kVideo,
                                          "my_test_stream_2"));

  // scene_object_1 unpublish video from stream my_test_stream_1
  //
  EXPECT_TRUE(UnpublishCamearTrack(scene_object_1, camera_track_1, expect_ok,
                                   wait_event));

  // We still have audio in stream: my_test_stream_1 , no stream are destroyed
  EXPECT_EQ(scene_object_2.scene->GetRemoteStreams().size(), 2);

  // scene_object_1 unpublish audio from stream my_test_stream_1 , no stream is
  // in my_test_stream_1, then we will close it internally
  //
  EXPECT_TRUE(UnpublishThreadAudioTrack(scene_object_1, audio_track_1,
                                        expect_ok, wait_event));

  expected_num_of_remote_stream = 1;

  RTE_LOG_VERBOSE << "Waiting for my_test_stream_1 leave";

  scene_object_1.scene->DestroyStream("my_test_stream_1");

  EXPECT_TRUE(
    WaitForRemoteStreamLeft(scene_object_2, expected_num_of_remote_stream));

  // Now my_test_stream_1 should be closed
  //
  auto remote_streams2 = scene_object_2.scene->GetRemoteStreams();
  EXPECT_EQ(remote_streams2.size(), 1);
  EXPECT_EQ(remote_streams2[0].stream_id, "my_test_stream_3");

  // scene_object_2 unpublish video from stream my_test_stream_2
  //

  UnpublishThreadVideoTrack(scene_object_2, custom_video_track_2, expect_ok,
                            wait_event);

  // Nothing changed since my_test_stream_2 still has audio
  //
  EXPECT_EQ(scene_object_1.scene->GetRemoteStreams().size(), 2);

  // scene_object_2 unpublish audio from stream my_test_stream_2
  //
  EXPECT_TRUE(UnpublishThreadAudioTrack(scene_object_2, custom_audio_track_2,
                                        expect_ok, wait_event));

  scene_object_2.scene->DestroyStream("my_test_stream_2");

  RTE_LOG_VERBOSE << "Waiting for my_test_stream_2 leave";

  EXPECT_TRUE(
    WaitForRemoteStreamLeft(scene_object_1, expected_num_of_remote_stream));

  auto remote_streams_1 = scene_object_1.scene->GetRemoteStreams();

  // my_test_stream_2 will be closed internally , so scenen1 should only has
  // my_test_stream_4
  //
  EXPECT_EQ(remote_streams_1.size(), 1);
  EXPECT_EQ(remote_streams_1[0].stream_id, "my_test_stream_4");

  EXPECT_TRUE(LeaveScene(scene_object_1, expect_ok, wait_event));

  RTE_LOG_VERBOSE << "Waiting for " << user_1 << "leave";

  expected_num_of_remote_usr = 0;
  EXPECT_TRUE(
    WaitForRemoteUserLeft(scene_object_2, expected_num_of_remote_usr));

  expected_num_of_remote_stream = 0;
  EXPECT_TRUE(
    WaitForRemoteStreamLeft(scene_object_2, expected_num_of_remote_stream));

  EXPECT_EQ(scene_object_2.scene->GetRemoteUsers().size(), 0);
  EXPECT_EQ(scene_object_2.scene->GetRemoteStreams().size(), 0);

  EXPECT_TRUE(LeaveScene(scene_object_2, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, TwoUserSubsribeEachOtherWithInvisibleMode) {
  // We could create two scenes, and publish/Unpublish tracks, then
  // subscribe/Unsubscribe each other
  //

  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");
  auto user_2 = GenerateRandomString("test_user");

  auto scene_object_1 = CreateScene(scene_name);
  auto scene_object_2 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Joining Scenes";
  bool user_visible_to_remote = false;
  EXPECT_TRUE(JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event,
                        user_visible_to_remote));
  EXPECT_TRUE(JoinScene(scene_object_2, user_2, token_, expect_ok, wait_event,
                        user_visible_to_remote));

  // scene_object_1 has my_test_stream_1 and my_test_stream_3
  // scene_object_2 has my_test_stream_2 and my_test_stream_4

  // publish all of them
  auto camera_track_1 = media_control_->CreateCameraVideoTrack();
  EXPECT_TRUE(PublishCameraTrack(scene_object_1, camera_track_1,
                                 "my_test_stream_1", expect_ok, wait_event));

  auto audio_track_1 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_1, audio_track_1,
                                      "my_test_stream_1", expect_ok,
                                      wait_event));

  auto custom_video_track_2 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_2, custom_video_track_2,
                                      "my_test_stream_2", expect_ok,
                                      wait_event));

  auto custom_audio_track_2 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, custom_audio_track_2,
                                      "my_test_stream_2", expect_ok,
                                      wait_event));

  auto custom_video_track_3 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_1, custom_video_track_3,
                                      "my_test_stream_3", expect_ok,
                                      wait_event));

  auto custom_audio_track_3 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_1, custom_audio_track_3,
                                      "my_test_stream_3", expect_ok,
                                      wait_event));

  auto custom_video_track_4 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_2, custom_video_track_4,
                                      "my_test_stream_4", expect_ok,
                                      wait_event));

  auto custom_audio_track_4 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, custom_audio_track_4,
                                      "my_test_stream_4", expect_ok,
                                      wait_event));

  std::this_thread::sleep_for(std::chrono::seconds(3));

  EXPECT_EQ(scene_object_1.scene->GetRemoteUsers().size(), 0);
  EXPECT_EQ(scene_object_2.scene->GetRemoteUsers().size(), 0);

  // Both scene_object_1 and scene_object_1 should have two remote streams
  // scene_object_1  remote stream = my_test_stream_2 and my_test_stream_4
  // scene_object_2  remote stream = my_test_stream_1 and my_test_stream_3
  //
  int expected_num_of_remote_stream = 2;
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_1, expected_num_of_remote_stream));
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_2, expected_num_of_remote_stream));

  auto remote_stream_1 = scene_object_1.scene->GetRemoteStreams();
  EXPECT_EQ(remote_stream_1.size(), 2);
  EXPECT_EQ(remote_stream_1[0].stream_id, "my_test_stream_2");
  EXPECT_EQ(remote_stream_1[1].stream_id, "my_test_stream_4");

  auto remote_stream_2 = scene_object_2.scene->GetRemoteStreams();
  EXPECT_EQ(remote_stream_2.size(), 2);
  EXPECT_EQ(remote_stream_2[0].stream_id, "my_test_stream_1");
  EXPECT_EQ(remote_stream_2[1].stream_id, "my_test_stream_3");

  // scene_object_1  subscribe audio and video from my_test_stream_2
  //
  scene_object_1.scene->SubscribeRemoteAudio("my_test_stream_2");

  VideoSubscribeOptions option;
  scene_object_1.scene->SubscribeRemoteVideo("my_test_stream_2", option);

  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_1, 1, MediaType::kAudio,
                                        "my_test_stream_2"));
  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_1, 1, MediaType::kVideo,
                                        "my_test_stream_2"));

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // scene_object_1  unsubscribe audio and video from my_test_stream_2
  //
  scene_object_1.scene->UnsubscribeRemoteAudio("my_test_stream_2");
  scene_object_1.scene->UnsubscribeRemoteVideo("my_test_stream_2");

  EXPECT_TRUE(WaitForUnsubscribeSucceeded(scene_object_1, 0, MediaType::kAudio,
                                          "my_test_stream_2"));
  EXPECT_TRUE(WaitForUnsubscribeSucceeded(scene_object_1, 0, MediaType::kVideo,
                                          "my_test_stream_2"));

  // scene_object_1 unpublish video from stream my_test_stream_1
  //
  EXPECT_TRUE(UnpublishCamearTrack(scene_object_1, camera_track_1, expect_ok,
                                   wait_event));

  // We still have audio in stream: my_test_stream_1 , no stream are destroyed
  EXPECT_EQ(scene_object_2.scene->GetRemoteStreams().size(), 2);

  // scene_object_1 unpublish audio from stream my_test_stream_1 , no stream is
  // in my_test_stream_1, then we will close it internally
  //
  EXPECT_TRUE(UnpublishThreadAudioTrack(scene_object_1, audio_track_1,
                                        expect_ok, wait_event));

  expected_num_of_remote_stream = 1;

  RTE_LOG_VERBOSE << "Waiting for my_test_stream_1 leave";

  scene_object_1.scene->DestroyStream("my_test_stream_1");

  EXPECT_TRUE(
    WaitForRemoteStreamLeft(scene_object_2, expected_num_of_remote_stream));

  // Now my_test_stream_1 should be closed
  //
  auto remote_streams2 = scene_object_2.scene->GetRemoteStreams();
  EXPECT_EQ(remote_streams2.size(), 1);
  EXPECT_EQ(remote_streams2[0].stream_id, "my_test_stream_3");

  // scene_object_2 unpublish video from stream my_test_stream_2
  //

  EXPECT_TRUE(UnpublishThreadVideoTrack(scene_object_2, custom_video_track_2,
                                        expect_ok, wait_event));

  // Nothing changed since my_test_stream_2 still has audio
  //
  EXPECT_EQ(scene_object_1.scene->GetRemoteStreams().size(), 2);

  // scene_object_2 unpublish audio from stream my_test_stream_2
  //
  EXPECT_TRUE(UnpublishThreadAudioTrack(scene_object_2, custom_audio_track_2,
                                        expect_ok, wait_event));

  scene_object_2.scene->DestroyStream("my_test_stream_2");

  RTE_LOG_VERBOSE << "Waiting for my_test_stream_2 leave";

  EXPECT_TRUE(
    WaitForRemoteStreamLeft(scene_object_1, expected_num_of_remote_stream));

  auto remote_streams_1 = scene_object_1.scene->GetRemoteStreams();

  // my_test_stream_2 will be closed internally , so scenen1 should only has
  // my_test_stream_4
  //
  EXPECT_EQ(remote_streams_1.size(), 1);
  EXPECT_EQ(remote_streams_1[0].stream_id, "my_test_stream_4");

  EXPECT_TRUE(LeaveScene(scene_object_1, expect_ok, wait_event));

  RTE_LOG_VERBOSE << "Waiting for " << user_1 << "leave";

  expected_num_of_remote_stream = 0;
  EXPECT_TRUE(
    WaitForRemoteStreamLeft(scene_object_2, expected_num_of_remote_stream));

  EXPECT_EQ(scene_object_2.scene->GetRemoteStreams().size(), 0);

  EXPECT_TRUE(LeaveScene(scene_object_2, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, SimpleMediaPlayer) {
  // We could publish media player
  //

  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");
  auto user_2 = GenerateRandomString("test_user");

  auto scene_object_1 = CreateScene(scene_name);
  auto scene_object_2 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Joining Scenes";
  EXPECT_TRUE(
    JoinScene(scene_object_1, user_1, token_, expect_ok, !wait_event));
  EXPECT_TRUE(
    JoinScene(scene_object_2, user_2, token_, expect_ok, !wait_event));

  auto media_player = media_control_->CreateMediaPlayer();

  auto ob = std::make_shared<MediaPlayerObserverSimple>(media_player);

  media_player->RegisterMediaPlayerObserver(ob);

  RtcStreamOptions pub_option;
  scene_object_1.scene->CreateOrUpdateRTCStream("my_media_player_stream",
                                                pub_option);

  int result = scene_object_1.scene->PublishMediaPlayer(
    "my_media_player_stream", media_player);
  EXPECT_EQ(result, agora::ERR_OK);

#if defined(WIN32)
  auto test_file_path1_ =
      GetExeRunningFolder() + "\\test_data\\h264_opus_01.mp4";
  result = media_player->Open(test_file_path1_, 0);
#else
  result = media_player->Open("./test_data/h264_opus_01.mp4", 0);
#endif

  EXPECT_EQ(result, agora::ERR_OK);

  EXPECT_TRUE(ob->WaitFileOpened(wait_timeout_in_seconds));

  result = media_player->Play();

  EXPECT_EQ(result, agora::ERR_OK);

  std::this_thread::sleep_for(std::chrono::milliseconds(1250));

  EXPECT_EQ(agora::ERR_OK, media_player->Mute(true));
  EXPECT_EQ(true, media_player->IsMuted());

  std::this_thread::sleep_for(std::chrono::milliseconds(550));

  EXPECT_EQ(agora::ERR_OK, media_player->Mute(false));
  EXPECT_EQ(false, media_player->IsMuted());

  int expected_num_of_remote_stream = 1;
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_2, expected_num_of_remote_stream));

  scene_object_2.scene->SubscribeRemoteAudio("my_media_player_stream");

  VideoSubscribeOptions option;
  scene_object_2.scene->SubscribeRemoteVideo("my_media_player_stream", option);

  auto video_ob_1 = std::make_shared<RteVideoFrameObserver>();

  scene_object_2.scene->RegisterRemoteVideoFrameObserver(video_ob_1);

  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_2, 1, MediaType::kAudio,
                                        "my_media_player_stream"));
  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_2, 1, MediaType::kVideo,
                                        "my_media_player_stream"));

  std::this_thread::sleep_for(std::chrono::milliseconds(1250));

  video_ob_1->video_frame_captured_semaphore.Wait(wait_timeout_in_seconds);

  EXPECT_GE(video_ob_1->stream_video_frame_count["my_media_player_stream"], 0);

  result = scene_object_1.scene->UnpublishMediaPlayer(media_player);
  EXPECT_EQ(result, agora::ERR_OK);

  EXPECT_TRUE(WaitForUnsubscribeSucceeded(scene_object_2, 0, MediaType::kAudio,
                                          "my_media_player_stream"));
  EXPECT_TRUE(WaitForUnsubscribeSucceeded(scene_object_2, 0, MediaType::kVideo,
                                          "my_media_player_stream"));

  EXPECT_TRUE(LeaveScene(scene_object_2, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, AllVideoTracks) {
  int track_count = 0;
  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");

  auto scene_object_1 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Joining Scenes";
  JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event);

  auto camera_track = media_control_->CreateCameraVideoTrack();

  auto video_ob_1 = std::make_shared<RteVideoFrameObserver>();
  camera_track->RegisterVideoFrameObserver(video_ob_1);

  auto video_ob_2 = std::make_shared<RteVideoFrameObserver>();
  auto custom_track_1 = CreateTestCustomVideoTrack();
  custom_track_1->GetTrack()->RegisterVideoFrameObserver(video_ob_2);

  custom_track_1->Start();
  auto custom_track_2 = CreateTestCustomVideoTrack();
  custom_track_2->Start();
  auto custom_track_3 = CreateTestCustomVideoTrack();
  custom_track_3->Start();

#if RTE_WIN || RTE_LINUX
  auto video_ob_3 = std::make_shared<RteVideoFrameObserver>();
  auto mixed_track = media_control_->CreateMixedVideoTrack();
  mixed_track->RegisterVideoFrameObserver(video_ob_3);
  mixed_track->AddTrack(custom_track_2->GetTrack());
  mixed_track->AddTrack(custom_track_3->GetTrack());
#endif

  auto screen_track = media_control_->CreateScreenVideoTrack();
  auto video_ob_4 = std::make_shared<RteVideoFrameObserver>();
  screen_track->RegisterVideoFrameObserver(video_ob_4);

#if RTE_WIN
  agora::rtc::Rectangle rect;
  EXPECT_EQ(screen_track->StartCaptureScreen(rect, rect), agora::ERR_OK);
#elif RTE_MAC
  agora::rtc::Rectangle rect;
  EXPECT_EQ(screen_track->StartCaptureScreen(nullptr, rect), agora::ERR_OK);
#elif RTE_ANDROID
#elif RTE_IPHONE
#endif
  scene_object_1.scene->CreateOrUpdateRTCStream("camera_stream_id_1", {""});

  int target_bitrate_bps = 500 * 1000;
  VideoEncoderConfiguration encoder_config;
  encoder_config.codecType = agora::rtc::VIDEO_CODEC_H264;
  encoder_config.bitrate = target_bitrate_bps;
  encoder_config.minBitrate = target_bitrate_bps / 2;
  encoder_config.dimensions.width = 1280;
  encoder_config.dimensions.height = 720;
  encoder_config.frameRate = 7;
  encoder_config.degradationPreference = agora::rtc::MAINTAIN_FRAMERATE;

  scene_object_1.scene->SetVideoEncoderConfiguration("camera_stream_id_1",
                                                     encoder_config);

  LayoutConfigs layout;
  layout.canvas.width = 1280;
  layout.canvas.height = 720;
  layout.canvas.fps = 15;

#if defined(WIN32)
  layout.canvas.image_file_path =
      GetExeRunningFolder() + "\\test_data\\test1.png";
#else
  layout.canvas.image_file_path = "./test_data/test1.png";
#endif

  layout.canvas.image_file_path = "";

#if RTE_WIN || RTE_LINUX
  mixed_track->SetLayout(layout);
#endif

  EXPECT_TRUE(PublishCameraTrack(scene_object_1, camera_track,
                                 "camera_stream_id_1", expect_ok, wait_event));
  track_count++;

  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_1, custom_track_1,
                                      "custom_video_stream_id_1", expect_ok,
                                      wait_event));

  track_count++;

  scene_object_1.scene->CreateOrUpdateRTCStream("mix_video_stream_id_1", {""});
  scene_object_1.scene->SetVideoEncoderConfiguration("mix_video_stream_id_1",
                                                     encoder_config);

#if RTE_WIN || RTE_LINUX
  EXPECT_EQ(scene_object_1.scene->PublishLocalVideoTrack(
                "mix_video_stream_id_1", mixed_track),
            agora::ERR_OK);
#endif

  track_count++;

  EXPECT_TRUE(WaitUnitlVideoStreaming(scene_object_1, "mix_video_stream_id_1"));

#if RTE_WIN
  scene_object_1.scene->CreateOrUpdateRTCStream("screen_video_stream_id_1",
                                                {""});
  EXPECT_EQ(scene_object_1.scene->PublishLocalVideoTrack(
                "screen_video_stream_id_1", screen_track),
            agora::ERR_OK);

  track_count++;
  EXPECT_TRUE(
      WaitUnitlVideoStreaming(scene_object_1, "screen_video_stream_id_1"));
#endif

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_EQ(scene_object_1.scene->GetRemoteStreams().size(), 0);

  EXPECT_EQ(scene_object_1.scene->GetLocalStreams().size(), track_count);

  EXPECT_TRUE(UnpublishCamearTrack(scene_object_1, camera_track, expect_ok,
                                   wait_event));

  scene_object_1.scene->DestroyStream("camera_stream_id_1");
  track_count--;

  EXPECT_EQ(scene_object_1.scene->GetLocalStreams().size(), track_count);

  UnpublishThreadVideoTrack(scene_object_1, custom_track_1, expect_ok,
                            wait_event);
  track_count--;
  scene_object_1.scene->DestroyStream("custom_video_stream_id_1");

  EXPECT_EQ(scene_object_1.scene->GetLocalStreams().size(), track_count);

#if RTE_WIN || RTE_LINUX
  EXPECT_EQ(scene_object_1.scene->UnpublishLocalVideoTrack(mixed_track),
            agora::ERR_OK);
#endif
  track_count--;

  EXPECT_TRUE(WaitUnitlVideoIdle(scene_object_1, "mix_video_stream_id_1"));
  scene_object_1.scene->DestroyStream("mix_video_stream_id_1");

  EXPECT_EQ(scene_object_1.scene->GetLocalStreams().size(), track_count);

#if RTE_WIN || RTE_MAC
  EXPECT_EQ(scene_object_1.scene->UnpublishLocalVideoTrack(screen_track),
            agora::ERR_OK);
  track_count--;

  EXPECT_TRUE(WaitUnitlVideoIdle(scene_object_1, "screen_video_stream_id_1"));
  scene_object_1.scene->DestroyStream("screen_video_stream_id_1");

  EXPECT_EQ(scene_object_1.scene->GetLocalStreams().size(), track_count);
#endif

  video_ob_1->video_frame_captured_semaphore.Wait(wait_timeout_in_seconds);
  video_ob_2->video_frame_captured_semaphore.Wait(wait_timeout_in_seconds);
#if RTE_WIN || RTE_LINUX
  video_ob_3->video_frame_captured_semaphore.Wait(wait_timeout_in_seconds);
#endif

#if RTE_WIN || RTE_MAC
  video_ob_4->video_frame_captured_semaphore.Wait(wait_timeout_in_seconds);
  assert(video_ob_4->video_frame_captured_count > 0);
  RTE_LOG_VERBOSE << "screen video onframe count : "
                  << video_ob_2->video_frame_captured_count;
#endif

  EXPECT_GE(video_ob_1->video_frame_captured_count, 0);
  EXPECT_GE(video_ob_2->video_frame_captured_count, 0);
#if RTE_WIN || RTE_LINUX
  EXPECT_GE(video_ob_3->video_frame_captured_count, 0);
#endif

  RTE_LOG_VERBOSE << "camera onframe count : "
                  << video_ob_1->video_frame_captured_count;

  RTE_LOG_VERBOSE << "custom video onframe count : "
                  << video_ob_2->video_frame_captured_count;

  RTE_LOG_VERBOSE << "mixed video onframe count : "
                  << video_ob_2->video_frame_captured_count;

  EXPECT_TRUE(LeaveScene(scene_object_1, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, AllAudioTracks) {
  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");
  auto user_2 = GenerateRandomString("test_user");

  auto scene_object_1 = CreateScene(scene_name);
  auto scene_object_2 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Joining Scenes";
  EXPECT_TRUE(JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_2, user_2, token_, expect_ok, wait_event));

  auto audio_frame_ob = std::make_shared<RteAudioFrameObserver>();

  AudioObserverOptions audio_ob_option;
  audio_ob_option.local_option.after_mix = true;
  audio_ob_option.local_option.after_record = true;
  audio_ob_option.local_option.before_playback = true;
  audio_ob_option.remote_option.after_playback_before_mix = true;

  scene_object_1.scene->RegisterAudioFrameObserver(audio_frame_ob,
                                                   audio_ob_option);

  auto microphone = media_control_->CreateMicrophoneAudioTrack();
  microphone->StartRecording();

  scene_object_1.scene->CreateOrUpdateRTCStream("microphone_stream", {""});
  EXPECT_EQ(scene_object_1.scene->PublishLocalAudioTrack("microphone_stream",
                                                         microphone),
            agora::ERR_OK);

  EXPECT_TRUE(WaitUnitlAudioStreaming(scene_object_1, "microphone_stream"));

  auto custom_audio_track = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, custom_audio_track,
                                      "custom_audio_stream_id_1", expect_ok,
                                      wait_event));

  EXPECT_TRUE(
    WaitUnitlAudioStreaming(scene_object_2, "custom_audio_stream_id_1"));

  int expected_num_of_remote_stream = 1;
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_1, expected_num_of_remote_stream));

  EXPECT_EQ(
    scene_object_1.scene->SubscribeRemoteAudio("custom_audio_stream_id_1"),
    agora::ERR_OK);
  // auto custom_audio_track = CreateTestCustomAudioTrack();

  /*EXPECT_TRUE( PublishThreadAudioTrack(scene_object_1, custom_audio_track,
     "custom_audio_stream_id_1", expect_ok, wait_event));*/

  // std::this_thread::sleep_for(std::chrono::seconds(5));

  audio_frame_ob->recorded_audio_frame_semaphore.Wait(wait_timeout_in_seconds);
  audio_frame_ob->playback_audio_frame_semaphore.Wait(wait_timeout_in_seconds);
  audio_frame_ob->mixed_audio_frame_semaphore.Wait(wait_timeout_in_seconds);
  audio_frame_ob->playback_audio_frame_before_mixing_semaphore.Wait(
    wait_timeout_in_seconds);

  EXPECT_GE(audio_frame_ob->recorded_audio_frame_count, 0);
  EXPECT_GE(audio_frame_ob->playback_audio_frame_count, 0);
  EXPECT_GE(audio_frame_ob->mixed_audio_frame_count, 0);
  EXPECT_GE(audio_frame_ob->playback_audio_frame_before_mixing_count, 0);

  RTE_LOG_VERBOSE << "recorded audio frame count : "
                  << audio_frame_ob->recorded_audio_frame_count;

  RTE_LOG_VERBOSE << "playback audio frame count : "
                  << audio_frame_ob->playback_audio_frame_count;

  RTE_LOG_VERBOSE << "mixed audio frame count : "
                  << audio_frame_ob->mixed_audio_frame_count;

  RTE_LOG_VERBOSE << "mixed audio frame count : "
                  << audio_frame_ob->playback_audio_frame_before_mixing_count;
}

TEST_F(TestSceneBasic, DirectCDN) {
  CreateDefaultScene();

  EXPECT_TRUE(JoinScene(test_scene_obj_, default_user_id_, token_, expect_ok,
                        wait_event));

  const char* stream_name_cdn = "my_test_cdn_stream_cdn";

  // TODO(yejun): remove hardcoded CDN link
  test_scene_obj_.scene->CreateOrUpdateDirectCDNStream(
    stream_name_cdn, {"rtmp://10.82.0.193/utlive/rte_cdn_streaming_1"});

  auto microphone = media_control_->CreateMicrophoneAudioTrack();
  microphone->StartRecording();

  auto camera_track = media_control_->CreateCameraVideoTrack();

  camera_track->StartCapture();

  VideoEncoderConfiguration video_config;
  video_config.codecType = agora::rtc::VIDEO_CODEC_H264;
  video_config.dimensions = {1280, 720};
  video_config.bitrate = 1000;
  video_config.frameRate = 15;

  EXPECT_EQ(test_scene_obj_.scene->SetVideoEncoderConfiguration(stream_name_cdn,
                                                                video_config),
            agora::ERR_OK);

  AudioEncoderConfiguration audio_config;
  EXPECT_EQ(test_scene_obj_.scene->SetAudioEncoderConfiguration(stream_name_cdn,
                                                                audio_config),
            agora::ERR_OK);

  EXPECT_EQ(test_scene_obj_.scene->PublishLocalVideoTrack(stream_name_cdn,
                                                          camera_track),
            agora::ERR_OK);

  EXPECT_EQ(test_scene_obj_.scene->PublishLocalAudioTrack(stream_name_cdn,
                                                          microphone),
            agora::ERR_OK);

  EXPECT_TRUE(WaitUnitlVideoStreaming(test_scene_obj_, stream_name_cdn));
  EXPECT_TRUE(WaitUnitlAudioStreaming(test_scene_obj_, stream_name_cdn));

  // Uncomment below code to test real cdn result by open url:
  // rtmp://10.82.0.193/utlive/rte_cdn_streaming_1
  /* std::this_thread::sleep_for(std::chrono::seconds(60));*/

  EXPECT_EQ(test_scene_obj_.scene->UnpublishLocalVideoTrack(camera_track),
            agora::ERR_OK);

  EXPECT_EQ(test_scene_obj_.scene->UnpublishLocalAudioTrack(microphone),
            agora::ERR_OK);

  EXPECT_TRUE(WaitUnitlVideoIdle(test_scene_obj_, stream_name_cdn));
  EXPECT_TRUE(WaitUnitlAudioIdle(test_scene_obj_, stream_name_cdn));

  test_scene_obj_.scene->DestroyStream("my_test_cdn_stream_cdn");
}

TEST_F(TestSceneBasic, ThreeUsersPubAndOneUserSubInCompatibleMode) {
  agora::rte::SdkProfile profile;
  profile.appid = RteTestGloablSettings::GetAppId();
  constexpr bool enable_compatible_mode = true;
  AgoraRteSDK::Init(profile, enable_compatible_mode);
  // We could create two scenes, and publish/Unpublish tracks, then
  // subscribe/Unsubscribe each other
  //

  auto scene_name =
    GenerateRandomString("default_rte_scene_name");

  auto user_1 = GenerateRandomString("test_user");
  auto user_2 = GenerateRandomString("test_user");
  auto user_3 = GenerateRandomString("test_user");
  auto user_4 = GenerateRandomString("test_user");

  SceneConfig config;
  config.scene_type = SceneType::kCompatible;

  auto scene_object_1 = CreateScene(scene_name, config);
  auto scene_object_2 = CreateScene(scene_name, config);
  auto scene_object_3 = CreateScene(scene_name, config);
  auto scene_object_4 = CreateScene(scene_name, config);

  RTE_LOG_VERBOSE << "Joining Scenes";
  EXPECT_TRUE(JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_2, user_2, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_3, user_3, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_4, user_4, token_, expect_ok, wait_event));

  int expected_num_of_remote_usr = 3;
  EXPECT_TRUE(
    WaitForRemoteUserJoined(scene_object_4, expected_num_of_remote_usr));

  EXPECT_EQ(scene_object_4.scene->GetRemoteUsers().size(), 3);

  auto audio_track_1 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_1, audio_track_1, user_1,
                                      expect_ok, !wait_event));

  auto custom_video_track_1 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_1, custom_video_track_1,
                                      user_1, expect_ok, !wait_event));

  auto audio_track_2 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, audio_track_2, user_2,
                                      expect_ok, !wait_event));

  auto custom_video_track_2 = CreateTestCustomVideoTrack();
  EXPECT_TRUE(PublishThreadVideoTrack(scene_object_2, custom_video_track_2,
                                      user_2, expect_ok, !wait_event));

  auto audio_track_3 = CreateTestCustomAudioTrack();

  EXPECT_TRUE(PublishThreadAudioTrack(scene_object_3, audio_track_3, user_3,
                                      expect_ok, !wait_event));

  // Only 3 stream for compatible mode
  //
  int expected_num_of_remote_stream = 3;
  EXPECT_TRUE(
    WaitForRemoteStreamJoined(scene_object_4, expected_num_of_remote_stream));

  // Only 3 stream for compatible mode
  //
  auto remote_streams = scene_object_4.scene->GetRemoteStreams();
  EXPECT_EQ(remote_streams.size(), 3);

  int found_stream = 0;
  for (auto& stream_name : remote_streams) {
    if (stream_name.stream_id == user_1) {
      found_stream++;
      EXPECT_EQ(stream_name.user_id,
                scene_object_1.scene->GetLocalUserInfo().user_id);
    }

    if (stream_name.stream_id.find(user_2) != std::string::npos) {
      found_stream++;
      EXPECT_EQ(stream_name.user_id,
                scene_object_2.scene->GetLocalUserInfo().user_id);
    }

    if (stream_name.stream_id.find(user_3) != std::string::npos) {
      found_stream++;
      EXPECT_EQ(stream_name.user_id,
                scene_object_3.scene->GetLocalUserInfo().user_id);
    }
  }

  EXPECT_EQ(found_stream, 3);

  EXPECT_EQ(scene_object_4.scene->GetRemoteStreamsByUserId(user_1).size(), 1);
  EXPECT_EQ(scene_object_4.scene->GetRemoteStreamsByUserId(user_2).size(), 1);
  EXPECT_EQ(scene_object_4.scene->GetRemoteStreamsByUserId(user_3).size(), 1);

  EXPECT_TRUE(LeaveScene(scene_object_1, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_2, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_3, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_4, expect_ok, wait_event));
}

TEST_F(TestSceneBasic, simple_adjust_remote_user_volume) {
  auto scene_name = GenerateRandomString("default_rte_scene_name");

  auto user_1 = "aaa";
  auto user_2 = "bbb";

  auto scene_object_1 = CreateScene(scene_name);
  auto scene_object_2 = CreateScene(scene_name);

  RTE_LOG_VERBOSE << "Join Scenes";
  EXPECT_TRUE(JoinScene(scene_object_1, user_1, token_, expect_ok, wait_event));
  EXPECT_TRUE(JoinScene(scene_object_2, user_2, token_, expect_ok, wait_event));

  EXPECT_TRUE(WaitForRemoteUserJoined(scene_object_1, 1));
  EXPECT_TRUE(WaitForRemoteUserJoined(scene_object_2, 1));

  EXPECT_EQ(scene_object_1.scene->GetRemoteUsers().size(), 1);
  EXPECT_EQ(scene_object_2.scene->GetRemoteUsers().size(), 1);

  auto audio_track_1 = CreateTestCustomAudioTrack();
  EXPECT_TRUE(
    PublishThreadAudioTrack(scene_object_1, audio_track_1, "my_test_stream_1",
                            expect_ok, !wait_event));

  //auto audio_track_2 = CreateTestCustomAudioTrack();
  //EXPECT_TRUE(PublishThreadAudioTrack(scene_object_2, audio_track_2, "my_test_stream_2", expect_ok, wait_event));

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_TRUE(WaitForRemoteStreamJoined(scene_object_2, 1));
  //EXPECT_TRUE(WaitForRemoteStreamJoined(scene_object_2, 1));
  auto remote_stream_2 = scene_object_2.scene->GetRemoteStreams();
  EXPECT_EQ(remote_stream_2.size(), 1);
  EXPECT_EQ(remote_stream_2[0].stream_id, "my_test_stream_1");

  RTE_LOG_VERBOSE << "RemoteStreamSize: " << remote_stream_2.size()
                  << ", stream_id: " << remote_stream_2[0].stream_id;

  scene_object_2.scene->SubscribeRemoteAudio("my_test_stream_1");
  EXPECT_TRUE(WaitForSubscribeSucceeded(scene_object_2, 1, MediaType::kAudio,
                                        "my_test_stream_1"));

  int volumes[] = {70, 0, 130, 30, 100, -60};
  int expected_volumes[] = {70, 0, 100, 30, 100, 0};

  for (int index = 0; index < sizeof(volumes) / sizeof(int); ++index) {
    RTE_LOG_VERBOSE << "Set volume " << volumes[index] << " , expected volume "
                    << expected_volumes[index];
    EXPECT_EQ(scene_object_2.scene->AdjustUserPlaybackSignalVolume(
      remote_stream_2[0].stream_id, volumes[index]), agora::ERR_OK);

    int current_volume = 0;
    EXPECT_EQ(scene_object_2.scene->GetUserPlaybackSignalVolume(
      remote_stream_2[0].stream_id, &current_volume), agora::ERR_OK);
    EXPECT_EQ(current_volume, expected_volumes[index]);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
  }

  EXPECT_TRUE(
    UnpublishThreadAudioTrack(scene_object_1, audio_track_1, expect_ok,
                              wait_event));

  RTE_LOG_VERBOSE << "Waiting for my_test_stream_1 leave";
  scene_object_1.scene->DestroyStream("my_test_stream_1");

  EXPECT_TRUE(WaitForRemoteStreamLeft(scene_object_2, 1));
  // EXPECT_EQ(scene_object_2.scene->GetRemoteStreams().size(), 0);

  EXPECT_TRUE(LeaveScene(scene_object_1, expect_ok, wait_event));
  EXPECT_TRUE(LeaveScene(scene_object_2, expect_ok, wait_event));
}

