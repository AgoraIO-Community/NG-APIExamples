package io.agora.ng_api

import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import io.agora.ng_api.NGApiScreen.*
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.*
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Scaffold
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.core.view.*
import androidx.navigation.NavController
import androidx.navigation.NavHostController
import androidx.navigation.NavType
import androidx.navigation.compose.*
import io.agora.ng_api.bean.PageDesc
import io.agora.ng_api.ui.BasicAudioBody
import io.agora.ng_api.ui.BasicVideoBody
import io.agora.ng_api.ui.DescBody
import io.agora.ng_api.ui.MainBody
import io.agora.ng_api.ui.component.NGApiAppBar
import io.agora.ng_api.ui.theme.NGApiTheme
import io.agora.ng_api.util.isNightMode
import kotlin.math.max

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Edge-To-Edge
        setupTheme()

        setContent {
            NGApiApp()
        }
    }

    private fun setupTheme(){
        WindowCompat.setDecorFitsSystemWindows(window,true)
        val isNightMode = isNightMode()
        val wic = WindowInsetsControllerCompat(window, window.decorView)
        wic.isAppearanceLightNavigationBars = !isNightMode
        wic.isAppearanceLightStatusBars = !isNightMode
        (window.decorView as ViewGroup).clipChildren = false
        ViewCompat.setOnApplyWindowInsetsListener(window.decorView) { v, insets ->
            val sInset = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(
                sInset.left,
                sInset.top,
                sInset.right,
                max(sInset.bottom, insets.getInsets(WindowInsetsCompat.Type.ime()).bottom)
            )
            WindowInsetsCompat.CONSUMED
        }
    }
}


@Composable
fun NGApiApp() {
    NGApiTheme {
        val navController = rememberNavController()
        val backstackEntry = navController.currentBackStackEntryAsState()
        val currentScreen = NGApiScreen.fromRoute(backstackEntry.value?.destination?.route)


//        navController.addOnDestinationChangedListener(NavController.OnDestinationChangedListener { controller, destination, arguments ->
//            if()
//        })

        Scaffold(
                backgroundColor = Color.Transparent,
            topBar = {
                NGApiAppBar(
                    title = currentScreen.pageDesc.title,
                    showBackIcon = currentScreen != MainScreen,
                    onClick = {navController.popBackStack()}
                )
            }
        ) { innerPadding ->
            RallyNavHost(navController, modifier = Modifier.padding(innerPadding))
        }
    }
}

@Composable
fun RallyNavHost(navController: NavHostController, modifier: Modifier = Modifier) {
    NavHost(
        navController = navController,
        startDestination = MainScreen.name,
        modifier = modifier
    ) {
        composable(MainScreen.name) {
            MainBody(navController)
        }
        composable("${DescScreen.name}/{${PageDesc.key}}"
        ,arguments = listOf(navArgument(PageDesc.key){
            type = NavType.StringType
            })) { entry->
            val param = entry.arguments?.getString(PageDesc.key)
            val screen = NGApiScreen.fromRoute(param)
            DescBody(screen.pageDesc)
        }
        composable(BasicAudioScreen.name) {
            BasicAudioBody()
        }
        composable(BasicVideoScreen.name) {
            BasicVideoBody()
        }

//        val accountsName = Accounts.name
//        composable(
//            route = "$accountsName/{name}",
//            arguments = listOf(
//                navArgument("name") {
//                    type = NavType.StringType
//                }
//            ),
//            deepLinks = listOf(
//                navDeepLink {
//                    uriPattern = "rally://$accountsName/{name}"
//                }
//            ),
//        ) { entry ->
//            val accountName = entry.arguments?.getString("name")
//            val account = UserData.getAccount(accountName)
//            SingleAccountBody(account = account)
//        }
    }
}