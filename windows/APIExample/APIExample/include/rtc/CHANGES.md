Note: Please update this file for every Agora API change you do. Simply fill in
your updates in the Working section below.


Agora High Level APIs (Working)
==================================================

API (yyyy-mm-dd)
==================================================
Purpose of this change

API file name #1
-------------
**Add:**
Short description
- Foo()
- Bar()

**Modified:**
Short description
- Changes Foo() to Foo1()
- Changes Bar() to Bar1()

**Deleted:**
Short description
- Deleted Foo()

API (2021-07-19)
==================================================
AgoraMediaBase.h
-------------
**Modified:**
- onPlaybackAudioFrameBeforeMixing(uid_t uid, AudioFrame& audioFrame) to
  onPlaybackAudioFrameBeforeMixing(base::user_id_t userId, AudioFrame& audioFrame)

API (2021-07-06)
==================================================
AgoraMediaBase.h
-------------
**Modified:**
- void onFrame(AudioPcmFrame* frame)
API (2021-06-18)
==================================================
Add methods for media player source

IAgoraMediaPlayerSource.h
-------------
**Add:**
- add method muteAudio()
- add method isAudioMuted()
- add method muteVideo()
- add method isVideoMuted()

API (2021-06-15)
==================================================
Add methods for streaming source interfance and observer

IAgoraMediaStreamingSource.h
-------------
**Add:**
- add method IMediaStreamingSource::parseMediaInfo()
- add method IMediaStreamingSourceObserver::onOpenDone()
- add method IMediaStreamingSourceObserver::onSeekDone()


API (2021-06-07)
==================================================
Rename AES-GCM to AES-GCM2

AgoraBase.h
-------------
**Modified:**
- rename ENCRYPTION_MODE::AES_128_GCM to ENCRYPTION_MODE::AES_128_GCM2
- rename ENCRYPTION_MODE::AES_256_GCM to ENCRYPTION_MODE::AES_256_GCM2

API (2020-05-31)
==================================================
Parse media info from url.

IAgoraMediaStreamingSource.h
-------------
**Add:**
- ParseMediaInfo

API (2021-05-25)
==================================================
adding operator== function for VideoDimensions and SimulcastStreamConfig

AgoraBase.h
-------------
**Add:**
- bool operator==(const VideoDimensions& rhs) const
- bool operator==(const SimulcastStreamConfig& rhs) const

API (2020-05-24)
==================================================
Support compute audio spectrum.

AgoraMediaBase.h
-------------
**Modified:**
- Update comments

API (2021-05-22)
==================================================
Modify note of AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_CHATROOM

AgoraBase.h
-------------
**Modified:**
- note of AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_CHATROOM


API (2021-06-02)
==================================================
Add new configurable keys

IAgoraParameter.h
-------------
**Add:**
- Add new key KEY_RTC_ENABLE_BUILT_IN_MEDIA_ENCRYPTION


API (2020-05-15)
==================================================
Add new configurable keys

IAgoraParameter.h
-------------
**Add:**
- Add new key KEY_RTC_AUDIO_INPUT_SAMPLE_RATE

API (2020-05-13)
==================================================
Support compute audio spectrum.

AgoraMediaBase.h
-------------
**Modified:**
- struct UserAudioSpectrumInfo
- IAudioSpectrumObserver

API (2021-05-12)
==================================================
IAgoraParameter.h
-------------
**Add:**
Add parameter key KEY_RTC_AUDIO_MAX_TARGET_DELAY

API (2020-05-11)
==================================================
Support Freeze check on media player based ffmpeg.

AgoraMediaPlayerTypes.h
-------------
**Add:**
Add in definition of MEDIA_PLAYER_EVENT
- PLAYER_EVENT_FREEZE_START
- PLAYER_EVENT_FREEZE_STOP

API (2021-05-10)
==================================================
Add some protection

AgoraBase.h
-------------
**Modified:**
- DownlinkNetworkInfo

API (2021-05-06)
==================================================
Add switches for c++11 features

API (2020-05-05)
==================================================
Support R/W recording audio frame observer.

AgoraMediaBase.h
-------------
**Modified:**
Move in definition of RAW_AUDIO_FRAME_OP_MODE_TYPE


