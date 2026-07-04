package com.example.btcontroller.ui

import android.bluetooth.BluetoothDevice
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Image
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.waitForUpOrCancellation
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.ui.draw.clip
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.btcontroller.R
import com.example.btcontroller.bluetooth.ConnectionState
import com.example.btcontroller.bluetooth.DeviceMode

private val GoldBar = Color(0xFFD4AF37)
private val PanelButtonFill = Color(0xFFE0E0E0)
private val ConnectButtonFill = Color(0xFFB3E5FC)
private val BorderBlack = Color.Black

/** Overlay positions tuned for asset1.jpg machine illustration */
private object PanelLayout {
    const val MACHINE_WIDTH = 0.58f
    const val CONNECT_TOP = 0.03f
    const val CONNECT_WIDTH = 0.50f
    const val CONNECT_END = 0.03f
    const val CONTROL_WIDTH = 0.205f
    const val CONTROL_END = 0.03f
    const val BTN_UP_Y = 0.255f
    const val BTN_STOP_Y = 0.435f
    const val BTN_DOWN_Y = 0.615f
    const val PANEL_BUTTON_HEIGHT = 50f
    const val ASSET1_WIDTH = 590f
    const val ASSET1_HEIGHT = 2425f
    /** Circle centers on asset1.jpg (fraction of image width/height) */
    const val CIRCLE_X_IN_IMAGE = 0.813f
    const val CIRCLE_UP_Y_IN_IMAGE = 0.656f
    const val CIRCLE_STOP_Y_IN_IMAGE = 0.695f
    const val CIRCLE_DOWN_Y_IN_IMAGE = 0.733f
    const val LINE_JUNCTION_FRACTION = 0.35f
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    pairedDevices: List<BluetoothDevice>,
    selectedDevice: BluetoothDevice?,
    connectionState: ConnectionState,
    lastError: String?,
    lastSent: String?,
    lastReceived: String?,
    deviceMode: DeviceMode?,
    topLimitActive: Boolean,
    botLimitActive: Boolean,
    onHoldActive: Boolean,
    onDeviceSelected: (BluetoothDevice) -> Unit,
    onConnectClick: () -> Unit,
    onDisconnectClick: () -> Unit,
    onRefreshClick: () -> Unit,
    onUpPress: () -> Unit,
    onUpRelease: () -> Unit,
    onStopPress: () -> Unit,
    onStopRelease: () -> Unit,
    onDownPress: () -> Unit,
    onDownRelease: () -> Unit,
    deviceLabel: (BluetoothDevice) -> String,
    appVersion: String,
) {
    val isConnected = connectionState == ConnectionState.Connected
    val isConnecting = connectionState == ConnectionState.Connecting
    var menuExpanded by remember { mutableStateOf(false) }

    AppAspectRatioFrame {
        MainScreenContent(
            pairedDevices = pairedDevices,
            selectedDevice = selectedDevice,
            connectionState = connectionState,
            lastError = lastError,
            lastSent = lastSent,
            lastReceived = lastReceived,
            deviceMode = deviceMode,
            topLimitActive = topLimitActive,
            botLimitActive = botLimitActive,
            onHoldActive = onHoldActive,
            onDeviceSelected = onDeviceSelected,
            onConnectClick = onConnectClick,
            onDisconnectClick = onDisconnectClick,
            onRefreshClick = onRefreshClick,
            onUpPress = onUpPress,
            onUpRelease = onUpRelease,
            onStopPress = onStopPress,
            onStopRelease = onStopRelease,
            onDownPress = onDownPress,
            onDownRelease = onDownRelease,
            deviceLabel = deviceLabel,
            appVersion = appVersion,
            isConnected = isConnected,
            isConnecting = isConnecting,
            menuExpanded = menuExpanded,
            onMenuExpandedChange = { menuExpanded = it },
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun MainScreenContent(
    pairedDevices: List<BluetoothDevice>,
    selectedDevice: BluetoothDevice?,
    connectionState: ConnectionState,
    lastError: String?,
    lastSent: String?,
    lastReceived: String?,
    deviceMode: DeviceMode?,
    topLimitActive: Boolean,
    botLimitActive: Boolean,
    onHoldActive: Boolean,
    onDeviceSelected: (BluetoothDevice) -> Unit,
    onConnectClick: () -> Unit,
    onDisconnectClick: () -> Unit,
    onRefreshClick: () -> Unit,
    onUpPress: () -> Unit,
    onUpRelease: () -> Unit,
    onStopPress: () -> Unit,
    onStopRelease: () -> Unit,
    onDownPress: () -> Unit,
    onDownRelease: () -> Unit,
    deviceLabel: (BluetoothDevice) -> String,
    appVersion: String,
    isConnected: Boolean,
    isConnecting: Boolean,
    menuExpanded: Boolean,
    onMenuExpandedChange: (Boolean) -> Unit,
) {
    val isAuto = deviceMode == DeviceMode.Auto
    val upEnabled = isConnected && !topLimitActive
    val downEnabled = isConnected && !botLimitActive
    val stopEnabled = isConnected

    Scaffold(
        modifier = Modifier.fillMaxSize(),
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text(
                            text = stringResource(R.string.app_title),
                            color = Color.White,
                            fontWeight = FontWeight.Medium,
                        )
                        Text(
                            text = "v$appVersion",
                            color = Color.White.copy(alpha = 0.85f),
                            fontSize = 11.sp,
                        )
                    }
                },
                actions = {
                    IconButton(onClick = { onMenuExpandedChange(true) }) {
                        Icon(
                            Icons.Default.MoreVert,
                            contentDescription = stringResource(R.string.menu),
                            tint = Color.White,
                        )
                    }
                    DeviceMenu(
                        expanded = menuExpanded,
                        onDismiss = { onMenuExpandedChange(false) },
                        pairedDevices = pairedDevices,
                        selectedDevice = selectedDevice,
                        lastSent = lastSent,
                        lastReceived = lastReceived,
                        lastError = lastError,
                        deviceLabel = deviceLabel,
                        onDeviceSelected = onDeviceSelected,
                        onRefreshClick = onRefreshClick,
                    )
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = GoldBar,
                    titleContentColor = Color.White,
                    actionIconContentColor = Color.White,
                ),
            )
        },
    ) { padding ->
        BoxWithConstraints(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .background(Color.White),
        ) {
            val connectWidth = maxWidth * PanelLayout.CONNECT_WIDTH
            val connectEnd = maxWidth * PanelLayout.CONNECT_END
            val controlWidth = (maxWidth * PanelLayout.CONTROL_WIDTH).coerceIn(120.dp, 210.dp)
            val controlEnd = maxWidth * PanelLayout.CONTROL_END
            val machineWidth = maxWidth * PanelLayout.MACHINE_WIDTH

            Box(
                modifier = Modifier
                    .align(Alignment.CenterStart)
                    .width(machineWidth)
                    .fillMaxHeight(),
            ) {
                Image(
                    painter = painterResource(R.drawable.asset1),
                    contentDescription = stringResource(R.string.machine_panel),
                    modifier = Modifier.fillMaxSize(),
                    contentScale = ContentScale.Fit,
                    alignment = Alignment.Center,
                )
            }

            LeaderLinesOverlay(
                machineWidth = machineWidth,
                controlEnd = controlEnd,
                controlWidth = controlWidth,
                modifier = Modifier.fillMaxSize(),
            )

            Column(
                modifier = Modifier
                    .align(Alignment.TopEnd)
                    .padding(
                        top = maxHeight * PanelLayout.CONNECT_TOP,
                        end = connectEnd,
                    )
                    .width(connectWidth),
                horizontalAlignment = Alignment.End,
            ) {
                ConnectButton(
                    isConnected = isConnected,
                    isConnecting = isConnecting,
                    enabled = selectedDevice != null || isConnected,
                    onClick = if (isConnected) onDisconnectClick else onConnectClick,
                    modifier = Modifier.fillMaxWidth(),
                )

                Spacer(modifier = Modifier.height(8.dp))

                StatusRow(connectionState = connectionState, deviceMode = deviceMode)

                } else if (isAuto) {
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = stringResource(R.string.mode_auto_latch),
                        color = Color(0xFF1565C0),
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Medium,
                        textAlign = TextAlign.End,
                        modifier = Modifier.fillMaxWidth(),
                    )
                }

                lastError?.let { error ->
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = error,
                        color = Color(0xFFC62828),
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Medium,
                        textAlign = TextAlign.End,
                        modifier = Modifier.fillMaxWidth(),
                    )
                }
            }

            val controlButtonModifier = Modifier
                .align(Alignment.TopEnd)
                .padding(end = controlEnd)
                .width(controlWidth)

            PanelButton(
                text = stringResource(R.string.btn_up),
                enabled = upEnabled,
                onPress = onUpPress,
                onRelease = if (isAuto) ({}) else onUpRelease,
                modifier = controlButtonModifier.offset(y = maxHeight * PanelLayout.BTN_UP_Y),
            )
            PanelButton(
                text = stringResource(R.string.btn_stop),
                enabled = stopEnabled,
                onPress = onStopPress,
                onRelease = onStopRelease,
                modifier = controlButtonModifier.offset(y = maxHeight * PanelLayout.BTN_STOP_Y),
            )
            PanelButton(
                text = stringResource(R.string.btn_down),
                enabled = downEnabled,
                onPress = onDownPress,
                onRelease = if (isAuto) ({}) else onDownRelease,
                modifier = controlButtonModifier.offset(y = maxHeight * PanelLayout.BTN_DOWN_Y),
            )
        }
    }
}

