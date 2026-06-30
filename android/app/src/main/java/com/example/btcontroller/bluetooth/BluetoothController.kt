package com.example.btcontroller.bluetooth



import android.Manifest

import android.annotation.SuppressLint

import android.bluetooth.BluetoothAdapter

import android.bluetooth.BluetoothDevice

import android.bluetooth.BluetoothGatt

import android.bluetooth.BluetoothGattCallback

import android.bluetooth.BluetoothGattCharacteristic

import android.bluetooth.BluetoothGattDescriptor

import android.bluetooth.BluetoothManager

import android.bluetooth.BluetoothProfile

import android.bluetooth.BluetoothSocket

import android.bluetooth.le.ScanCallback

import android.bluetooth.le.ScanResult

import android.content.Context

import android.content.pm.PackageManager

import android.os.Build

import androidx.core.content.ContextCompat

import java.io.IOException

import java.io.InputStream

import java.io.OutputStream

import java.util.UUID

import kotlin.coroutines.resume

import kotlin.coroutines.resumeWithException

import kotlinx.coroutines.CoroutineScope

import kotlinx.coroutines.Dispatchers

import kotlinx.coroutines.Job

import kotlinx.coroutines.delay

import kotlinx.coroutines.flow.MutableStateFlow

import kotlinx.coroutines.flow.StateFlow

import kotlinx.coroutines.flow.asStateFlow

import kotlinx.coroutines.launch

import kotlinx.coroutines.suspendCancellableCoroutine

import kotlinx.coroutines.withContext



enum class ConnectionState {

    Disconnected,

    Connecting,

    Connected,

    Error,

}



class BluetoothController(private val context: Context) {



    companion object {

        private val SPP_UUID: UUID =

            UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")

        private val NUS_SERVICE_UUID: UUID =

            UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")

        private val NUS_RX_UUID: UUID =

            UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")

        private val NUS_TX_UUID: UUID =

            UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

        private val CCCD_UUID: UUID =

            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

        private const val PREFS_NAME = "bt_controller_prefs"

        private const val KEY_LAST_DEVICE = "last_device_address"

        private const val CONNECT_PREP_MS = 500L

        private const val POST_CONNECT_MS = 600L

        private const val BLE_SCAN_MS = 4500L

        private const val DEVICE_BT_NAME = "TB-1E"

    }



    private val bluetoothManager: BluetoothManager? =

        context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager

    private val adapter: BluetoothAdapter? = bluetoothManager?.adapter



    private var socket: BluetoothSocket? = null

    private var outputStream: OutputStream? = null

    private var inputStream: InputStream? = null

    private var bleGatt: BluetoothGatt? = null

    private var bleRxCharacteristic: BluetoothGattCharacteristic? = null

    private var useBleTransport = false

    private var connectJob: Job? = null

    private var readJob: Job? = null



    private val _connectionState = MutableStateFlow(ConnectionState.Disconnected)

    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()



    private val _lastError = MutableStateFlow<String?>(null)

    val lastError: StateFlow<String?> = _lastError.asStateFlow()



    private val _lastReceived = MutableStateFlow<String?>(null)

    val lastReceived: StateFlow<String?> = _lastReceived.asStateFlow()



    private val _lastSent = MutableStateFlow<String?>(null)

    val lastSent: StateFlow<String?> = _lastSent.asStateFlow()



    val isBluetoothAvailable: Boolean

        get() = adapter != null



    val isBluetoothEnabled: Boolean

        get() = adapter?.isEnabled == true



    fun hasRequiredPermissions(): Boolean {

        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {

            ContextCompat.checkSelfPermission(

                context,

                Manifest.permission.BLUETOOTH_CONNECT

            ) == PackageManager.PERMISSION_GRANTED &&

                ContextCompat.checkSelfPermission(

                    context,

                    Manifest.permission.BLUETOOTH_SCAN

                ) == PackageManager.PERMISSION_GRANTED

        } else {

            ContextCompat.checkSelfPermission(

                context,

                Manifest.permission.ACCESS_FINE_LOCATION

            ) == PackageManager.PERMISSION_GRANTED

        }

    }



    @SuppressLint("MissingPermission")

    fun getPairedDevices(): List<BluetoothDevice> {

        if (!hasRequiredPermissions() || adapter == null) return emptyList()

        return adapter.bondedDevices?.sortedBy { it.name ?: it.address } ?: emptyList()

    }



    @SuppressLint("MissingPermission")