API (2021-05-06)
==================================================
Delete RemoteVideoStreamInfo and refine DownlinkNetworkInfo.

AgoraBase.h
-------------
**Deleted:**
- RemoteVideoStreamInfo

**Modified:**
- refine DownlinkNetworkInfo.

API (2020-04-30)
==================================================
Add new configurable keys

IAgoraParameter.h
-------------
**Add:**
- Add new key KEY_RTC_AUDIO_OPENSL_MODE


API (2020-04-29)
==================================================
AgoraBase.h
-------------
**Add:**
- AUDIO_SCENARIO_TYPE::AUDIO_SCENARIO_CHORUS


API (2021-04-30)
==================================================
AgoraBase.h
-------------
**Add:**
- LOCAL_VIDEO_STREAM_ERROR::LOCAL_VIDEO_STREAM_ERROR_BACKGROUD
- LOCAL_VIDEO_STREAM_ERROR::LOCAL_VIDEO_STREAM_ERROR_MULTIPLE_FOREGROUND_APPS
- LOCAL_VIDEO_STREAM_ERROR::LOCAL_VIDEO_STREAM_ERROR_SYSTEM_PRESSURE


API (2021-04-28)
==================================================
AgoraBase.h
-------------
**Add:**
- Add field mosValue for struct RemoteAudioStats

API (2021-04-20)
==================================================
Support AES-GCM encryption for media packet

AgoraBase.h
-------------
**Add:**
- ENCRYPTION_MODE::AES_128_GCM
- ENCRYPTION_MODE::AES_256_GCM
- EncryptionConfig::encryptionKdfSalt

API (2020-04-19)
==================================================
Add copy assignment for AudioPcmFrame.
AgoraMediaBase.h
-------------
**Add:**
Add AudioPcmFrame & operator=(const AudioPcmFrame & src)

API (2020-04-19)
==================================================
Add audio effect preset enum.
AgoraBase.h
-------------
**Add:**
Add VOICE_BEAUTIFIER_PRESET, AUDIO_EFFECT_PRESET, VOICE_CONVERSION_PRESET

API (2021-04-16)
==================================================

AgoraMediaBase.h
-------------
**Add:**
- Add new struct AudioSpectrumInfo
- Add new interface IAudioSpectrumObserver
API (2021-04-07)
==================================================
AgoraBase.h
-------------
**Add:**
- add AudioEncodedFrameObserverConfig
- add IAudioEncodedFrameObserver

API (2021-04-15)
==================================================
Add support for win arm64

AgoraBase.h
-------------
**Add:**
- Add support for win arm64

**Deleted:**
- ENCRYPTION_MODE::AES_128_XTS
- ENCRYPTION_MODE::AES_128_ECB
- ENCRYPTION_MODE::AES_256_XTS

API (2021-04-15)
==================================================
add chatroom audio scenario

AgoraBase.h
-------------
**Deleted:**
- Deleted enum AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_CHATROOM_ENTERTAINMENT
- Deleted enum AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_EDUCATION
- Deleted enum AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_SHOWROOM

**Modify:**
- modify enum AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_CHATROOM_GAMING to AUDIO_SCENARIO_TYPE#AUDIO_SCENARIO_CHATROOM

API (2021-04-06)
==================================================

AgoraMediaBase.h
-------------
**Add:**
- Add new interface IVideoEncodedFrameObserver

API (2021-03-31)
==================================================
IAgoraMediaStreamingSource.h
-------------
**Add:**
- add interface IMediaStreamingSource


API (2021-03-26)
==================================================
Add new parameters in ScreenCaptureParameters

AgoraBase.h
-------------
**Add:**
- Add new parameter `bool captureMouseCursor`
- Add new parameter `bool windowFocus`

API (2021-03-24)
==================================================
AgoraBase.h
-------------
**Add:**
- add struct EAR_MONITORING_FILTER_TYPE

API file name #2
-------------

API (2021-04-22)
==================================================

AgoraMediaPlayerTypes.h
-------------
**Add:**
- Add new enum value MEDIA_PLAYER_ERROR::PLAYER_ERROR_NO_PERMISSION
- Add new enum value MEDIA_PLAYER_ERROR::PLAYER_ERROR_INTERRUPTED