@Composable
private fun LeaderLinesOverlay(
    machineWidth: Dp,
    controlEnd: Dp,
    controlWidth: Dp,
    modifier: Modifier = Modifier,
) {
    val density = LocalDensity.current
    val strokeWidth = with(density) { 0.75.dp.toPx() }
    val buttonHeight = with(density) { PanelLayout.PANEL_BUTTON_HEIGHT.dp.toPx() }

    Canvas(modifier = modifier) {
        val height = size.height
        val machineWidthPx = machineWidth.toPx()
        val buttonLeft = size.width - controlEnd.toPx() - controlWidth.toPx()

        fun buttonCenterY(anchorFraction: Float): Float =
            height * anchorFraction + buttonHeight / 2f

        fun mapImagePoint(xFraction: Float, yFraction: Float): Offset {
            val scale = minOf(
                machineWidthPx / PanelLayout.ASSET1_WIDTH,
                height / PanelLayout.ASSET1_HEIGHT,
            )
            val displayedWidth = PanelLayout.ASSET1_WIDTH * scale
            val displayedHeight = PanelLayout.ASSET1_HEIGHT * scale
            val offsetX = (machineWidthPx - displayedWidth) / 2f
            val offsetY = (height - displayedHeight) / 2f
            return Offset(
                x = offsetX + xFraction * displayedWidth,
                y = offsetY + yFraction * displayedHeight,
            )
        }

        val upCircle = mapImagePoint(
            PanelLayout.CIRCLE_X_IN_IMAGE,
            PanelLayout.CIRCLE_UP_Y_IN_IMAGE,
        )
        val stopCircle = mapImagePoint(
            PanelLayout.CIRCLE_X_IN_IMAGE,
            PanelLayout.CIRCLE_STOP_Y_IN_IMAGE,
        )
        val downCircle = mapImagePoint(
            PanelLayout.CIRCLE_X_IN_IMAGE,
            PanelLayout.CIRCLE_DOWN_Y_IN_IMAGE,
        )
        val upButtonY = buttonCenterY(PanelLayout.BTN_UP_Y)
        val stopButtonY = buttonCenterY(PanelLayout.BTN_STOP_Y)
        val downButtonY = buttonCenterY(PanelLayout.BTN_DOWN_Y)

        fun junctionX(circleX: Float): Float =
            circleX + (buttonLeft - circleX) * PanelLayout.LINE_JUNCTION_FRACTION

        val upJunctionX = junctionX(upCircle.x)
        val stopJunctionX = junctionX(stopCircle.x)
        val downJunctionX = junctionX(downCircle.x)

        // LÊN: green circle → diagonal up → horizontal into button
        drawLine(
            color = BorderBlack,
            start = upCircle,
            end = Offset(upJunctionX, upButtonY),
            strokeWidth = strokeWidth,
        )
        drawLine(
            color = BorderBlack,
            start = Offset(upJunctionX, upButtonY),
            end = Offset(buttonLeft, upButtonY),
            strokeWidth = strokeWidth,
        )

        // DỪNG: red circle → diagonal up → horizontal into button
        drawLine(
            color = BorderBlack,
            start = stopCircle,
            end = Offset(stopJunctionX, stopButtonY),
            strokeWidth = strokeWidth,
        )
        drawLine(
            color = BorderBlack,
            start = Offset(stopJunctionX, stopButtonY),
            end = Offset(buttonLeft, stopButtonY),
            strokeWidth = strokeWidth,
        )

        // XUỐNG: green circle → diagonal down → horizontal into button
        drawLine(
            color = BorderBlack,
            start = downCircle,
            end = Offset(downJunctionX, downButtonY),
            strokeWidth = strokeWidth,
        )
        drawLine(
            color = BorderBlack,
            start = Offset(downJunctionX, downButtonY),
            end = Offset(buttonLeft, downButtonY),
            strokeWidth = strokeWidth,
        )
    }
}

