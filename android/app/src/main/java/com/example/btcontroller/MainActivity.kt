package com.example.btcontroller

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.Dispatchers
import com.example.btcontroller.BuildConfig
import com.example.btcontroller.bluetooth.BluetoothController
import com.example.btcontroller.bluetooth.ConnectionState
import com.example.btcontroller.ui.MainScreen

class MainActivity : ComponentActivity() {

    private lateinit var bluetoothController: BluetoothController

    private var pairedDevices by mutableStateOf<List<BluetoothDevice>>(emptyList())
    private var selectedDevice by mutableStateOf<BluetoothDevice?>(null)
    private var connectionState by mutableStateOf(ConnectionState.Disconnected)
    private var lastError by mutableStateOf<String?>(null)
    private var lastSent by mutableStateOf<String?>(null)
    private var lastReceived by mutableStateOf<String?>(null)

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { results ->
        val granted = results.values.all { it }
        if (granted) {
            refreshDevices()
        } else {
            showToast("Bluetooth permissions are required")
        }
    }

    @SuppressLint("MissingPermission")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        bluetoothController = BluetoothController(this)

        lifecycleScope.launch {
            bluetoothController.connectionState.collect { state ->
                connectionState = state
            }
        }

        lifecycleScope.launch {
            bluetoothController.lastError.collect { error ->
                lastError = error
                if (error != null && connectionState == ConnectionState.Error) {
                    showToast(error)
                }
            }
        }

        lifecycleScope.launch {
            bluetoothController.lastSent.collect { sent ->
                lastSent = sent
            }
        }

        lifecycleScope.launch {
            bluetoothController.lastReceived.collect { received ->
                lastReceived = received
            }
        }

        requestPermissionsIfNeeded()
        refreshDevices()

        setContent {
            MaterialTheme(colorScheme = lightColorScheme()) {
                MainScreen(
                    pairedDevices = pairedDevices,
                    selectedDevice = selectedDevice,
                    connectionState = connectionState,
                    lastError = lastError,
                    lastSent = lastSent,
                    lastReceived = lastReceived,
                    onDeviceSelected = { device -> selectedDevice = device },
                    onConnectClick = { connectSelectedDevice() },
                    onDisconnectClick = { bluetoothController.disconnect() },
                    onRefreshClick = { refreshDevices() },
                    onUpPress = { bluetoothController.sendCommand("UP") },
                    onUpRelease = { bluetoothController.sendCommand("RELEASE") },
                    onStopPress = { bluetoothController.sendCommand("STOP") },
                    onStopRelease = { bluetoothController.sendCommand("RELEASE") },
                    onDownPress = { bluetoothController.sendCommand("DOWN") },
                    onDownRelease = { bluetoothController.sendCommand("RELEASE") },
                    deviceLabel = { device -> deviceLabel(device) },
                    appVersion = BuildConfig.VERSION_NAME,
                )
            }
        }
    }

    override fun onPause() {
        super.onPause()
        if (connectionState == ConnectionState.Connected) {
            bluetoothController.sendCommand("RELEASE")
        }
    }

    override fun onDestroy() {
        bluetoothController.disconnect()
        super.onDestroy()
    }

    private fun requestPermissionsIfNeeded() {
        if (bluetoothController.hasRequiredPermissions()) return

        val permissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.ACCESS_FINE_LOCATION,
            )
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }

        permissionLauncher.launch(permissions)
    }

    @SuppressLint("MissingPermission")
    private fun refreshDevices() {
        if (!bluetoothController.hasRequiredPermissions()) return

        if (!bluetoothController.isBluetoothAvailable) {
            showToast("Bluetooth is not available on this device")
            return
        }

        if (!bluetoothController.isBluetoothEnabled) {
            showToast("Enable Bluetooth in Settings")
            return
        }

        lifecycleScope.launch {
            val devices = withContext(Dispatchers.IO) {
                bluetoothController.getAvailableDevices()
            }

            pairedDevices = devices
            selectedDevice = selectedDevice
                ?: bluetoothController.getLastDevice()
                ?: devices.firstOrNull()

            if (devices.isEmpty()) {
                showToast("Power on TB-1E (ESP32), then tap refresh in the app menu")
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun connectSelectedDevice() {
        val device = selectedDevice
        if (device == null) {
            showToast("Select a paired device")
            return
        }

        if (!bluetoothController.hasRequiredPermissions()) {
            requestPermissionsIfNeeded()
            return
        }

        bluetoothController.connect(device, lifecycleScope)
    }

    @SuppressLint("MissingPermission")
    private fun deviceLabel(device: BluetoothDevice): String {
        val name = device.name?.takeIf { it.isNotBlank() } ?: "Unknown"
        return "$name (${device.address})"
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }
}