API (2020-04-01)
==================================================
add MEDIA_PLAYER_EVENT enum and api in AgoraMediaPlayerTypes.h

AgoraMediaPlayerTypes.h
-------------
**Add:**
- add PLAYER_EVENT_BUFFER_LOW and PLAYER_EVENT_BUFFER_RECOVER

IAgoraMediaPlayerSource.h
-------------
**Add:**
- setPlayerOption
- onPlayBufferUpdated


API (2020-03-26)
==================================================
modify struct VideoTrackInfo

API (2021-03-25)
==================================================
AgoraBase.h
-------------
**Add:**
- add DataStreamConfig

API (2020-03-19)
==================================================
modify to adapt c++ 11

API (2021-03-11)
==================================================
AgoraBase.h
-------------
**Add:**
- add err_num :   ERR_PCMSEND_FORMAT and ERR_PCMSEND_BUFFEROVERFLOW

API (2021-03-09)
==================================================
Modify PLAYER_STATE_STOPPED

AgoraMediaPlayerTypes.h
-------------
**Modify:**
- PLAYER_STATE_STOPPED


API (2021-03-09)
==================================================
Add new parameters in ScreenCaptureParameters

AgoraBase.h
-------------
**Add:**
- Add new parameter `view_t *excludeWindowList`
- Add new parameter `int excludeWindowCount`

API (2021-02-19)
==================================================
Add VIDEO_CODEC_GENERIC_JPEG enum
Add parameter key

AgoraBase.h
-------------
**Add:**
- Add VIDEO_CODEC_GENERIC_JPEG

IAgoraParameter.h
-------------
**Add:**
Add parameter key KEY_RTC_VIDEO_CODEC_TYPE

API (2021-01-28)
==================================================
Add new configurable keys

IAgoraParameter.h
-------------
**Add:**
- Add new key KEY_RTC_VIDEO_HW_ENCODER_PROVIDER

API (2021-01-12)
==================================================
Change definition of bytes_per_sample without channel

AgoraMediaBase.h
-------------
**Add:**
- enum BYTES_PER_SAMPLE

**Modified:**
- change param type of AudioFrame#bytesPerSample from size_t to enum BYTES_PER_SAMPLE

API (2021-01-21)
==================================================
Change enum order

AgoraBase.h
-------------

**Modified:**
- Change the order of VIDEO_SOURCE_CUSTOM and VIDEO_SOURCE_MEDIA_PLAYER


API file name #2
-------------

API (2021-03-03)
API (2020-12-24)
==================================================
AgoraMediaBase.h
-------------
**Add:**
- enum MAX_METADATA_SIZE_TYPE
- Add metadata_buffer and metadata_size for struct ExternalVideoFrame

API (2020-12-11)
==================================================

AgoraBase.h
-------------

**Modified:**
getAograCertificateVerifyResult ---> getAgoraCertificateVerifyResult


API (2020-12-07)
==================================================

AgoraBase.h
-------------

**Add:**
- int createAgoraCredential(agora::util::AString &credential);
- int AGORA_CALL getAograCertificateVerifyResult(const char *credential_buf, int credential_len, const char *certificate_buf, int certificate_len);
- void setAgoraLicenseCallback(agora::base::LicenseCallback *callback);
- agora::base::LicenseCallback* getAgoraLicenseCallback();

**Deleted:**
License's API
- int agora_genCredential(agora::util::AString &credential, const char *path)
- int agora_verifyCertificate(const char *cert, const char *path);
- void agora_registerLicenseCallback(agora::base::LicenseCallback* callback);
- void agora_unregisterLicenseCallback();

API (2020-12-21)
==================================================

**Add:**
AgoraBase.h
-------------
- Add enum STREAM_SUBSCRIBE_STATE
- Add enum STREAM_PUBLISH_STATE

API (2020-10-16)
==================================================
Modify the comments of DEGRADATION_PREFERENCE.

AgoraBase.h
-------------
**Modify:**
- enum DEGRADATION_PREFERENCE

API (2020-10-12)
==================================================
Add camera direction for configuration.

AgoraBase.h
-------------
**Add:**
- enum CAMERA_DIRECTION
- struct CameraCapturerConfiguration