@Composable
private fun ConnectButton(
    isConnected: Boolean,
    isConnecting: Boolean,
    enabled: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val label = when {
        isConnecting -> stringResource(R.string.connecting)
        isConnected -> stringResource(R.string.disconnect)
        else -> stringResource(R.string.connect)
    }

    OutlinedButton(
        onClick = onClick,
        enabled = enabled && !isConnecting,
        modifier = modifier.height(48.dp),
        shape = RoundedCornerShape(10.dp),
        border = BorderStroke(1.5.dp, BorderBlack),
        contentPadding = PaddingValues(horizontal = 10.dp, vertical = 8.dp),
        colors = androidx.compose.material3.ButtonDefaults.outlinedButtonColors(
            containerColor = ConnectButtonFill,
            contentColor = BorderBlack,
            disabledContainerColor = ConnectButtonFill.copy(alpha = 0.5f),
            disabledContentColor = BorderBlack.copy(alpha = 0.5f),
        ),
    ) {
        Text(
            text = label,
            fontWeight = FontWeight.Bold,
            fontSize = 10.sp,
            maxLines = 1,
            softWrap = false,
            overflow = TextOverflow.Clip,
            textAlign = TextAlign.Center,
            modifier = Modifier.fillMaxWidth(),
        )
    }
}

