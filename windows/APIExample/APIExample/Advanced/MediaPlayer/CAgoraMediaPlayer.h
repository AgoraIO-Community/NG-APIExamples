#pragma once
#include "AGVideoWnd.h"
#include <IAgoraMediaPlayer.h>
#include <IAgoraMediaPlayerSource.h>
#define VIDEO_SLIDER_RANGE		10000
using namespace agora;
using namespace agora::base;

class MediaPlayerObserver final : public agora::rte::IAgoraRteMediaPlayerObserver {
public:
	MediaPlayerObserver(std::shared_ptr<agora::rte::IAgoraRteMediaPlayer> player)
		: player_(player)
	{}
	~MediaPlayerObserver() {}

	void SetMsgReceiver(HWND hWnd) { m_hMsgHanlder = hWnd; }
	void OnPlayerStateChanged(const agora::rte::RteFileInfo& current_file_info,
		agora::rte::MEDIA_PLAYER_STATE state,
		agora::rte::MEDIA_PLAYER_ERROR ec) override {
		
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(mediaPLAYER_STATE_CHANGED), (WPARAM)state, (LPARAM)ec);
	}

	void OnPositionChanged(const agora::rte::RteFileInfo& current_file_info,
		int64_t position) override {
		if(m_hMsgHanlder)
		    ::PostMessage(m_hMsgHanlder, WM_MSGID(mediaPLAYER_POSTION_CHANGED), (WPARAM)new int64_t(position), NULL);
	}

	void OnPlayerEvent(const agora::rte::RteFileInfo& current_file_info,
		agora::rte::MEDIA_PLAYER_EVENT event) override {
	

		switch (event) {
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_COMPLETE:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_ERROR:
			
			break;

		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_SEEK_BEGIN:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_VIDEO_PUBLISHED:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_AUDIO_PUBLISHED:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_AUDIO_TRACK_CHANGED:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_BUFFER_LOW:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_BUFFER_RECOVER:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_FREEZE_START:
		case agora::rte::MEDIA_PLAYER_EVENT::PLAYER_EVENT_FREEZE_STOP:
		default:
			break;
		}
	}

	void OnAllMediasCompleted(int32_t err_code) override {
		
	}

	void OnMetadata(const agora::rte::RteFileInfo& current_file_info,
		agora::rte::MEDIA_PLAYER_METADATA_TYPE type, const uint8_t* data,
		uint32_t length) override {
		
	}

	void OnPlayerBufferUpdated(const agora::rte::RteFileInfo& current_file_info,
		int64_t playCachedBuffer) override {}

	void OnAudioFrame(const agora::rte::RteFileInfo& current_file_info,
		const agora::rte::AudioPcmFrame& audio_frame) override {
		
	}

	void OnVideoFrame(const agora::rte::RteFileInfo& current_file_info,
		const agora::rte::VideoFrame& video_frame) override {
		
	}

private:
	HWND m_hMsgHanlder;
	std::weak_ptr<agora::rte::IAgoraRteMediaPlayer> player_;
};

class CMediaPlayerSourceObserver : public agora::rtc::IMediaPlayerSourceObserver {
public:
	//set the message notify window handler
	void SetMsgReceiver(HWND hWnd) { m_hMsgHanlder = hWnd; }
	virtual ~CMediaPlayerSourceObserver() {}

	/**
	 * Reports the playback state change.
	 *
	 * When the state of the playback changes, the media player occurs this
	 * callback to report the new playback state and the reason or error for the
	 * change.
	 *
	 * @param state The new playback state after change. See
	 * \ref agora::media::base::MEDIA_PLAYER_STATE "MEDIA_PLAYER_STATE"
	 * @param ec The player's error code. See
	 * \ref agora::media::base::MEDIA_PLAYER_ERROR "MEDIA_PLAYER_ERROR"
	 */
	virtual void onPlayerSourceStateChanged(media::base::MEDIA_PLAYER_STATE state,
		media::base::MEDIA_PLAYER_ERROR ec) {
		if(m_hMsgHanlder)
		::PostMessage(m_hMsgHanlder, WM_MSGID(mediaPLAYER_STATE_CHANGED), (WPARAM)state, (LPARAM)ec);
	}

	/**
	 * Reports current playback progress.
	 *
	 * The callback occurs once every one second during the playback and reports
	 * current playback progress.
	 *
	 * @param position Current playback progress (ms).
	 */
	virtual void onPositionChanged(int64_t position)
	{
		::PostMessage(m_hMsgHanlder, WM_MSGID(mediaPLAYER_POSTION_CHANGED), (WPARAM)new int64_t(position), NULL);
	}
	/**
	 * Reports the playback event.
	 *
	 * - After calling the \ref agora::rtc::IMediaPlayer::seek "seek" method,
	 * the media player occurs the callback to report the results of the seek
	 * operation.
	 * - After calling the
	 * \ref agora::rtc::IMediaPlayer::selectAudioTrack "selectAudioTrack" method,
	 * the media player occurs the callback to report that the audio track
	 * changes.
	 *
	 * @param event The playback event. See
	 *  \ref agora::media::base::MEDIA_PLAYER_EVENT "MEDIA_PLAYER_EVENT"
	 * for details.
	 */
	virtual void onPlayerEvent(media::base::MEDIA_PLAYER_EVENT event)
	{

	}

