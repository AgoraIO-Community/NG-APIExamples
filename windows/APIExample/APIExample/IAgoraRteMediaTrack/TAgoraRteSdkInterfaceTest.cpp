// ======================= TAgoraRteSdkInterfaceTest.cpp ======================= 
//
// Created by minbo on 2021/7/8.
//

#include "AgoraRteSdkImpl.hpp" 
#include "TestUitls.h"
#include "gtest/gtest.h"

using namespace agora::rte;

TEST(AgoraRteSDKTest, MediaFactoryBasic) {
  auto media_factory = AgoraRteSDK::GetRteMediaFactory();
  EXPECT_NE(media_factory, nullptr);

  auto rtc_service = AgoraRteSDK::GetRtcService();
  EXPECT_NE(rtc_service, nullptr);
}

TEST(AgoraRteSDKTest, AudioDeviceManagerBasic) {
  agora::rte::SdkProfile profile;
  AgoraRteSDK::Init(profile);

#if RTE_WIN || RTE_MAC
  auto adm = AgoraRteSDK::GetAudioDeviceManager();
  EXPECT_NE(adm, nullptr);

  auto play_back = adm->EnumeratePlaybackDevices();
  auto record = adm->EnumerateRecordingDevices();

  play_back->GetCount();
  record->GetCount();
#endif

  AgoraRteSDK::Deinit();
}

TEST(AgoraRteSDKTest, VideoDeviceManagerBasic) {
  agora::rte::SdkProfile profile;
  AgoraRteSDK::Init(profile);

#if RTE_WIN || RTE_MAC
  auto vdm = AgoraRteSDK::GetVideoDeviceManager();
  EXPECT_NE(vdm, nullptr);

  auto video = vdm->EnumerateVideoDevices();
#endif

  AgoraRteSDK::Deinit();
}

TEST(AgoraRteSDKTest, CustomerServiceCreator) {
  agora::rte::AgoraServiceCreatorFunctionPtr fun = createAgoraService;
  AgoraRteSDK::SetAgoraServiceCreator(fun);
  agora::rte::SdkProfile profile;
  profile.appid = "test_appid";

  AgoraRteSDK::Init(profile);

  SceneConfig config;
  auto scene = AgoraRteSDK::CreateRteScene("test_scene", config);
  EXPECT_TRUE(scene != nullptr);
  AgoraRteSDK::Deinit();
}