    fun getAvailableDevices(): List<BluetoothDevice> {

        if (!hasRequiredPermissions() || adapter == null) return emptyList()



        val byAddress = linkedMapOf<String, BluetoothDevice>()

        for (device in getPairedDevices()) {

            byAddress[device.address] = device

        }

        for (device in scanBleDevices()) {

            byAddress[device.address] = device

        }

        return byAddress.values.sortedBy { it.name ?: it.address }

    }



    @SuppressLint("MissingPermission")

    private fun scanBleDevices(): List<BluetoothDevice> {

        val scanner = adapter?.bluetoothLeScanner ?: return emptyList()

        val found = linkedMapOf<String, BluetoothDevice>()

        val callback = object : ScanCallback() {

            override fun onScanResult(callbackType: Int, result: ScanResult) {

                val name = deviceName(result) ?: return

                if (name.equals(DEVICE_BT_NAME, ignoreCase = true)) {

                    found[result.device.address] = result.device

                }

            }

        }



        scanner.startScan(callback)

        try {

            Thread.sleep(BLE_SCAN_MS)

        } finally {

            scanner.stopScan(callback)

        }

        return found.values.toList()

    }



    @SuppressLint("MissingPermission")

    private fun deviceName(result: ScanResult): String? {

        return result.device.name?.takeIf { it.isNotBlank() }

            ?: result.scanRecord?.deviceName?.takeIf { it.isNotBlank() }

    }



    fun getLastDeviceAddress(): String? {

        return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

            .getString(KEY_LAST_DEVICE, null)

    }



    @SuppressLint("MissingPermission")

    fun getLastDevice(): BluetoothDevice? {

        val address = getLastDeviceAddress() ?: return null

        return adapter?.getRemoteDevice(address)

    }



    private fun saveLastDevice(address: String) {

        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

            .edit()

            .putString(KEY_LAST_DEVICE, address)

            .apply()

    }



    @SuppressLint("MissingPermission")

    fun connect(device: BluetoothDevice, scope: CoroutineScope) {

        if (!hasRequiredPermissions()) {

            _connectionState.value = ConnectionState.Error

            _lastError.value = "Bluetooth permissions not granted"

            return

        }



        disconnect(sendStop = false)



        connectJob = scope.launch {

            _connectionState.value = ConnectionState.Connecting

            _lastError.value = null



            try {

                useBleTransport = shouldUseBle(device)



                if (useBleTransport) {

                    withContext(Dispatchers.IO) {

                        adapter?.cancelDiscovery()

                        connectBle(device)

                    }

                } else {

                    val connectedSocket = withContext(Dispatchers.IO) {

                        ensureBonded(device)

                        adapter?.cancelDiscovery()

                        Thread.sleep(CONNECT_PREP_MS)

                        connectSocket(device)

                    } ?: throw IOException("Bluetooth adapter unavailable")



                    socket = connectedSocket

                    outputStream = connectedSocket.outputStream

                    inputStream = connectedSocket.inputStream

                }



                saveLastDevice(device.address)

                delay(POST_CONNECT_MS)



                if (!isTransportConnected()) {

                    throw IOException("Link closed right after connect")

                }



                _connectionState.value = ConnectionState.Connected

                if (!useBleTransport) {

                    startReading(scope)

                }

            } catch (e: IOException) {

                cleanupConnection()

                _connectionState.value = ConnectionState.Error

                _lastError.value = friendlyConnectError(e.message, useBleTransport)

            }

        }

    }



    private fun shouldUseBle(device: BluetoothDevice): Boolean {

        return when (device.type) {

            BluetoothDevice.DEVICE_TYPE_LE -> true

            BluetoothDevice.DEVICE_TYPE_CLASSIC -> false

            BluetoothDevice.DEVICE_TYPE_DUAL -> false

            else -> device.name?.equals(DEVICE_BT_NAME, ignoreCase = true) == true &&

                device.type != BluetoothDevice.DEVICE_TYPE_CLASSIC

        }

    }



    private fun isTransportConnected(): Boolean {

        return if (useBleTransport) {

            bleGatt != null && bleRxCharacteristic != null

        } else {

            socket?.isConnected == true

        }

    }



