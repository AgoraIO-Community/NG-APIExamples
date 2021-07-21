// ======================= TAgoraRteFunctionTest.cpp ======================= 
//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#include "AgoraRteSdkImpl.hpp" 
#include "TestUitls.h"
#include "gtest/gtest.h"


using namespace agora::rte;

TEST(AgoraRteFunctionTest, ParserRtcUserIdV1) {
  // We can parse the stream info and user info
  std::string rtc_stream_id = R"({"v":"1","u":"user1","s":"test"})";
  RteRtcStreamInfo stream_info;
  EXPECT_TRUE(AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, stream_info));
  EXPECT_EQ(stream_info.is_major_stream, false);
  EXPECT_EQ(stream_info.stream_id, "test");
  EXPECT_EQ(stream_info.user_id, "user1");

  std::string rtc_user_id = R"({"v":"1","u":"user1","s":""})";
  RteRtcStreamInfo user_info;
  EXPECT_TRUE(AgoraRteUtils::ExtractRtcStreamInfo(rtc_user_id, user_info));
  EXPECT_EQ(user_info.is_major_stream, true);
  EXPECT_EQ(user_info.stream_id, "");
  EXPECT_EQ(user_info.user_id, "user1");

  std::string rtc_user_special_id =
      R"({"v":"1","u":"{user1{my_test_*_2}}","s":""})";
  RteRtcStreamInfo user_special_info;
  EXPECT_TRUE(AgoraRteUtils::ExtractRtcStreamInfo(rtc_user_special_id,
                                                  user_special_info));
  EXPECT_EQ(user_special_info.is_major_stream, true);
  EXPECT_EQ(user_special_info.stream_id, "");
  EXPECT_EQ(user_special_info.user_id, "{user1{my_test_*_2}}");
}

TEST(AgoraRteFunctionTest, ParserRtcUserIdV1Extension) {
  // We can parse the stream info even there is extensions we don't support yet
  //
  std::string rtc_stream_id =
      R"({"v":"1","u":"user1","s":"test","l":"my_test_layer11"})";
  RteRtcStreamInfo stream_info;
  EXPECT_TRUE(AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, stream_info));
  EXPECT_EQ(stream_info.is_major_stream, false);
  EXPECT_EQ(stream_info.stream_id, "test");
  EXPECT_EQ(stream_info.user_id, "user1");
}

TEST(AgoraRteFunctionTest, ParserRtcUserIdNegative) {
  // We should fail to parser stream info if someone add whitespace between json
  // elements.
  //
  std::string rtc_stream_id =
      R"({"v":    "1",    "u":"user1","s":"test","l","my_test_layer"})";
  RteRtcStreamInfo stream_info;
  EXPECT_FALSE(AgoraRteUtils::ExtractRtcStreamInfo(rtc_stream_id, stream_info));
}

