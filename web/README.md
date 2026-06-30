# TB-1E Web Controller

Browser version of the TB-1E controller. Same UI and protocol as the Android app.

Works over **Web Bluetooth (BLE)** — connect to ESP32 **TB-1E**, hold **LÊN / DỪNG / XUỐNG**, release to send `RELEASE`.

## Browser support

| Browser | Works? | Notes |
|---------|--------|-------|
| **Chrome on PC (Windows/macOS/Linux)** | **Yes** | Same as Android app — connect **TB-1E**, mouse or keyboard |
| **Edge on PC** | **Yes** | Web Bluetooth supported |
| **Chrome on Android** | **Yes** | Touch controls |
| **Safari on iPhone** | **No** | Use iOS app or Bluefy |
| Firefox | No | Use Chrome or Edge |

**PC (Chrome):** open the GitHub Pages URL → **KẾT NỐI** → pick **TB-1E**. Hold **LÊN / DỪNG / XUỐNG** with mouse, or keys **↑ ↓ Space** (release sends `RELEASE`).

**Hardware:** ESP32-S3 / C3 / C6 with BLE firmware. Classic ESP32-WROOM (SPP only) does **not** work in the browser.

## Deploy on GitHub Pages (PC Chrome or phone)

After pushing to `main`:

1. Repo → **Settings** → **Pages** → Source: **GitHub Actions**
2. Push triggers `.github/workflows/deploy-web.yml`
3. Open the Pages URL (HTTPS), e.g.:

   `https://liemlh2801-gif.github.io/TB-1E-Wireless-Controller-V1.1/`

4. **PC:** Chrome or Edge → **KẾT NỐI** → **TB-1E** → use buttons or **↑ ↓ Space**
5. **Android:** Chrome → **KẾT NỐI** → hold touch buttons

Safari on iPhone cannot use Web Bluetooth — use the iOS app.

## HTTPS required

Web Bluetooth only runs on **secure pages** (`https://` or `http://localhost`).

Opening `http://192.168.x.x:8080` from your phone **will not work**.

### Option A — PC Chrome (GitHub Pages or localhost)

1. Deploy `web/` to GitHub Pages (HTTPS), or run locally:

```powershell
cd web
npx --yes serve -l 3000
```

2. Open **`http://localhost:3000`** in **Chrome** or **Edge** on the same PC.
3. Turn on **Bluetooth** (Windows Settings → Bluetooth).
4. Power on ESP32 **TB-1E** (BLE firmware).
5. Click **KẾT NỐI MÁY THỬ DÂY AN TOÀN** → select **TB-1E**.
6. Control: click/hold **LÊN / DỪNG / XUỐNG**, or keyboard **↑ ↓ Space** (release = `RELEASE`).

### Option B — Quick UI test on PC (no BLE over plain HTTP)

```bash
cd web
npx --yes serve -l 3000
```

Open `http://localhost:3000` — UI only; Web Bluetooth needs **HTTPS** or **localhost** (localhost works in Chrome on PC).

### Option C — Phone on same Wi‑Fi (HTTPS)

1. Install [mkcert](https://github.com/FiloSottile/mkcert) (once).
2. From the `web` folder run:

```powershell
.\serve-https.ps1
```

3. On your phone (Chrome), open `https://<your-PC-IP>:8443`
4. Accept the certificate warning (self-signed).
5. Tap **KẾT NỐI** → choose **TB-1E**.

### Option D — Deploy to HTTPS hosting

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
