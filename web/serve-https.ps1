# Serve web/ over HTTPS for Web Bluetooth on phone Chrome.
# Usage: cd web && .\serve-https.ps1

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $here

$port = 8443
$cert = Join-Path $here "dev-cert.pem"
$key = Join-Path $here "dev-key.pem"

if (-not (Test-Path $cert)) {
    Write-Host "Creating self-signed certificate..."
    $openssl = Get-Command openssl -ErrorAction SilentlyContinue
    if (-not $openssl) {
        Write-Host "Install OpenSSL or mkcert, then re-run."
        Write-Host "Alternative: deploy web/ to GitHub Pages / Netlify (HTTPS)."
        exit 1
    }
    & openssl req -x509 -newkey rsa:2048 -keyout $key -out $cert -days 365 -nodes -subj "/CN=TB-1E-local"
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}

if (-not $python) {
    Write-Host "Python not found. Install Python 3 or use: npx serve (HTTP only - phone BLE won't work)."
    exit 1
}

$ip = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike '127.*' -and $_.PrefixOrigin -ne 'WellKnown' } | Select-Object -First 1).IPAddress
Write-Host ""
Write-Host "Open on phone Chrome (same Wi-Fi):"
Write-Host "  https://${ip}:${port}/"
Write-Host ""
Write-Host "Accept the certificate warning once."
Write-Host "Press Ctrl+C to stop."
Write-Host ""

$pyScript = @"
import http.server, ssl, os
os.chdir(r'$here')
httpd = http.server.HTTPServer(('0.0.0.0', $port), http.server.SimpleHTTPRequestHandler)
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.load_cert_chain(certfile=r'$cert', keyfile=r'$key')
httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
httpd.serve_forever()
"@

& $python.Source -c $pyScript
