package io.agora.ng_api.util

import android.content.res.Configuration
import android.content.res.Resources
import android.util.Log

fun String.agoraLog(){
    Log.d("lq",this)
}

fun isNightMode(): Boolean {
    return isNightMode(Resources.getSystem().configuration)
}
fun isNightMode(configuration: Configuration): Boolean {
    return configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK == Configuration.UI_MODE_NIGHT_YES
}