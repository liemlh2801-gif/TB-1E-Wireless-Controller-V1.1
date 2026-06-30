package com.example.btcontroller.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color

/** Portrait viewport width : height = 9 : 19.5 */
const val APP_ASPECT_RATIO = 9f / 19.5f

@Composable
fun AppAspectRatioFrame(
    modifier: Modifier = Modifier,
    letterboxColor: Color = Color.Black,
    content: @Composable () -> Unit,
) {
    BoxWithConstraints(
        modifier = modifier
            .fillMaxSize()
            .background(letterboxColor),
    ) {
        val frameModifier = if (maxWidth / maxHeight > APP_ASPECT_RATIO) {
            Modifier.fillMaxHeight().aspectRatio(APP_ASPECT_RATIO)
        } else {
            Modifier.fillMaxWidth().aspectRatio(APP_ASPECT_RATIO)
        }

        Box(
            modifier = frameModifier.align(Alignment.Center),
        ) {
            content()
        }
    }
}