	/**
	 * Reports the reception of the media metadata.
	 *
	 * The callback occurs when the player receivers the media metadata and
	 * reports the detailed information of the media metadata.
	 *
	 * @param data The detailed data of the media metadata.
	 * @param length The data length (bytes).
	 */
	virtual void onMetaData(const void* data, int length)
	{

	}


	/**
	 * @brief Triggered when play buffer updated, once every 1 second
	 *
	 * @param int cached buffer during playing, in milliseconds
	 */
	virtual void onPlayBufferUpdated(int64_t playCachedBuffer)
	{

	}

	/**
	 * Occurs when one playback of the media file is completed.
	 */
	virtual void onCompleted()
	{

	}
private:
	HWND m_hMsgHanlder;
};
class CAgoraMediaPlayerHandler : public agora::rte::IAgoraRteSceneEventHandler
{
public:
	virtual ~CAgoraMediaPlayerHandler() {}
	virtual void OnConnectionStateChanged(agora::rte::ConnectionState old_state, agora::rte::ConnectionState new_state,
		agora::rte::ConnectionChangedReason reason) override {
		if (m_hMsgHanlder)
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_CONNECTION_STATE), (int)old_state, (int)reason);
	}

	virtual void OnRemoteUserJoined(const std::vector<agora::rte::UserInfo>& users) override {}
	virtual void OnRemoteUserLeft(const std::vector<agora::rte::UserInfo>& users) override {}
	//set the message notify window handler
	void SetMsgReceiver(HWND hWnd) { m_hMsgHanlder = hWnd; }
	virtual void OnRemoteStreamAdded(const std::vector<agora::rte::StreamInfo>& streams)  override {}

	/**
	 * Occurs when remote streams are removed.
	 *
	 * @param streams Removed remote streams.
	 */
	virtual void OnRemoteStreamRemoved(const std::vector<agora::rte::StreamInfo>& streams)  override {
	}

	/**
	 * Occurs when the media state of the local stream changes.
	 *
	 * @param streams Information of the local stream.
	 * @param media_type Media type of the local stream.
	 * @param old_state Old state of the local stream.
	 * @param new_state New state of the local stream.
	 * @param reason The reason of the state change.
	 */
	virtual void OnLocalStreamStateChanged(const agora::rte::StreamInfo& streams, agora::rte::MediaType media_type,
		agora::rte::StreamMediaState old_state, agora::rte::StreamMediaState new_state,
		agora::rte::StreamStateChangedReason reason) override {
		if (m_hMsgHanlder) {

			RemoteVideoStateChanged* videoState = new RemoteVideoStateChanged(streams, media_type, old_state, new_state, reason);
			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_VIDEO_STATE_CHANGED), (WPARAM)videoState, 0);
		}
	}

	/**
	 * Occurs when the media state of the remote stream changes.
	 *
	 * @param streams Information of the remote stream.
	 * @param media_type Media type of the remote stream.
	 * @param old_state Old state of the remote stream.
	 * @param new_state New state of the remote stream.
	 * @param reason The reason of the state change.
	 */
	virtual void OnRemoteStreamStateChanged(const agora::rte::StreamInfo& streams, agora::rte::MediaType media_type,
		agora::rte::StreamMediaState old_state, agora::rte::StreamMediaState new_state,
		agora::rte::StreamStateChangedReason reason) override {
		if (m_hMsgHanlder) {

			::PostMessage(m_hMsgHanlder, WM_MSGID(EID_LOCAL_VIDEO_STATE_CHANGED), (int)old_state, (int)reason);
		}
	}

	/**
	 * Reports the volume information of users.
	 *
	 * @param speakers The volume information of users.
	 * @param totalVolume Total volume after audio mixing. The value ranges between 0 (lowest volume) and 255 (highest volume).
	 */
	virtual void OnAudioVolumeIndication(const std::vector<agora::rte::AudioVolumeInfo>& speakers,
		int totalVolume) override {}

	/**
	 * Occurs when the token will expire in 30 seconds for the user.
	 *
	 * //TODO (yejun): expose APIto renew scene token
	 * @param token The token that will expire in 30 seconds.
	 */
	virtual void OnSceneTokenWillExpired(const std::string& scene_id, const std::string& token) override {}

	/**
	 * Occurs when the token has expired for a user.
	 *
	 * //TODO (yejun): expose APIto renew scene token
	 *
	 * @param stream_id The ID of the scene.
	 */
	virtual void OnSceneTokenExpired(const std::string& scene_id)override {}

	/**
	 * Occurs when the token of a stream expires in 30 seconds.
	 * If the token you specified when calling 'CreateOrUpdateRTCStream' expires,
	 * the user will drop offline. This callback is triggered 30 seconds before the token expires, to
	 * remind you to renew the token by calling 'CreateOrUpdateRTCStream' again with new token.
	 * //TODO (yejun): Need to tell how to generate new token, ETA for new token facility
	 * Upon receiving this callback, generate a new token at your app server
	 *
	 * @param stream_id
	 * @param token
	 */

	virtual void OnStreamTokenWillExpire(const std::string& stream_id, const std::string& token) override {}

	/**
	 * Occurs when the token has expired for a stream.
	 *
	 * Upon receiving this callback, you must generate a new token on your server and call
	 * "CreateOrUpdateRTCStream" to pass the new token to the SDK.
	 *
	 * @param stream_id The ID of the scene.
	 */
	virtual void OnStreamTokenExpired(const std::string& stream_id) override {}

	virtual void OnBypassCdnStateChanged(
		const std::string& stream_id, const std::string& target_cdn_url,
		agora::rte::CDNBYPASS_STREAM_PUBLISH_STATE state,
		agora::rte::CDNBYPASS_STREAM_PUBLISH_ERROR err_code) override {}

	virtual void OnBypassCdnPublished(
		const std::string& stream_id, const std::string& target_cdn_url,
		agora::rte::CDNBYPASS_STREAM_PUBLISH_ERROR err_code) override {}

	virtual void OnBypassCdnUnpublished(const std::string& stream_id,
		const std::string& target_cdn_url) override {}

	virtual void OnBypassTranscodingUpdated(const std::string& stream_id) override {}

	virtual void OnSceneStats(const agora::rte::SceneStats& stats) override {}

	virtual void OnLocalStreamStats(const std::string& stream_id, const agora::rte::LocalStreamStats& stats) override {}

	virtual void OnRemoteStreamStats(const std::string& stream_id, const agora::rte::RemoteStreamStats& stats) override {}
