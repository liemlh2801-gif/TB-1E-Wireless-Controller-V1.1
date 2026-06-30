import SwiftUI

private let goldBar = Color(red: 0.831, green: 0.686, blue: 0.216)
private let panelButtonFill = Color(red: 0.878, green: 0.878, blue: 0.878)
private let connectButtonFill = Color(red: 0.702, green: 0.898, blue: 0.988)
private let appAspectRatio: CGFloat = 9.0 / 19.5

struct MainView: View {
    @EnvironmentObject private var bluetooth: BluetoothManager
    @Environment(\.scenePhase) private var scenePhase

    var body: some View {
        GeometryReader { proxy in
            ZStack {
                Color.black.ignoresSafeArea()

                let frameSize = aspectFitFrame(in: proxy.size)
                ZStack {
                    Color.white

                    VStack(spacing: 0) {
                        topBar
                        panelContent
                            .frame(maxWidth: .infinity, maxHeight: .infinity)
                    }
                }
                .frame(width: frameSize.width, height: frameSize.height)
                .position(x: proxy.size.width / 2, y: proxy.size.height / 2)
            }
        }
        .onAppear {
            if bluetooth.isBluetoothReady {
                bluetooth.refreshDevices()
            }
        }
        .onChange(of: bluetooth.isBluetoothReady) { ready in
            if ready {
                bluetooth.refreshDevices()
            }
        }
        .onDisappear {
            bluetooth.disconnect(sendRelease: true)
        }
        .onChange(of: scenePhase) { phase in
            if phase != .active, bluetooth.isConnected {
                bluetooth.sendCommand("RELEASE")
            }
        }
    }

