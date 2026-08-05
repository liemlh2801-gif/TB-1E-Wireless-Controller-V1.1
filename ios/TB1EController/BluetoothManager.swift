import Combine
import CoreBluetooth
import Foundation

enum ConnectionState: Equatable {
    case disconnected
    case connecting
    case connected
    case error
}

struct DiscoveredDevice: Identifiable, Equatable {
    let id: UUID
    let name: String
    let peripheral: CBPeripheral
    let rssi: Int

    var label: String { "\(name) (\(id.uuidString.prefix(8)))" }
}

enum DeviceMode: Equatable {
    case manual
    case auto
}

final class BluetoothManager: NSObject, ObservableObject {
    static let deviceName = "TB-1E"
    static let appVersion = "1.7.10"
    static let onHoldBeepNanos: UInt64 = 2_000_000_000

    private static let serviceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    private static let rxUUID = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
    private static let txUUID = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
    private static let lastDeviceKey = "last_device_uuid"

    @Published private(set) var connectionState: ConnectionState = .disconnected
    @Published private(set) var discoveredDevices: [DiscoveredDevice] = []
    @Published var selectedDeviceId: UUID?
    @Published private(set) var lastError: String?
    @Published private(set) var lastSent: String?
    @Published private(set) var lastReceived: String?
    @Published private(set) var deviceMode: DeviceMode?
    @Published private(set) var topLimitActive = false
    @Published private(set) var botLimitActive = false
    @Published private(set) var onHoldActive = false
    @Published private(set) var isBluetoothReady = false

    private var central: CBCentralManager!
    private var connectedPeripheral: CBPeripheral?
    private var rxCharacteristic: CBCharacteristic?
    private var pendingConnectId: UUID?

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main)
        selectedDeviceId = Self.loadLastDeviceId()
    }

    var isConnected: Bool { connectionState == .connected }

    var selectedDevice: DiscoveredDevice? {
        guard let selectedDeviceId else { return nil }
        return discoveredDevices.first { $0.id == selectedDeviceId }
    }

    func refreshDevices() {
        guard central.state == .poweredOn else {
            lastError = "Bật Bluetooth trên iPhone."
            return
        }

        discoveredDevices.removeAll()
        lastError = nil

        let known = central.retrieveConnectedPeripherals(withServices: [Self.serviceUUID])
        for peripheral in known {
            appendDevice(peripheral: peripheral, rssi: 0)
        }

        central.scanForPeripherals(
            withServices: [Self.serviceUUID],
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
        )

        DispatchQueue.main.asyncAfter(deadline: .now() + 4.5) { [weak self] in
            self?.central.stopScan()
        }
    }

    func connectSelectedDevice() {
        guard central.state == .poweredOn else {
            connectionState = .error
            lastError = "Bật Bluetooth trên iPhone."
            return
        }

        guard let device = selectedDevice else {
            lastError = "Chọn thiết bị TB-1E trong menu."
            return
        }

        disconnect(sendRelease: false)
        connectionState = .connecting
        lastError = nil
        pendingConnectId = device.id
        connectedPeripheral = device.peripheral
        device.peripheral.delegate = self
        central.connect(device.peripheral, options: nil)
    }

    func disconnect(sendRelease: Bool = true) {
        if sendRelease, connectionState == .connected {
            sendCommand("RELEASE")
        }

        if let peripheral = connectedPeripheral {
            central.cancelPeripheralConnection(peripheral)
        }

        rxCharacteristic = nil
        connectedPeripheral = nil
        pendingConnectId = nil
        resetMachineState()
        connectionState = .disconnected
    }

    func sendCommand(_ command: String) {
        guard connectionState == .connected,
              let peripheral = connectedPeripheral,
              let characteristic = rxCharacteristic,
              let data = "\(command)\n".data(using: .utf8)
        else {
            return
        }

        peripheral.writeValue(data, for: characteristic, type: .withResponse)
        lastSent = command
    }

    private func resetMachineState() {
        deviceMode = nil
        topLimitActive = false
        botLimitActive = false
        onHoldActive = false
    }

    private func handleIncomingMessage(_ message: String) {
        lastReceived = message
        switch message {
        case "MODE: MANUAL":
            deviceMode = .manual
        case "MODE: AUTO":
            deviceMode = .auto
        case "LIMIT: TOP":
            topLimitActive = true
        case "LIMIT: TOP OFF":
            topLimitActive = false
        case "LIMIT: BOT":
            botLimitActive = true
        case "LIMIT: BOT OFF":
            botLimitActive = false
        case "HOLD: ON":
            onHoldActive = true
        case "HOLD: OFF":
            onHoldActive = false
        default:
            break
        }
    }

    private static func matchesDevice(_ peripheral: CBPeripheral, advertisedName: String? = nil) -> Bool {
        let name = peripheral.name ?? advertisedName ?? ""
        return name.caseInsensitiveCompare(deviceName) == .orderedSame
    }

    private func appendDevice(peripheral: CBPeripheral, rssi: Int, advertisedName: String? = nil) {
        let resolvedName = peripheral.name ?? advertisedName ?? Self.deviceName
        guard resolvedName.caseInsensitiveCompare(Self.deviceName) == .orderedSame else { return }

        let device = DiscoveredDevice(
            id: peripheral.identifier,
            name: resolvedName,
            peripheral: peripheral,
            rssi: rssi
        )

        if let index = discoveredDevices.firstIndex(where: { $0.id == device.id }) {
            discoveredDevices[index] = device
        } else {
            discoveredDevices.append(device)
        }

        discoveredDevices.sort { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }

        if selectedDeviceId == nil {
            selectedDeviceId = device.id
        }
    }

    private func saveLastDevice(_ id: UUID) {
        UserDefaults.standard.set(id.uuidString, forKey: Self.lastDeviceKey)
        selectedDeviceId = id
    }

    private static func loadLastDeviceId() -> UUID? {
        guard let raw = UserDefaults.standard.string(forKey: lastDeviceKey) else { return nil }
        return UUID(uuidString: raw)
    }

    private func failConnection(_ message: String) {
        connectionState = .error
        lastError = message
        rxCharacteristic = nil
        connectedPeripheral = nil
        pendingConnectId = nil
        resetMachineState()
    }
}

