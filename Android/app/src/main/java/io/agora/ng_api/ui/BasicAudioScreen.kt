package io.agora.ng_api.ui

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material.Button
import androidx.compose.material.Text
import androidx.compose.material.TextField
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import io.agora.ng_api.NGApiScreen
import io.agora.ng_api.R

@Composable
fun BasicAudioBody(){
    Column {
        Box(Modifier.fillMaxSize()) {
            Text(text = stringResource(id = NGApiScreen.BasicAudioScreen.pageDesc.desc))
        }
        Row {
            var channelName = ""

            TextField(value = channelName, onValueChange = { t ->
                channelName = t
            }, placeholder = { Text(text = stringResource(id = R.string.channel_name)) })

            Button(onClick = {  }) { Text(text = stringResource(id = R.string.btn_join)) }

        }
    }
}