API (2020-09-28)
==================================================
Add mirror mode to VideoEncoderConfiguration.

AgoraBase.h
-------------

**Add:**
- Mirror mode (VIDEO_MIRROR_MODE_TYPE) in VideoEncoderConfiguration


API (2020-09-27)
==================================================
Add mirror mode to VideoCanvas.

AgoraBase.h
-------------

**Add:**
- Mirror mode (VIDEO_MIRROR_MODE_TYPE) in VideoCanvas


API (2021-02-02)
==================================================
Add DownlinkNetworkInfo and modify NetworkInfo

AgoraBase.h
-------------
**Add:**
- struct DownlinkNetworkInfo

**Modified:**
- Changes struct NetworkInfo to struct UplinkNetworkInfo


API (2020-09-23)
==================================================
AgoraBase.h
-------------
**Add:**
- Add VIDEO_SOURCE_MEDIA_PLAYER for enum VIDEO_SOURCE_TYPE
- Add field priv_size for struct VideoCanvas

AgoraMediaBase.h
-------------
**Add:**
- Add onMediaPlayerVideoFrame() member function for IVideoFrameObserver


API (2020-11-25)
==================================================
Modify REMOTE_VIDEO_STREAM_TYPE.

AgoraBase.h
-------------
**Add:**
- enum VIDEO_STREAM_TYPE

**Deleted:**
- Deleted enum REMOTE_VIDEO_STREAM_TYPE

**Modified:**
- Changes Add VIDEO_STREAM_TYPE streamType to EncodedVideoFrameInfo


API (2020-11-24)
==================================================
Add tx/rxPacketLossRate in RtcStats.

AgoraBase.h
-------------
**Add:**
- RtcStats::txPacketLossRate
- RtcStats::rxPacketLossRate


API (2020-11-14)

AgoraMediaPlayerTypes.h
-------------
**Add:**
- MEDIA_PLAYER_STATE::PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED
=======
API (2020-11-12)
==================================================
- CHANNEL_PROFILE_TYPE::CHANNEL_PROFILE_LIVE_BROADCASTING2

AgoraBase.h
-------------
CHANNEL_PROFILE_TYPE::CHANNEL_PROFILE_LIVE_BROADCASTING2


API (2020-11-09)

AgoraBase.h
-------------
**Add:**
License's API
- int agora_genCredential(agora::util::AString &credential, const char *path)
- int agora_verifyCertificate(const char *cert, const char *path);
- void agora_registerLicenseCallback(agora::base::LicenseCallback* callback);
- void agora_unregisterLicenseCallback();

License's Callback
- class LicenseCallback;

**Modified:**
License's Error Code Types
- ERR_LICENSE_CREDENTIAL_INVALID = 131,
- ERR_CERT_RAW = 157,
- ERR_CERT_JSON_PART = 158,
- ERR_CERT_JSON_INVAL = 159,
- ERR_CERT_JSON_NOMEM = 160,
- ERR_CERT_CUSTOM = 161,
- ERR_CERT_CREDENTIAL = 162,
- ERR_CERT_SIGN = 163,
- ERR_CERT_FAIL = 164,
- ERR_CERT_BUF = 165,
- ERR_CERT_NULL = 166,
- ERR_CERT_DUEDATE = 167,
- ERR_CERT_REQUEST = 168,


API (2020-09-04)
==================================================

AgoraBase.h
-------------
**Modified:**
Refine AGORA_API definitions to keep aligned with agora_api.h

AgoraRefPtr.h
-------------
**Modified:**
Add operator*().

IAgoraLog.h
-------------
**Modified:**
Add log related constants.

IAgoraMediaPlayerSource.h
-------------
**Modified:**
Change the 'int' parameters to 'int64_t' to keep aligned with the other APIs.


API (2020-09-09)
==================================================
Modify EncodedVideoFrameInfo.

AgoraBase.h
-------------
**Modify:**
Modify EncodedVideoFrameInfo.
- Remove packetizationMode.


API (2020-09-11)
==================================================
Add enum NETWORK_TYPE.

AgoraBase.h
-------------
**Add:**
- enum NETWORK_TYPE