    private var topBar: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text("Bluetooth Controller")
                    .font(.headline)
                    .foregroundStyle(.white)
                Text("v\(BluetoothManager.appVersion)")
                    .font(.caption)
                    .foregroundStyle(.white.opacity(0.85))
            }

            Spacer()

            Menu {
                Button("Refresh devices") {
                    bluetooth.refreshDevices()
                }

                Divider()

                if bluetooth.discoveredDevices.isEmpty {
                    Text("No TB-1E found")
                } else {
                    ForEach(bluetooth.discoveredDevices) { device in
                        Button {
                            bluetooth.selectedDeviceId = device.id
                        } label: {
                            if bluetooth.selectedDeviceId == device.id {
                                Label(device.label, systemImage: "checkmark")
                            } else {
                                Text(device.label)
                            }
                        }
                    }
                }

                if let lastSent = bluetooth.lastSent {
                    Divider()
                    Text("Last sent: \(lastSent)")
                }
                if let lastReceived = bluetooth.lastReceived {
                    Text("Last received: \(lastReceived)")
                }
            } label: {
                Image(systemName: "ellipsis.circle")
                    .font(.title2)
                    .foregroundStyle(.white)
            }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(goldBar)
    }

    private var panelContent: some View {
        GeometryReader { geo in
            let connectWidth = geo.size.width * PanelLayout.connectWidth
            let connectEnd = geo.size.width * PanelLayout.connectEnd
            let controlWidth = min(max(geo.size.width * PanelLayout.controlWidth, 120), 210)
            let controlEnd = geo.size.width * PanelLayout.controlEnd
            let machineWidth = geo.size.width * PanelLayout.machineWidth

            ZStack(alignment: .topTrailing) {
                HStack(spacing: 0) {
                    Image("asset1")
                        .resizable()
                        .scaledToFit()
                        .frame(width: machineWidth)
                        .accessibilityLabel("Safety belt tester machine panel")
                    Spacer(minLength: 0)
                }

                LeaderLinesOverlay(
                    machineWidth: machineWidth,
                    controlEnd: controlEnd,
                    controlWidth: controlWidth,
                    height: geo.size.height
                )

                VStack(alignment: .trailing, spacing: 8) {
                    connectButton
                        .frame(width: connectWidth)

                    statusRow

                    if let error = bluetooth.lastError {
                        Text(error)
                            .font(.caption.weight(.medium))
                            .foregroundStyle(Color(red: 0.776, green: 0.157, blue: 0.157))
                            .multilineTextAlignment(.trailing)
                            .frame(width: connectWidth, alignment: .trailing)
                    }
                }
                .padding(.top, geo.size.height * PanelLayout.connectTop)
                .padding(.trailing, connectEnd)

                panelButton(title: "LÊN", enabled: bluetooth.isConnected) {
                    bluetooth.sendCommand("UP")
                } onRelease: {
                    bluetooth.sendCommand("RELEASE")
                }
                .frame(width: controlWidth)
                .padding(.trailing, controlEnd)
                .offset(y: geo.size.height * PanelLayout.btnUpY)

                panelButton(title: "DỪNG", enabled: bluetooth.isConnected) {
                    bluetooth.sendCommand("STOP")
                } onRelease: {
                    bluetooth.sendCommand("RELEASE")
                }
                .frame(width: controlWidth)
                .padding(.trailing, controlEnd)
                .offset(y: geo.size.height * PanelLayout.btnStopY)

                panelButton(title: "XUỐNG", enabled: bluetooth.isConnected) {
                    bluetooth.sendCommand("DOWN")
                } onRelease: {
                    bluetooth.sendCommand("RELEASE")
                }
                .frame(width: controlWidth)
                .padding(.trailing, controlEnd)
                .offset(y: geo.size.height * PanelLayout.btnDownY)
            }
        }
    }

    private var connectButton: some View {
        Button {
            if bluetooth.isConnected {
                bluetooth.disconnect(sendRelease: true)
            } else {
                bluetooth.connectSelectedDevice()
            }
        } label: {
            Text(connectLabel)
                .font(.system(size: 10, weight: .bold))
                .multilineTextAlignment(.center)
                .lineLimit(1)
                .minimumScaleFactor(0.7)
                .frame(maxWidth: .infinity)
                .frame(height: 48)
                .background(connectButtonFill.opacity(connectEnabled ? 1 : 0.5))
                .overlay(
                    RoundedRectangle(cornerRadius: 10)
                        .stroke(Color.black, lineWidth: 1.5)
                )
        }
        .disabled(!connectEnabled)
        .buttonStyle(.plain)
    }

    private var connectLabel: String {
        switch bluetooth.connectionState {
        case .connecting:
            return "ĐANG KẾT NỐI…"
        case .connected:
            return "NGẮT KẾT NỐI"
        default:
            return "KẾT NỐI MÁY THỬ DÂY AN TOÀN"
        }
    }

    private var connectEnabled: Bool {
        bluetooth.isConnected || bluetooth.selectedDevice != nil
    }

    private var statusRow: some View {
        HStack(spacing: 8) {
            Circle()
                .fill(statusColor)
                .frame(width: 12, height: 12)
            Text(statusLabel)
                .font(.system(size: 14, weight: .bold))
                .foregroundStyle(.black)
        }
    }

    private var statusColor: Color {
        switch bluetooth.connectionState {
        case .connected:
            return Color(red: 0.180, green: 0.490, blue: 0.196)
        case .connecting:
            return Color(red: 0.976, green: 0.659, blue: 0.145)
        case .error, .disconnected:
            return Color(red: 0.776, green: 0.157, blue: 0.157)
        }
    }

    private var statusLabel: String {
        switch bluetooth.connectionState {
        case .connected:
            return "Status: Connected"
        case .connecting:
            return "Status: Connecting…"
        case .error:
            return "Status: Error"
        case .disconnected:
            return "Status: Disconnected"
        }
    }

    private func panelButton(
        title: String,
        enabled: Bool,
        onPress: @escaping () -> Void,
        onRelease: @escaping () -> Void
    ) -> some View {
        MomentaryButton(
            title: title,
            enabled: enabled,
            onPress: onPress,
            onRelease: onRelease
        )
    }

    private func aspectFitFrame(in size: CGSize) -> CGSize {
        let targetRatio = appAspectRatio
        let currentRatio = size.width / size.height

        if currentRatio > targetRatio {
            let height = size.height
            return CGSize(width: height * targetRatio, height: height)
        }

        let width = size.width
        return CGSize(width: width, height: width / targetRatio)
    }
}

