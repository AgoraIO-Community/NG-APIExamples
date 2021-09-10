package io.agora.ng_api.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.NavHostController
import io.agora.ng_api.NGApiScreen

@Composable
fun MainBody(navHostController: NavHostController) {
    // type 未设置的默认不展示
    val screens = NGApiScreen.values().asList().filter { it.pageDesc.type > 0 }
    LazyColumn {
        items(screens) { screen ->
            Text(text = stringResource(id = screen.pageDesc.title),
                style = TextStyle(fontSize = 18.sp),
                modifier = Modifier
                    .clickable { navHostController.navigate("${NGApiScreen.DescScreen.name}/${screen.name}") }
                    .fillMaxWidth()
                    .padding(18.dp))
        }
    }
}