package io.agora.ng_api.ui.component

import android.view.View
import androidx.annotation.StringRes
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.indication
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.Icon
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Text
import androidx.compose.material.ripple.rememberRipple
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.agora.ng_api.R

@Composable
fun NGApiAppBar(title: String, showBackIcon: Boolean, onClick: () -> Unit) {
    Box(
        Modifier
            .requiredHeight(56.dp)
            .fillMaxWidth(), contentAlignment = Alignment.CenterStart
    ) {
        if (showBackIcon)
            Icon(
                painter = painterResource(id = R.drawable.ic_arrow_left),
                contentDescription = stringResource(
                    id = R.string.btn_icon_back
                ),
                Modifier
                    .padding(12.dp)
                    .clickable(onClick = onClick, indication = rememberRipple(
                        bounded = false,
                        radius = 24.dp
                    ),
                        interactionSource = remember {
                            MutableInteractionSource()
                        })
            )
        Text(
            text = title,
            textAlign = TextAlign.Center,
            maxLines = 1,
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 48.dp), style = TextStyle(fontSize = 20.sp)
        )
    }
}

@Composable
fun NGApiAppBar(@StringRes title: Int, showBackIcon: Boolean, onClick: () -> Unit) {
    NGApiAppBar(title = stringResource(id = title), showBackIcon = showBackIcon, onClick)
}


@Preview(showSystemUi = false)
@Composable
fun PreviewNGApiAppBar() {
    NGApiAppBar(title = R.string.app_name, showBackIcon = true) {}
}