@Composable
private fun PanelButton(
    text: String,
    enabled: Boolean,
    onPress: () -> Unit,
    onRelease: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val containerColor = if (enabled) PanelButtonFill else PanelButtonFill.copy(alpha = 0.45f)
    val contentColor = if (enabled) BorderBlack else BorderBlack.copy(alpha = 0.45f)

    Box(
        modifier = modifier
            .height(50.dp)
            .clip(RoundedCornerShape(10.dp))
            .then(
                if (enabled) {
                    Modifier.pointerInput(Unit) {
                        awaitEachGesture {
                            awaitFirstDown(requireUnconsumed = false)
                            onPress()
                            waitForUpOrCancellation()
                            onRelease()
                        }
                    }
                } else {
                    Modifier
                },
            )
            .border(1.5.dp, contentColor, RoundedCornerShape(10.dp))
            .background(containerColor),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = text,
            fontWeight = FontWeight.Bold,
            fontSize = 17.sp,
            color = contentColor,
        )
    }
}

@Composable
private fun StatusRow(
    connectionState: ConnectionState,
    deviceMode: DeviceMode?,
    topLimitActive: Boolean,
    botLimitActive: Boolean,
    onHoldActive: Boolean,
    modifier: Modifier = Modifier,
) {
    Row(
        modifier = modifier,
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.End,
    ) {
        StatusDot(connectionState)
        Spacer(modifier = Modifier.width(8.dp))
        Column(horizontalAlignment = Alignment.End) {
            Text(
                text = statusLabel(connectionState),
                fontWeight = FontWeight.Bold,
                fontSize = 14.sp,
                color = BorderBlack,
            )
            deviceMode?.let { mode ->
                Text(
                    text = when (mode) {
                        DeviceMode.Manual -> stringResource(R.string.mode_manual)
                        DeviceMode.Auto -> stringResource(R.string.mode_auto)
                    },
                    fontSize = 12.sp,
                    fontWeight = FontWeight.Medium,
                    color = if (mode == DeviceMode.Auto) Color(0xFF1565C0) else Color(0xFF2E7D32),
                )
            }
        }
    }
}

