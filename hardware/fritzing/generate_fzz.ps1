# Package TB-1E Fritzing sketch (.fz) and CW-022 part into a .fzz archive.

# ESP32 uses the imported user part (ESP32 DevKitc V4) — not embedded.

# Usage: .\generate_fzz.ps1



$ErrorActionPreference = "Stop"

$Root = $PSScriptRoot

$OutFzz = Join-Path $Root "TB-1E_ESP32_CW022_Wiring.fzz"

$FzFile = Join-Path $Root "TB-1E_ESP32_CW022_Wiring.fz"

$CwPartDir = Join-Path $Root "parts\TB1E_CW022_4ch"

$CwPartId = "TB1E_CW022_4ch"



if (-not (Test-Path $FzFile)) {

    Write-Error "Missing sketch file: $FzFile"

}

if (-not (Test-Path $CwPartDir)) {

    Write-Error "Missing part folder: $CwPartDir"

}



if (Test-Path $OutFzz) {

    Remove-Item $OutFzz -Force

}



Add-Type -AssemblyName System.IO.Compression

Add-Type -AssemblyName System.IO.Compression.FileSystem

$zip = [System.IO.Compression.ZipFile]::Open($OutFzz, [System.IO.Compression.ZipArchiveMode]::Create)

$store = [System.IO.Compression.CompressionLevel]::NoCompression



function Add-TextEntry($archive, $name, $text) {

    $entry = $archive.CreateEntry($name, $store)

    $writer = New-Object System.IO.StreamWriter($entry.Open())

    $writer.Write($text)

    $writer.Close()

}



try {

    Add-TextEntry $zip "TB-1E_ESP32_CW022_Wiring.fz" (Get-Content $FzFile -Raw -Encoding UTF8)



    # Fritzing 0.9.3 expects flat part.*.fzp + svg.view.*.svg entries (see H-Bridge.fzz).

    $fzpSrc = Join-Path $CwPartDir "$CwPartId.fzp"

    $fzpText = Get-Content $fzpSrc -Raw -Encoding UTF8

    Add-TextEntry $zip "part.$CwPartId.fzp" $fzpText



    $svgMap = @(

        @{ Sub = "svg\breadboard"; Prefix = "svg.breadboard" },

        @{ Sub = "svg\schematic"; Prefix = "svg.schematic" },

        @{ Sub = "svg\icon"; Prefix = "svg.icon" }

    )

    foreach ($map in $svgMap) {

        $svgDir = Join-Path $CwPartDir $map.Sub

        if (Test-Path $svgDir) {

            Get-ChildItem -Path $svgDir -File | ForEach-Object {

                $arcName = "$($map.Prefix).$($_.Name)"

                [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $_.FullName, $arcName, $store) | Out-Null

            }

        }

    }

}

finally {

    $zip.Dispose()

}



Write-Host "Created: $OutFzz"



$AsciiDir = "C:\TB1E_Fritzing"

if (-not (Test-Path $AsciiDir)) {

    New-Item -ItemType Directory -Path $AsciiDir | Out-Null

}

Copy-Item -Path $OutFzz -Destination (Join-Path $AsciiDir "TB-1E_ESP32_CW022_Wiring.fzz") -Force

Write-Host "Copied to: $AsciiDir\TB-1E_ESP32_CW022_Wiring.fzz"


