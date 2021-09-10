package io.agora.ng_api

import io.agora.ng_api.bean.PageDesc
import io.agora.ng_api.util.agoraLog

enum class NGApiScreen(val pageDesc: PageDesc) {
    // ## Common section
    MainScreen(pageDesc = PageDesc(R.string.app_name)),
    DescScreen(pageDesc = PageDesc(R.string.app_name)),
    // ## Demo section

    // ### basic section
    BasicAudioScreen(
        pageDesc = PageDesc(
            R.string.title_join_channel_audio,
            R.string.desc_join_channel_audio,
            1,
            1
        )
    ),
    BasicVideoScreen(
        pageDesc = PageDesc(
            R.string.title_join_channel_video,
            R.string.desc_join_channel_video,
            3,
            1
        )
    );

    companion object {
        /**
         * 根据 Navigation 栈内的 String 标识当前是哪个页面
         * APP初始化时为"null"，然后走"MainScreen"
         **/
        fun fromRoute(route: String?): NGApiScreen{
            if(route?.contains(DescScreen.name) == true) return DescScreen
            return when (route) {
                MainScreen.name -> MainScreen
                BasicAudioScreen.name -> BasicAudioScreen
                BasicVideoScreen.name -> BasicVideoScreen
                null -> MainScreen
                else -> throw IllegalArgumentException("Route $route is not recognized.")
            }
        }

    }
}