@Composable
private fun StatusDot(state: ConnectionState) {
    val color = when (state) {
        ConnectionState.Connected -> Color(0xFF2E7D32)
        ConnectionState.Connecting -> Color(0xFFF9A825)
        ConnectionState.Error -> Color(0xFFC62828)
        ConnectionState.Disconnected -> Color(0xFFC62828)
    }

    Box(
        modifier = Modifier
            .size(12.dp)
            .background(color, RoundedCornerShape(50)),
    )
}

@Composable
private fun DeviceMenu(
    expanded: Boolean,
    onDismiss: () -> Unit,
    pairedDevices: List<BluetoothDevice>,
    selectedDevice: BluetoothDevice?,
    lastSent: String?,
    lastReceived: String?,
    deviceMode: DeviceMode?,
    topLimitActive: Boolean,
    botLimitActive: Boolean,
    onHoldActive: Boolean,
    lastError: String?,
    deviceLabel: (BluetoothDevice) -> String,
    onDeviceSelected: (BluetoothDevice) -> Unit,
    onRefreshClick: () -> Unit,
) {
    DropdownMenu(
        expanded = expanded,
        onDismissRequest = onDismiss,
    ) {
        DropdownMenuItem(
            text = { Text(stringResource(R.string.refresh_devices)) },
            onClick = {
                onRefreshClick()
                onDismiss()
            },
        )

        HorizontalDivider()

        if (pairedDevices.isEmpty()) {
            DropdownMenuItem(
                text = { Text(stringResource(R.string.no_paired_devices)) },
                onClick = { onDismiss() },
                enabled = false,
            )
        } else {
            pairedDevices.forEach { device ->
                val selected = device.address == selectedDevice?.address
                DropdownMenuItem(
                    text = {
                        Text(
                            text = deviceLabel(device),
                            fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal,
                        )
                    },
                    onClick = {
                        onDeviceSelected(device)
                        onDismiss()
                    },
                )
            }
        }

        if (lastSent != null || lastReceived != null || lastError != null) {
            HorizontalDivider()
            lastSent?.let {
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.last_sent, it), fontSize = 12.sp) },
                    onClick = { onDismiss() },
                    enabled = false,
                )
            }
            lastReceived?.let {
                DropdownMenuItem(
                    text = { Text(stringResource(R.string.last_received, it), fontSize = 12.sp) },
                    onClick = { onDismiss() },
                    enabled = false,
                )
            }
        }
    }
}

@Composable
private fun statusLabel(state: ConnectionState): String = when (state) {
    ConnectionState.Connected -> stringResource(R.string.status_connected)
    ConnectionState.Connecting -> stringResource(R.string.status_connecting)
    ConnectionState.Error -> stringResource(R.string.status_error)
    ConnectionState.Disconnected -> stringResource(R.string.status_disconnected)
}
