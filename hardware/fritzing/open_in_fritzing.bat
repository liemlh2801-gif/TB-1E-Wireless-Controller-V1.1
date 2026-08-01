@echo off
REM Open TB-1E Fritzing wiring sketch with local Fritzing 0.9.3
setlocal
set "FRITZING=E:\fritzing.0.9.3b.64.pc\Fritzing.exe"
set "SRC=%~dp0TB-1E_ESP32_CW022_Wiring.fzz"
set "DSTDIR=C:\TB1E_Fritzing"
set "DST=%DSTDIR%\TB-1E_ESP32_CW022_Wiring.fzz"

if not exist "%FRITZING%" (
  echo Fritzing not found at E:\fritzing.0.9.3b.64.pc\Fritzing.exe
  pause
  exit /b 1
)

if not exist "%SRC%" (
  echo Project not found. Run: python generate_tb1e_fritzing.py
  pause
  exit /b 1
)

if not exist "%DSTDIR%" mkdir "%DSTDIR%"

REM Fritzing 0.9.3 QuaZip fails on Windows paths with non-ASCII characters (zip.open error).
copy /Y "%SRC%" "%DST%" >nul
echo Opening: %DST%
echo.
echo Note: use this batch file or open from C:\TB1E_Fritzing\ — not File -^> Open on the repo path.
start "" "%FRITZING%" "%DST%"