private:
	HWND m_hMsgHanlder;
};

// media player state
enum MEDIAPLAYERSTATE
{
	mediaPLAYER_READY,
	mediaPLAYER_OPEN,
	mediaPLAYER_PLAYING,
	mediaPLAYER_PAUSE,
	mediaPLAYER_STOP,
};


class CAgoraMediaPlayer : public CDialogEx
{
	DECLARE_DYNAMIC(CAgoraMediaPlayer)

public:
	CAgoraMediaPlayer(CWnd* pParent = nullptr);   
	virtual ~CAgoraMediaPlayer();

	enum { IDD = IDD_DIALOG_MEDIA_PLAYER };

public:
	//Initialize the Ctrl Text.
	void InitCtrlText();
	//Initialize media player.
	void InitMediaPlayerKit();
	//Uninitialized media player .
	void UnInitMediaPlayerKit();
	//Initialize the Agora SDK
	bool InitAgora();
	//UnInitialize the Agora SDK
	void UnInitAgora();
	//resume window status
	void ResumeStatus();
	
private:
	bool m_joinChannel = false;
	bool m_initialize = false;
	bool m_attach = false;
	bool m_publishVideo = false;
	bool m_publishAudio = false;
	bool m_publishMeidaplayer = false;

	CAGVideoWnd m_localVideoWnd;
	CAgoraMediaPlayerHandler m_eventHandler;
	
	//IMediaPlayer *m_mediaPlayer = nullptr;
	MEDIAPLAYERSTATE m_mediaPlayerState = mediaPLAYER_READY;
	std::shared_ptr<agora::rte::IAgoraRteMediaFactory> media_control_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteMediaPlayer> media_player_ = nullptr;
	std::shared_ptr<MediaPlayerObserver> player_observer_ = nullptr;
	std::shared_ptr<agora::rte::IAgoraRteScene> m_scene = nullptr;
	std::shared_ptr<CAgoraMediaPlayerHandler> m_sceneEventHandler = nullptr;
	std::vector<std::string> connectionStates;
	std::string local_user_id_ = "player_user_id";


protected:
	virtual void DoDataExchange(CDataExchange* pDX);   
	LRESULT OnmediaPlayerStateChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnmediaPlayerPositionChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDJoinChannelSuccess(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDLeaveChannel(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserJoined(WPARAM wParam, LPARAM lParam);
	LRESULT OnEIDUserOffline(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEIDConnectionStateChanged(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_staVideoArea;
	CListBox m_lstInfo;
	CStatic m_staDetail;
	CStatic m_staChannel;
	CEdit m_edtChannel;
	CButton m_btnJoinChannel;
	CStatic m_staVideoSource;
	CEdit m_edtVideoSource;
	CButton m_btnOpen;
	CButton m_btnStop;
	CButton m_btnPlay;
	
	CButton m_btnPublishVideo;
	
	CSliderCtrl m_sldVideo;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonJoinchannel();
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonPlay();
	
	afx_msg void OnBnClickedButtonPublishVideo();
	
	afx_msg void OnSelchangeListInfoBroadcasting();
	afx_msg void OnDestroy();
	afx_msg void OnReleasedcaptureSliderVideo(NMHDR *pNMHDR, LRESULT *pResult);
};
