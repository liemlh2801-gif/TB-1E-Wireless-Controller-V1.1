import SwiftUI

@main
struct TB1EControllerApp: App {
    @StateObject private var bluetooth = BluetoothManager()

    var body: some Scene {
        WindowGroup {
            MainView()
                .environmentObject(bluetooth)
        }
    }
}
