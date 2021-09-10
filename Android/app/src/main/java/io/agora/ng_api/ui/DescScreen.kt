package io.agora.ng_api.ui

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.dimensionResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import io.agora.ng_api.R
import io.agora.ng_api.bean.PageDesc

@Composable
fun DescBody(pageDesc: PageDesc) {
    Column {
        Box(modifier = Modifier
            .fillMaxWidth()
            .weight(1f, true), contentAlignment = Alignment.Center) {
            Text(text = stringResource(id = pageDesc.desc))
        }
        Spacer(
            modifier = Modifier
                .background(color = MaterialTheme.colors.surface)
                .fillMaxWidth()
                .requiredSize(1.dp)
        )
        Row(Modifier.padding(vertical = 12.dp)) {
            var channelName by remember { mutableStateOf("") }

            var errorState by remember{ mutableStateOf(false)}
            val offsetX: Dp by animateDpAsState(targetValue = if(errorState)0.dp else 1.dp,
                animationSpec = keyframes {
                    durationMillis = 300
                    50.dp.at(80)
                    (-50).dp.at(240)
                    0.dp.at(300)
                }
            )

            BasicTextField(
                value = channelName, onValueChange = { channelName = it },
                maxLines = 1,
                modifier = Modifier
                    .offset(x = offsetX)
                    .weight(1f, true)
                    .padding(horizontal = 12.dp)
            ) {
                Box(
                    modifier = Modifier
                        .background(
                            MaterialTheme.colors.surface,
                            shape = RoundedCornerShape(8.dp)
                        )
                        .padding(10.dp)
                ) {
                    it()
                }
            }

            Button(
                onClick = {
                    if (channelName.isEmpty()) {
                        errorState = true
                    }
                },
                modifier = Modifier.padding(end = 12.dp)
            ) { Text(text = stringResource(id = R.string.btn_join)) }

        }
    }
}