private struct MomentaryButton: View {
    let title: String
    let enabled: Bool
    let onPress: () -> Void
    let onRelease: () -> Void

    @State private var isPressed = false

    var body: some View {
        Text(title)
            .font(.system(size: 17, weight: .bold))
            .foregroundStyle(enabled ? .black : .black.opacity(0.45))
            .frame(maxWidth: .infinity)
            .frame(height: PanelLayout.panelButtonHeight)
            .background(panelButtonFill.opacity(enabled ? 1 : 0.45))
            .overlay(
                RoundedRectangle(cornerRadius: 10)
                    .stroke(Color.black.opacity(enabled ? 1 : 0.45), lineWidth: 1.5)
            )
            .contentShape(RoundedRectangle(cornerRadius: 10))
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { _ in
                        guard enabled, !isPressed else { return }
                        isPressed = true
                        onPress()
                    }
                    .onEnded { _ in
                        guard isPressed else { return }
                        isPressed = false
                        onRelease()
                    }
            )
    }
}

private struct LeaderLinesOverlay: View {
    let machineWidth: CGFloat
    let controlEnd: CGFloat
    let controlWidth: CGFloat
    let height: CGFloat

    var body: some View {
        Canvas { context, size in
            let buttonLeft = size.width - controlEnd - controlWidth
            let buttonHeight = PanelLayout.panelButtonHeight

            func buttonCenterY(_ anchor: CGFloat) -> CGFloat {
                height * anchor + buttonHeight / 2
            }

            func mapImagePoint(xFraction: CGFloat, yFraction: CGFloat) -> CGPoint {
                let scale = min(
                    machineWidth / PanelLayout.asset1Width,
                    height / PanelLayout.asset1Height
                )
                let displayedWidth = PanelLayout.asset1Width * scale
                let displayedHeight = PanelLayout.asset1Height * scale
                let offsetX = (machineWidth - displayedWidth) / 2
                let offsetY = (height - displayedHeight) / 2
                return CGPoint(
                    x: offsetX + xFraction * displayedWidth,
                    y: offsetY + yFraction * displayedHeight
                )
            }

            let upCircle = mapImagePoint(
                xFraction: PanelLayout.circleXInImage,
                yFraction: PanelLayout.circleUpYInImage
            )
            let stopCircle = mapImagePoint(
                xFraction: PanelLayout.circleXInImage,
                yFraction: PanelLayout.circleStopYInImage
            )
            let downCircle = mapImagePoint(
                xFraction: PanelLayout.circleXInImage,
                yFraction: PanelLayout.circleDownYInImage
            )

            let upButtonY = buttonCenterY(PanelLayout.btnUpY)
            let stopButtonY = buttonCenterY(PanelLayout.btnStopY)
            let downButtonY = buttonCenterY(PanelLayout.btnDownY)

            func junctionX(_ circleX: CGFloat) -> CGFloat {
                circleX + (buttonLeft - circleX) * PanelLayout.lineJunctionFraction
            }

            drawLeaderLine(context: &context, from: upCircle, junctionX: junctionX(upCircle.x), buttonY: upButtonY, buttonLeft: buttonLeft)
            drawLeaderLine(context: &context, from: stopCircle, junctionX: junctionX(stopCircle.x), buttonY: stopButtonY, buttonLeft: buttonLeft)
            drawLeaderLine(context: &context, from: downCircle, junctionX: junctionX(downCircle.x), buttonY: downButtonY, buttonLeft: buttonLeft)
        }
    }

    private func drawLeaderLine(
        context: inout GraphicsContext,
        from circle: CGPoint,
        junctionX: CGFloat,
        buttonY: CGFloat,
        buttonLeft: CGFloat
    ) {
        var path = Path()
        path.move(to: circle)
        path.addLine(to: CGPoint(x: junctionX, y: buttonY))
        path.addLine(to: CGPoint(x: buttonLeft, y: buttonY))
        context.stroke(path, with: .color(.black), lineWidth: 0.75)
    }
}

#Preview {
    MainView()
        .environmentObject(BluetoothManager())
}
