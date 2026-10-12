$ErrorActionPreference = 'Stop'

$keil = 'D:\Keil_v5\UV4\UV4.exe'
$stlink = 'C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe'
$project = Join-Path $PSScriptRoot 'motor.uvprojx'
$hex = Join-Path $PSScriptRoot 'Objects\motor.hex'
$buildLog = Join-Path $PSScriptRoot 'build_flash_run.log'

if (Get-Process UV4 -ErrorAction SilentlyContinue) {
    throw 'Close all Keil uVision windows before running this script.'
}

$arguments = "-r `"$project`" -t `"STM32`" -o `"$buildLog`""
$build = Start-Process -FilePath $keil -ArgumentList $arguments `
    -PassThru -Wait -WindowStyle Hidden

if ($build.ExitCode -ne 0) {
    throw "Keil rebuild failed with exit code $($build.ExitCode). See $buildLog"
}

$buildText = Get-Content -LiteralPath $buildLog -Raw
if ($buildText -notmatch '0 Error\(s\), 0 Warning\(s\)') {
    throw "Keil rebuild did not finish cleanly. See $buildLog"
}

& $stlink -c SWD -P $hex -V after_programming -Rst -Run
if ($LASTEXITCODE -ne 0) {
    throw "ST-Link programming failed with exit code $LASTEXITCODE."
}

# Wait for the bounded startup alignment before checking the execution region.
Start-Sleep -Milliseconds 11500
$coreText = (& $stlink -c SWD HOTPLUG -CoreReg | Out-String)
& $stlink -c SWD HOTPLUG -Run | Out-Null

if ($coreText -notmatch 'PC\s*=\s*0x080[0-9A-Fa-f]{5}') {
    throw "Programming verified, but the CPU is not executing from Flash.`r`n$coreText"
}

Write-Host 'Build, program, verify, reset, and Flash execution check passed.'