extension BluetoothManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        isBluetoothReady = central.state == .poweredOn

        switch central.state {
        case .poweredOff:
            connectionState = .disconnected
            lastError = "Bật Bluetooth trên iPhone."
        case .unauthorized:
            lastError = "Cho phép Bluetooth trong Cài đặt iPhone."
        case .unsupported:
            lastError = "Thiết bị này không hỗ trợ Bluetooth LE."
        default:
            break
        }
    }

    func centralManager(
        _ central: CBCentralManager,
        didDiscover peripheral: CBPeripheral,
        advertisementData: [String: Any],
        rssi RSSI: NSNumber
    ) {
        let advertisedName = advertisementData[CBAdvertisementDataLocalNameKey] as? String
        appendDevice(peripheral: peripheral, rssi: RSSI.intValue, advertisedName: advertisedName)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.discoverServices([Self.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        failConnection("Không kết nối BLE tới TB-1E. Bấm refresh, thử lại.")
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        rxCharacteristic = nil
        connectedPeripheral = nil
        resetMachineState()
        connectionState = .disconnected
        if error != nil {
            lastError = "Mất kết nối TB-1E."
        }
    }
}

extension BluetoothManager: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if error != nil {
            failConnection("Không tìm thấy dịch vụ BLE trên TB-1E.")
            central.cancelPeripheralConnection(peripheral)
            return
        }

        guard let service = peripheral.services?.first(where: { $0.uuid == Self.serviceUUID }) else {
            failConnection("TB-1E BLE service not found.")
            central.cancelPeripheralConnection(peripheral)
            return
        }

        peripheral.discoverCharacteristics([Self.rxUUID, Self.txUUID], for: service)
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didDiscoverCharacteristicsFor service: CBService,
        error: Error?
    ) {
        if error != nil {
            failConnection("Không đọc được đặc tính BLE.")
            central.cancelPeripheralConnection(peripheral)
            return
        }

        guard let characteristics = service.characteristics else {
            failConnection("TB-1E BLE service not found.")
            central.cancelPeripheralConnection(peripheral)
            return
        }

        guard let rx = characteristics.first(where: { $0.uuid == Self.rxUUID }),
              let tx = characteristics.first(where: { $0.uuid == Self.txUUID })
        else {
            failConnection("TB-1E BLE service not found.")
            central.cancelPeripheralConnection(peripheral)
            return
        }

        rxCharacteristic = rx
        peripheral.setNotifyValue(true, for: tx)
        saveLastDevice(peripheral.identifier)
        connectionState = .connected
        lastError = nil
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didUpdateValueFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        guard error == nil,
              characteristic.uuid == Self.txUUID,
              let data = characteristic.value,
              let message = String(data: data, encoding: .utf8)?.trimmingCharacters(in: .whitespacesAndNewlines),
              !message.isEmpty
        else {
            return
        }

        lastReceived = message
        handleIncomingMessage(message)
    }

    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        if let error {
            lastError = "Send failed: \(error.localizedDescription)"
            connectionState = .error
        }
    }
}