    private fun friendlyConnectError(raw: String?, ble: Boolean): String {

        if (raw == null) {

            return if (ble) {

                "Không kết nối BLE tới TB-1E. Bật ESP32, bấm refresh trong app, thử lại."

            } else {

                "Không kết nối được. Settings → Bluetooth → TB-1E → Ngắt kết nối, thử lại."

            }

        }

        if (raw.contains("read failed", ignoreCase = true) ||

            raw.contains("read ret: -1", ignoreCase = true) ||

            raw.contains("timeout", ignoreCase = true)

        ) {

            return if (ble) {

                "Không mở được kênh BLE tới TB-1E (ESP32-S3). Bấm refresh, chọn TB-1E, thử lại."

            } else {

                "Không mở được kênh Bluetooth tới TB-1E (ESP32). Settings → Ngắt kết nối TB-1E, tắt BT trên PC, thử lại."

            }

        }

        return raw

    }



    @SuppressLint("MissingPermission")

    private fun ensureBonded(device: BluetoothDevice) {

        if (device.bondState == BluetoothDevice.BOND_BONDED) {

            return

        }

        if (device.bondState == BluetoothDevice.BOND_NONE) {

            device.createBond()

        }

        val deadline = System.currentTimeMillis() + 10_000L

        while (System.currentTimeMillis() < deadline) {

            if (device.bondState == BluetoothDevice.BOND_BONDED) {

                return

            }

            if (device.bondState == BluetoothDevice.BOND_NONE) {

                break

            }

            Thread.sleep(200)

        }

        if (device.bondState != BluetoothDevice.BOND_BONDED) {

            throw IOException("Pair TB-1E in phone Settings first")

        }

    }



    @SuppressLint("MissingPermission")

    private suspend fun connectBle(device: BluetoothDevice) {

        suspendCancellableCoroutine { continuation ->

            val gatt = device.connectGatt(

                context,

                false,

                object : BluetoothGattCallback() {

                    override fun onConnectionStateChange(

                        gatt: BluetoothGatt,

                        status: Int,

                        newState: Int,

                    ) {

                        if (status != BluetoothGatt.GATT_SUCCESS) {

                            if (continuation.isActive) {

                                continuation.resumeWithException(

                                    IOException("BLE connect failed ($status)")

                                )

                            }

                            return

                        }



                        when (newState) {

                            BluetoothProfile.STATE_CONNECTED -> gatt.discoverServices()

                            BluetoothProfile.STATE_DISCONNECTED -> {

                                if (continuation.isActive) {

                                    continuation.resumeWithException(

                                        IOException("BLE disconnected")

                                    )

                                }

                            }

                        }

                    }



                    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {

                        if (status != BluetoothGatt.GATT_SUCCESS) {

                            if (continuation.isActive) {

                                continuation.resumeWithException(

                                    IOException("BLE service discovery failed")

                                )

                            }

                            return

                        }



                        val service = gatt.getService(NUS_SERVICE_UUID)

                        val rx = service?.getCharacteristic(NUS_RX_UUID)

                        val tx = service?.getCharacteristic(NUS_TX_UUID)

                        if (rx == null || tx == null) {

                            if (continuation.isActive) {

                                continuation.resumeWithException(

                                    IOException("TB-1E BLE service not found")

                                )

                            }

                            return

                        }



                        gatt.setCharacteristicNotification(tx, true)

                        val cccd = tx.getDescriptor(CCCD_UUID)

                        if (cccd != null) {

                            cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE

                            gatt.writeDescriptor(cccd)

                        }



                        bleGatt = gatt

                        bleRxCharacteristic = rx

                        if (continuation.isActive) {

                            continuation.resume(Unit)

                        }

                    }



                    override fun onCharacteristicChanged(

                        gatt: BluetoothGatt,

                        characteristic: BluetoothGattCharacteristic,

                        value: ByteArray,

                    ) {

                        val message = String(value).trim()

                        if (message.isNotEmpty()) {

                            _lastReceived.value = message

                        }

                    }



                    @Deprecated("Deprecated in API 33")

                    override fun onCharacteristicChanged(

                        gatt: BluetoothGatt,

                        characteristic: BluetoothGattCharacteristic,

                    ) {

                        val value = characteristic.value ?: return

                        val message = String(value).trim()

                        if (message.isNotEmpty()) {

                            _lastReceived.value = message

                        }

                    }

                },

                BluetoothDevice.TRANSPORT_LE,

            )



            continuation.invokeOnCancellation {

                gatt?.close()

            }

        }

    }