API (2020-09-04)
==================================================
Add more methods to IVideoFrameObserver.

AgoraMediaBase.h
-------------
**Modified:**
- Changes Add getVideoFormatPreference/getRotationApplied/getMirrorApplied to IVideoFrameObserver


API (2020-09-03)
==================================================

AgoraBase.h
-------------
**Modified:**
add members of RtcStats
- packetsBeforeFirstKeyFramePacket
- firstAudioPacketDurationAfterUnmute
- firstVideoPacketDurationAfterUnmute
- firstVideoKeyFramePacketDurationAfterUnmute
- firstVideoKeyFrameDecodedDurationAfterUnmute
- firstVideoKeyFrameRenderedDurationAfterUnmute


API (2020-08-19)
==================================================

AgoraMediaBase.h
-------------
**Add:** OpenGL type
- enum EGL_CONTEXT_TYPE

**Add:** Video Buffer types
- VIDEO_BUFFER_TYPE::VIDEO_BUFFER_ARRAY
- VIDEO_BUFFER_TYPE::VIDEO_BUFFER_TEXTURE

**Add:** Texture related parameter
- ExternalVideoFrame::eglContext
- ExternalVideoFrame::eglType
- ExternalVideoFrame::textureId


API (2020-08-14)
==================================================
Add enum

AgoraBase.h
-------------
**Add:**
Add enum
- VIDEO_SOURCE_TYPE


API (2020-08-04)
==================================================

AgoraBase.h
-------------
**Add:**
- AUDIO_CODEC_TYPE::AUDIO_CODEC_G722
- AUDIO_PROFILE_TYPE::AUDIO_PROFILE_IOT


API (2020-07-29)
==================================================

AgoraMediaBase.h
-------------
Add `struct AudioParameters`
Rename some fields of `struct AudioPcmFrame`


API (2020-07-14)
==================================================
IAgoraMediaPlayerSource.h
-------------
**Modify:**
Change include dir.


API (2020-07-06)
==================================================
AgoraBase.h
-------------
**Modify:**
Change include dir.


API (2020-07-01)
==================================================
AgoraBase.h
-------------
**Modify:**
Remove string UID.


API (2020-06-27)
==================================================
Make include path correct


API (2020-06-18)
==================================================
Modify EncodedVideoFrameInfo.

AgoraBase.h
-------------
**Modify:**
Modify EncodedVideoFrameInfo.
- Remove packetizationMode.


API (2020-06-17)
Change namespace to avoid confliect with media_server_library

AgoraOptional.h
-------------
**Modified:**
Change namespace
- Changes base::Optional to base_utils::Optional


API (2020-06-17)
==================================================
Fix header file macro

AgoraOptional.h
-------------
**Add:**
Add string_value to Optional
- Optional::string_value()

**Modified:**
- `BASE_OPTIONAL_H_` to `AGORA_OPTIONAL_H_`


API (2020-05-29)
==================================================
Updated docs for the following header files:
- AgoraBase.h
- AgoraMediaBase.h


API (2020-06-22)
==================================================

IAgoraMediaPlayerSource.h
-------------
**Modified:**
- setLooping(bool looping) to setLoopCount(int loopCount)


API (2020-06-10)
==================================================
Add NetworkInfo for feedback to user

AgoraBase.h
-------------
**Add:**
Add NetworkInfo
- struct NetworkInfo


API (2020-06-08)
==================================================

IAgoraMediaPlayerSource.h
-------------
**Add:**
Add new Api in Media Player
- takeScreenshot()
- selectInternalSubtitle()
- setExternalSubtitle()


API (2020-06-08)
==================================================

AgoraBase.h
-------------
**Modified:**
- REMOTE_VIDEO_STREAM_STATE rename to REMOTE_VIDEO_STATE
- Changes REMOTE_VIDEO_STATE_PLAYING to REMOTE_VIDEO_STATE_DECODING

