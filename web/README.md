# TB-1E Web Controller

Browser version of the TB-1E controller. Same UI and protocol as the Android app.

Works over **Web Bluetooth (BLE)** — connect to ESP32 **TB-1E**, hold **LÊN / DỪNG / XUỐNG**, release to send `RELEASE`.

## Browser support

| Browser | Works? | Notes |
|---------|--------|-------|
| **Chrome on Android** | **Yes** | Recommended |
| Chrome / Edge on desktop | Yes | For testing |
| **Safari on iPhone** | **No** | Apple does not support Web Bluetooth |
| Firefox | No | Use Chrome or native app |

**Safari iPhone:** use the native **iOS app** in `ios/`, or install a Web Bluetooth browser such as [Bluefy](https://apps.apple.com/app/bluefy/id1492821405).

**Hardware:** ESP32-S3 / C3 / C6 with BLE firmware (`esp32/bt_controller/bt_controller.ino`). Classic ESP32-WROOM SPP is **not** supported in the browser.

## HTTPS required

Web Bluetooth only runs on **secure pages** (`https://` or `http://localhost`).

Opening `http://192.168.x.x:8080` from your phone **will not work**.

### Option A — Quick test on PC (Chrome)

```bash
cd web
npx --yes serve -l 3000
```

Open `http://localhost:3000` on the **same PC** with Chrome (for UI test only; phone needs HTTPS below).

### Option B — Phone on same Wi‑Fi (HTTPS)

1. Install [mkcert](https://github.com/FiloSottile/mkcert) (once).
2. From the `web` folder run:

```powershell
.\serve-https.ps1
```

3. On your phone (Chrome), open `https://<your-PC-IP>:8443`
4. Accept the certificate warning (self-signed).
5. Tap **KẾT NỐI** → choose **TB-1E**.

### Option C — Deploy to HTTPS hosting

Upload the `web/` folder to any static host with HTTPS (GitHub Pages, Netlify, etc.) and open the URL on your phone.

## Panel image

If the machine image is missing:

```bash
cp ../android/app/src/main/res/drawable/asset1.jpg web/assets/asset1.jpg
```

## Protocol

| Action | Command |
|--------|---------|
| LÊN pressed | `UP` |
| XUỐNG pressed | `DOWN` |
| DỪNG pressed | `STOP` |
| Release | `RELEASE` |

BLE service: Nordic UART `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`

## Files

```
web/
├── index.html
├── manifest.json
├── css/styles.css
├── js/bluetooth.js
├── js/app.js
├── assets/asset1.jpg
├── serve-https.ps1
└── README.md
```
