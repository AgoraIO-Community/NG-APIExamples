package io.agora.ng_api.bean

import androidx.annotation.StringRes

data class PageDesc(@StringRes val title:Int
, @StringRes val desc:Int = 0
, val permFlag:Int = 0
, val type:Int = 0){
    companion object{
        @JvmField
        final val key = "PageDescBean"
    }
}