REMOTE_VIDEO_STATE_REASON
- REMOTE_VIDEO_STATE_REASON_INTERNAL
- REMOTE_VIDEO_STATE_REASON_NETWORK_CONGESTION
- REMOTE_VIDEO_STATE_REASON_NETWORK_RECOVERY
- REMOTE_VIDEO_STATE_REASON_LOCAL_MUTED
- REMOTE_VIDEO_STATE_REASON_LOCAL_UNMUTED
- REMOTE_VIDEO_STATE_REASON_REMOTE_MUTED
- REMOTE_VIDEO_STATE_REASON_REMOTE_UNMUTED
- REMOTE_VIDEO_STATE_REASON_REMOTE_OFFLINE
- REMOTE_VIDEO_STATE_REASON_AUDIO_FALLBACK
- REMOTE_VIDEO_STATE_REASON_AUDIO_FALLBACK_RECOVERY


API (2020-06-03)
=========================

Move IAgoraMediaPlayerSource.h from api2 to here, and add getSourceId() for IMediaPlayerSource

**Add:**
AgoraMediaBase.h
-------------
Add enum MEDIA_PLAYER_SOURCE_TYPE.



API (2020-05-31)
=========================

AgoraMediaBase.h
-------------
**Add:**
- Add enum MEDIA_PLAYER_PLAYBACK_SPEED.

IAgoraMeidaPlayerSource.h
-------------
**Add:**
Add new Api in Media Player
- changePlaybackSpeed()
- selectAudioTrack()
- setPlayerOption()

AgoraMeidaPlayerTypes.h
-------------
**Add:**
Move Media Player related declaration from AgoraMediaBase.h to AgoraMeidaPlayerTypes.h
- enum MEDIA_PLAYER_STATE
- enum MEDIA_PLAYER_ERROR
- enum MEDIA_PLAYER_PLAYBACK_SPEED
- enum MEDIA_STREAM_TYPE
- enum MEDIA_PLAYER_EVENT
- struct MediaStreamInfo
- enum MEDIA_PLAYER_METADATA_TYPE

AgoraMediaBase.h
-------------
**Delete:**
Move Media Player related declaration from AgoraMediaBase.h to AgoraMeidaPlayerTypes.h
- enum MEDIA_PLAYER_STATE
- enum MEDIA_PLAYER_ERROR
- enum MEDIA_PLAYER_PLAYBACK_SPEED
- enum MEDIA_STREAM_TYPE
- enum MEDIA_PLAYER_EVENT
- struct MediaStreamInfo
- enum MEDIA_PLAYER_METADATA_TYPE


API (2020-06-09)
==================================================
Modify PacketOptions.

AgoraMediaBase.h
-------------
**Modified:**
Add audioLevelIndication into PacketOptions.
- PacketOptions: Add member audioLevelIndication


API (2020-05-28)
==================================================
Modify VideoEncoderConfiguration and VIDEO_CODEC_TYPE

AgoraBase.h
-------------
**Modified:**
- Add member enableGenericH264 for VideoEncoderConfiguration
- Add new type VIDEO_CODEC_GENERIC(6) for enum VIDEO_CODEC_TYPE
- Add new type VIDEO_FRAME_TYPE_DROPPABLE_FRAME(6) for enum VIDEO_FRAME_TYPE


API (2020-05-22)
=========================
**Add:**
AgoraMediaBase.h
-------------
Add enum VIDEO_OBSERVER_POSITION.


API (2020-05-12)
=========================

**Add:**
AgoraMediaBase.h
-------------
Add data member videoRotation in struct MediaStreamInfo.

API (2020-05-12)
==================================================
Add lastmile and connection state enum

AgoraBase.h
-------------
**Add:**
Add lastmile and connection state enum
- enum LASTMILE_PROBE_RESULT_STATE
- struct LastmileProbeOneWayResult
- struct LastmileProbeResult
- struct LastmileProbeConfig
- enum CONNECTION_CHANGED_REASON_TYPE
- struct LastmileProbeConfig
- enum AUDIO_REVERB_PRESET
- struct ScreenCaptureParameters
- enum VOICE_CHANGER_PRESET


API (2020-05-09)

AgoraBase.h
--------------------
**Add:**
Add AUDIO_SCENARIO_HIGH_DEFINITION for AUDIO_SCENARIO_TYPE

API (2021-05-26)
=========================
**Add:**

AgoraBase.h
-------------
Add enum DISABLE to disable VQC to DEGRADATION_PREFERENCE
