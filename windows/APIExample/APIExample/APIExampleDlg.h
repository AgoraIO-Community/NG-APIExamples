
// APIExampleDlg.h : header file
//

#pragma once
#include "Basic/LiveBroadcasting/CLiveBroadcastingDlg.h"
#include "Advanced/MultiVideoSource/CAgoraMutilVideoSourceDlg.h"
#include "Advanced/MediaPlayer/CAgoraMediaPlayer.h"
#include "Advanced/CustomAVCapture/CAgoraAVCaptureDlg.h"
/*
#include "Advanced/MultiChannel/CAgoraMultiChannelDlg.h"
#include "Advanced/MultiCamera/CMultiCameraDlg.h"
#include "Advanced/OriginalVideo/CAgoraOriginalVideoDlg.h"
#include "Advanced/OriginalAudio/CAgoraOriginalAudioDlg.h"
#include "Advanced/RTMPinject/AgoraRtmpInjectionDlg.h"
#include "Advanced/RTMPStream/AgoraRtmpStreaming.h"
#include "Advanced/VideoMetadata/CAgoraMetaDataDlg.h"
#include "Advanced/ScreenShare/AgoraScreenCapture.h"

#include "Advanced/AudioProfile/CAgoraAudioProfile.h"
#include "Advanced/AudioMixing/CAgoraAudioMixingDlg.h"
#include "Advanced/CustomEncrypt/CAgoraCustomEncryptDlg.h"
#include "Advanced/PerCallTest/CAgoraPerCallTestDlg.h"
#include "Advanced/AudioVolume/CAgoraAudioVolumeDlg.h"
#include "Advanced/RegionConn/CAgoraRegionConnDlg.h"
#include "Advanced/MediaEncrypt/CAgoraMediaEncryptDlg.h"
#include "Advanced/AudioEffect/CAgoraEffectDlg.h"
#include "Advanced/LocalVideoTranscoding/CLocalVideoTranscodingDlg.h"*/
#include <mutex>
#include <vector>
#include <map>
const int MAIN_AREA_BOTTOM = 15;

// CAPIExampleDlg dialog

class CAPIExampleDlg : public CDialogEx
{
// Construction
public:
	CAPIExampleDlg(CWnd* pParent = nullptr);	// standard constructor
	~CAPIExampleDlg();
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_APIEXAMPLE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonFaq();
    afx_msg void OnBnClickedButtonRegister();
    afx_msg void OnBnClickedButtonDemo();
    afx_msg LRESULT OnEIDJoinLeaveChannel(WPARAM wParam, LPARAM lParam);
   
private:
    void InitSceneDialog();
    void InitSceneList();
    void InitCtrlText();
    HTREEITEM GetHitItem(NMHDR *pNMHDR);
    
    void ReleaseScene(CTreeCtrl& treeScene, HTREEITEM& hSelectItem);
    void CreateScene(CTreeCtrl& treeScene, CString selectedText);
    std::unique_ptr<CLiveBroadcastingDlg>           m_pLiveBroadcasting = nullptr;
	std::unique_ptr < CAgoraMutilVideoSourceDlg>	m_pMultiVideoSourceDlg = nullptr;
	std::unique_ptr < CAgoraMediaPlayer	>		    m_pmediaPlayerDlg = nullptr;
	std::unique_ptr <CAgoraAVCaptureDlg>            m_pCaputreVideoDlg = nullptr;
	
  /*  CAgoraRtmpInjectionDlg      *m_pRtmpInjectDlg    = nullptr;
    CAgoraRtmpStreamingDlg      *m_pRtmpStreamingDlg = nullptr;
    CAgoraMetaDataDlg           *m_pVideoSEIDlg      = nullptr;
    CAgoraScreenCapture         *m_pScreenCap        = nullptr;
	CAgoraAudioProfile			*m_pAudioProfileDlg  = nullptr;
	CAgoraAudioMixingDlg		*m_pAudioMixingDlg   = nullptr;
	CAgoraOriginalVideoDlg		*m_pOriginalVideoDlg = nullptr;
	CAgoraOriginalAudioDlg		*m_pOriginalAudioDlg = nullptr;
	CAgoraCustomEncryptDlg		*m_pCustomEncryptDlg = nullptr;
	CAgoraMultiChannelDlg		*m_pMultiChannelDlg   = nullptr;
	
	CAgoraPerCallTestDlg		*m_pPerCallTestDlg		= nullptr;
	
	CAgoraRegionConnDlg			*m_pRegionConnDlg		= nullptr;
	CAgoraMediaEncryptDlg		*m_pMediaEncrptDlg		= nullptr;
	CAgoraEffectDlg				*m_pEffectDlg			= nullptr;
	
	CAgoraAudioVolumeDlg		*m_pAudioVolumeDlg = nullptr;
	CMultiCameraDlg             *m_pMultiCameraDlg = nullptr;
	CLocalVideoTranscodingDlg   *m_pLocalVideoTranscodingDlg = nullptr;*/
    CString m_preSelectedItemText = _T("");
    std::vector<CString> m_vecBasic, m_vecAdvanced;
	std::mutex m_mutex;
	
    BOOL m_bJoinChannel = FALSE;
public:
    CStatic m_staMainArea;
    CTreeCtrl m_lstBasicScene;
    CTreeCtrl m_lstAdvanced;
  
    afx_msg void OnClickListBasic(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSelchangedListAdvanced(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnClickListAdvanced(NMHDR *pNMHDR, LRESULT *pResult);
   
    afx_msg void OnSelchangedListBasic(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSelchangingListBasic(NMHDR *pNMHDR, LRESULT *pResult);
    
    afx_msg void OnSelchangingListAdvanced(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedButtonDocumentWebsite();
	CStatic m_stalstInfo;
    CStatic m_staAdvancedScene;
    CStatic m_grpBasicScene;
    CButton m_btnDemoWebSite;
    CButton m_btnRegister;
    CButton m_btnFAQ;
    CButton m_btnDocWebsite;
    CStatic m_grpDoc;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClose();
	CButton m_chkCompatible;
	afx_msg void OnClickedCheckCompatible();
};

