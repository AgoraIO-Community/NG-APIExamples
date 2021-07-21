//
//  Agora SDK
//
//  Copyright (c) 2018 Agora.io. All rights reserved.
//

#pragma once  // NOLINT(build/header_guard)

#include "api2/IAgoraService.h"
#include "base/AgoraBase.h"
#include "base/AgoraRefPtr.h"

namespace agora {
namespace rtc {
class IMediaPlayerObserver;
class ILocalAudioTrack;
/** This interface provide access to a media player. You have to create multiple players
 * in order to playback multiple media sources simultaneously.
 */
class IMediaPlayer : public RefCountInterface {
 public:
  enum PlayerState {
    // Default state
    STATE_IDLE = 0,
    // Start connecting to the source URL.
    STATE_CONNECTING = 1,
    // State after calling preloadToBuffer() and before preloading completed.
    STATE_BUFFERING = 2,
    // Connected to the source URL, Preload completed, or player is paused.
    STATE_PAUSED = 3,
    // Playing
    STATE_PLAYING = 4,
    // STATE_FASTFORWARDING = 5,
    // STATE_REWINDING = 6,
    STATE_FAILED = 7
  };

  enum PlaySpeed { SPEED_PAUSE = 0, SPEED_PLAY = 1000 };

 public:
  // Play an media file using specific url
  virtual void play(const char* url) = 0;
  virtual void playFromPosition(const char* url, int position) = 0;
  // Default speed is PACE_PLAY
  virtual void setPlaySpeed(PlaySpeed spped) = 0;
  // Preload a url into internal buffer.
  virtual void preloadToBuffer(const char* url) = 0;
  virtual void playLocalBuffer(int bufferHandle) = 0;
  virtual void playLocalBufferFromPosition(int bufferHandle, int position) = 0;
  // Release buffer manually. The buffer will be released automatically when player is
  // released.
  virtual void releaseBuffer(int bufferHandle) = 0;
  // This controls the volume for local playback. If you want to adjust volume for
  // publication, then you could call createAudioTrackForPublication to get a local
  // track and use ILocalAudioTrack::adjustSignalingVolume().

  // Volume from [0, 100], 0 means mute the track and 100 is original volume level.
  virtual void adjustPlaybackSignalLevel(int level) = 0;
  // Create a local track for publication. Then you can send this audio track of media player
  // to remote ends, using ILocalUser::PublishAudio().
  virtual ILocalAudioTrack* createAudioTrackForPublication() = 0;
  virtual void getPosition(int& position) = 0;
  virtual void getState(PlayerState& state) = 0;
  virtual void close() = 0;

  virtual void registerPlayerObserver(IMediaPlayerObserver* observer) = 0;
  virtual void unregisterPlayerObserver(IMediaPlayerObserver* observer) = 0;

 protected:
  ~IMediaPlayer() {}
};

class IMediaPlayerObserver {
  virtual void onPlayerStateChanged(const IMediaPlayer::PlayerState& state) = 0;
  virtual void onPositionChanged(const int position) = 0;
  // Callback for preload.
  virtual void onPreloadSuccess(const char* url, int bufferHandler) = 0;
  virtual void onPreloadFailure(const char* url, ERROR_CODE_TYPE error) = 0;
};

}  // namespace rtc
}  // namespace agora
