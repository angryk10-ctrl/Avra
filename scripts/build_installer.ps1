# scripts/build_installer.ps1
# Builds both Avra_Studio_Win64.zip and the self-extracting GUI installer Avra_Studio_Setup_Win64.exe

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

$cmakeCandidates = @(
    "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "cmake.exe"
)
$cmakeExe = $cmakeCandidates | Where-Object { Get-Command $_ -ErrorAction SilentlyContinue } | Select-Object -First 1
if ($cmakeExe -and (Test-Path (Join-Path $root "build\CMakeCache.txt"))) {
    & $cmakeExe --build (Join-Path $root "build") --config Release --target Avra_Studio Avra_Setup_Stub -- /m | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE"
    }
}

$stage = Join-Path $env:TEMP "Avra_Studio_Win64_Stage"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null

$exeSrc = Join-Path $root "build\Release\Avra.exe"
if (-not (Test-Path $exeSrc)) {
    $exeSrc = Join-Path $root "Avra.exe"
}
Copy-Item $exeSrc (Join-Path $stage "Avra.exe") -Force
New-Item -ItemType Directory -Path (Join-Path $stage "bin") | Out-Null
Copy-Item $exeSrc (Join-Path $stage "bin\Avra.exe") -Force

foreach ($dir in @("include", "src", "assets")) {
    $srcDir = Join-Path $root $dir
    if (Test-Path $srcDir) {
        Copy-Item $srcDir (Join-Path $stage $dir) -Recurse -Force
    }
}
New-Item -ItemType Directory -Path (Join-Path $stage "plugins") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage "presets") -Force | Out-Null

foreach ($f in @("CMakeLists.txt", "README.md", "LICENSE")) {
    $srcFile = Join-Path $root $f
    if (Test-Path $srcFile) {
        Copy-Item $srcFile (Join-Path $stage $f) -Force
    }
}

$zipPath = Join-Path $root "Avra_Studio_Win64.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path "$stage\*" -DestinationPath $zipPath -CompressionLevel Optimal -Force
Remove-Item $stage -Recurse -Force

$stubPath = Join-Path $root "build\Release\Avra_Setup_Stub.exe"
$setupOutPath = Join-Path $root "Avra_Studio_Setup_Win64.exe"

if (Test-Path $stubPath) {
    $stubBytes = [System.IO.File]::ReadAllBytes($stubPath)
    $zipBytes  = [System.IO.File]::ReadAllBytes($zipPath)
    $zipLen    = [uint64]$zipBytes.LongLength
    $lenBytes  = [System.BitConverter]::GetBytes($zipLen)
    $magicBytes = [System.Text.Encoding]::ASCII.GetBytes("AVRASFX1")

    $outStream = [System.IO.File]::Open($setupOutPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
    try {
        $outStream.Write($stubBytes, 0, $stubBytes.Length)
        $outStream.Write($zipBytes, 0, $zipBytes.Length)
        $outStream.Write($lenBytes, 0, 8)
        $outStream.Write($magicBytes, 0, 8)
    } finally {
        $outStream.Close()
    }
    Write-Host ("Built Avra_Studio_Setup_Win64.exe ({0:N2} MB) and Avra_Studio_Win64.zip ({1:N2} MB)" -f ((Get-Item $setupOutPath).Length / 1MB), ((Get-Item $zipPath).Length / 1MB))
}