    private fun startReading(scope: CoroutineScope) {

        readJob = scope.launch(Dispatchers.IO) {

            val buffer = ByteArray(256)

            val stream = inputStream ?: return@launch



            while (_connectionState.value == ConnectionState.Connected) {

                try {

                    if (socket?.isConnected != true) {

                        break

                    }



                    val available = stream.available()

                    if (available <= 0) {

                        Thread.sleep(100)

                        continue

                    }



                    val bytes = stream.read(buffer, 0, minOf(buffer.size, available))

                    if (bytes > 0) {

                        val message = String(buffer, 0, bytes).trim()

                        if (message.isNotEmpty()) {

                            _lastReceived.value = message

                        }

                    } else if (bytes < 0) {

                        break

                    }

                } catch (e: IOException) {

                    if (_connectionState.value == ConnectionState.Connected) {

                        _lastError.value = friendlyConnectError(e.message, useBleTransport)

                        _connectionState.value = ConnectionState.Error

                    }

                    break

                }

            }



            if (_connectionState.value == ConnectionState.Connected) {

                cleanupConnection()

                _connectionState.value = ConnectionState.Disconnected

                if (_lastError.value == null) {

                    _lastError.value = "Connection lost"

                }

            }

        }

    }



    @SuppressLint("MissingPermission")

    fun sendCommand(command: String) {

        if (_connectionState.value != ConnectionState.Connected) return



        try {

            if (useBleTransport) {

                val characteristic = bleRxCharacteristic

                    ?: throw IOException("BLE link not ready")

                val gatt = bleGatt ?: throw IOException("BLE link not ready")

                characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT

                characteristic.value = "$command\n".toByteArray()

                if (!gatt.writeCharacteristic(characteristic)) {

                    throw IOException("BLE write failed")

                }

            } else {

                outputStream?.write("$command\n".toByteArray())

                outputStream?.flush()

            }

            _lastSent.value = command

        } catch (e: IOException) {

            _lastError.value = "Send failed: ${e.message}"

            cleanupConnection()

            _connectionState.value = ConnectionState.Error

        }

    }



    fun disconnect(sendStop: Boolean = true) {

        connectJob?.cancel()

        connectJob = null



        if (sendStop && _connectionState.value == ConnectionState.Connected) {

            try {

                sendCommand("RELEASE")

            } catch (_: IOException) {

                // Best-effort safety stop before closing the link.

            }

        }



        readJob?.cancel()

        readJob = null

        cleanupConnection()

        _connectionState.value = ConnectionState.Disconnected

    }



    @SuppressLint("MissingPermission")

    private fun connectSocket(device: BluetoothDevice): BluetoothSocket {

        val errors = mutableListOf<String>()



        device.fetchUuidsWithSdp()

        Thread.sleep(1200)



        fun tryConnect(name: String, create: () -> BluetoothSocket): BluetoothSocket? {

            var candidate: BluetoothSocket? = null

            return try {

                candidate = create()

                candidate.connect()

                candidate

            } catch (e: IOException) {

                errors.add("$name: ${e.message ?: "failed"}")

                try {

                    candidate?.close()

                } catch (_: IOException) {

                }

                null

            }

        }



        repeat(3) { attempt ->

            tryConnect("insecure channel 1 (try ${attempt + 1})") {

                val method = device.javaClass.getMethod(

                    "createInsecureRfcommSocket",

                    Int::class.javaPrimitiveType,

                )

                method.invoke(device, 1) as BluetoothSocket

            }?.let { return it }



            Thread.sleep(600)

        }



        for (channel in 2..4) {

            tryConnect("insecure channel $channel") {

                val method = device.javaClass.getMethod(

                    "createInsecureRfcommSocket",

                    Int::class.javaPrimitiveType,

                )

                method.invoke(device, channel) as BluetoothSocket

            }?.let { return it }

        }



        tryConnect("insecure SPP UUID") {

            device.createInsecureRfcommSocketToServiceRecord(SPP_UUID)

        }?.let { return it }



        tryConnect("secure channel 1") {

            val method = device.javaClass.getMethod(

                "createRfcommSocket",

                Int::class.javaPrimitiveType,

            )

            method.invoke(device, 1) as BluetoothSocket

        }?.let { return it }



        throw IOException(errors.lastOrNull() ?: "All connect methods failed")

    }



    @SuppressLint("MissingPermission")

    private fun cleanupConnection() {

        try {

            inputStream?.close()

        } catch (_: IOException) {

        }

        try {

            outputStream?.close()

        } catch (_: IOException) {

        }

        try {

            socket?.close()

        } catch (_: IOException) {

        }

        try {

            bleGatt?.close()

        } catch (_: Exception) {

        }



        inputStream = null

        outputStream = null

        socket = null

        bleGatt = null

        bleRxCharacteristic = null

        useBleTransport = false

    }

}


