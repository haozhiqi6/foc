[CmdletBinding()]
param([switch]$NoPathUpdate)

$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
$Tools = Join-Path $Root 'tools'
$Downloads = Join-Path $Tools 'downloads'
New-Item -ItemType Directory -Force -Path $Downloads | Out-Null

$gccZip = Join-Path $Downloads 'arm-gnu-toolchain-14.3.rel1-mingw-w64-i686-arm-none-eabi.zip'
$gccUrl = 'https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-mingw-w64-i686-arm-none-eabi.zip'
$gccRoot = Join-Path $Tools 'arm-gnu-toolchain-14.3.rel1'
$gccBin = Get-ChildItem -LiteralPath $gccRoot -Filter 'arm-none-eabi-gcc.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $gccBin) {
    if (-not (Test-Path $gccZip)) {
        Write-Host 'Downloading Arm GNU Toolchain 14.3.Rel1...'
        Invoke-WebRequest -Uri $gccUrl -OutFile $gccZip
    }
    Expand-Archive -LiteralPath $gccZip -DestinationPath $gccRoot -Force
    $gccBin = Get-ChildItem -LiteralPath $gccRoot -Filter 'arm-none-eabi-gcc.exe' -Recurse | Select-Object -First 1
}
if (-not $gccBin) { throw 'arm-none-eabi-gcc.exe was not found after extraction.' }

$openocdRoot = Join-Path $Tools 'openocd'
$openocdBin = Get-ChildItem -LiteralPath $openocdRoot -Filter 'openocd.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $openocdBin) {
    Write-Host 'Resolving latest xPack OpenOCD Windows release...'
    $release = Invoke-RestMethod -Uri 'https://api.github.com/repos/xpack-dev-tools/openocd-xpack/releases/latest'
    $asset = $release.assets | Where-Object { $_.name -match 'win32-x64.*\.zip$' } | Select-Object -First 1
    if (-not $asset) { throw 'No Windows x64 OpenOCD zip was found in the latest release.' }
    $openocdZip = Join-Path $Downloads $asset.name
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $openocdZip
    Expand-Archive -LiteralPath $openocdZip -DestinationPath $openocdRoot -Force
    $openocdBin = Get-ChildItem -LiteralPath $openocdRoot -Filter 'openocd.exe' -Recurse | Select-Object -First 1
}
if (-not $openocdBin) { throw 'openocd.exe was not found after extraction.' }

$gccDir = $gccBin.Directory.FullName
$openocdDir = $openocdBin.Directory.FullName
[Environment]::SetEnvironmentVariable('ARM_GCC_BIN', $gccDir, 'User')
[Environment]::SetEnvironmentVariable('OPENOCD_BIN', $openocdDir, 'User')
if (-not $NoPathUpdate) {
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $parts = @($userPath -split ';' | Where-Object { $_ })
    foreach ($entry in @($gccDir, $openocdDir)) {
        if ($parts -notcontains $entry) { $parts += $entry }
    }
    [Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User')
}
$env:ARM_GCC_BIN = $gccDir
$env:OPENOCD_BIN = $openocdDir
$env:Path = "$gccDir;$openocdDir;$env:Path"
Write-Host "GCC: $gccDir"
Write-Host "OpenOCD: $openocdDir"
Write-Host 'Tool installation completed. Restart VS Code so it sees the updated PATH.'
