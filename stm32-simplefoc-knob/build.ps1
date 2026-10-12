[CmdletBinding()]
param([switch]$Clean)
$ErrorActionPreference = 'Stop'
$AsciiRoot = 'C:\Users\PC\Documents\Codex\stm32-motor'
$Root = if (Test-Path -LiteralPath $AsciiRoot) { $AsciiRoot } else { $PSScriptRoot }
$Build = Join-Path $Root 'build'
$Obj = Join-Path $Build 'obj'
$Bin = Join-Path $Build 'bin'

function Resolve-Tool([string]$Name, [string[]]$Candidates) {
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "Cannot find $Name. Add GNU Arm tools to PATH or set ARM_GCC_BIN."
}

$gccCandidates = @(
    (Join-Path $Root 'tools\arm-gnu-toolchain-14.3.rel1\bin\arm-none-eabi-gcc.exe')
)
if ($env:ARM_GCC_BIN) { $gccCandidates += (Join-Path $env:ARM_GCC_BIN 'arm-none-eabi-gcc.exe') }
$gccCandidates += @(
    "$env:USERPROFILE\AppData\Local\Programs\Arm GNU Toolchain arm-none-eabi\bin\arm-none-eabi-gcc.exe",
    'C:\Program Files\Arm GNU Toolchain arm-none-eabi\bin\arm-none-eabi-gcc.exe'
)
$gcc = Resolve-Tool 'arm-none-eabi-gcc' $gccCandidates
$gccDir = Split-Path $gcc
$objcopy = Resolve-Tool 'arm-none-eabi-objcopy' @((Join-Path $gccDir 'arm-none-eabi-objcopy.exe'))
$size = Resolve-Tool 'arm-none-eabi-size' @((Join-Path $gccDir 'arm-none-eabi-size.exe'))

if ($Clean -and (Test-Path $Build)) { Remove-Item -Recurse -Force -LiteralPath $Build }
New-Item -ItemType Directory -Force -Path $Obj,$Bin | Out-Null

$common = @(
    '-mcpu=cortex-m3','-mthumb','-mfloat-abi=soft','-ffunction-sections','-fdata-sections',
    '-fno-common','-g3','-Og','-Wall','-Wextra','-Wno-unused-parameter','-std=gnu11',
    '-DSTM32F10X_MD','-DUSE_STDPERIPH_DRIVER','-DHSE_VALUE=8000000','-D__nop=__NOP',
    '-Ilib/inc','-Istartup','-IHardware','-ISimpleFOC','-Iuser'
)
$sources = @()
$sources += (Get-ChildItem -File -Recurse (Join-Path $Root 'startup') -Filter '*.c').FullName
$sources += (Get-ChildItem -File -Recurse (Join-Path $Root 'lib/src') -Filter '*.c').FullName
$sources += (Get-ChildItem -File -Recurse (Join-Path $Root 'Hardware') -Filter '*.c').FullName
$sources += (Get-ChildItem -File -Recurse (Join-Path $Root 'SimpleFOC') -Filter '*.c').FullName
$sources += (Get-ChildItem -File -Recurse (Join-Path $Root 'user') -Filter '*.c').FullName
$sources += (Join-Path $Root 'startup/startup_stm32f103x8.S')

$objects = @()
$compileDb = @()
foreach ($source in $sources) {
    $relative = $source.Substring($Root.TrimEnd('\\').Length).TrimStart('\\')
    $object = Join-Path $Obj (($relative -replace '[\\/]', '_') + '.o')
    $objects += $object
    $args = @('-c',$source,'-o',$object) + $common
    Write-Host "CC $relative"
    & $gcc @args
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $relative" }
    $compileDb += [pscustomobject]@{ directory=$Root; file=$source; arguments=@($gcc)+$args }
}
$compileDb | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 -LiteralPath (Join-Path $Build 'compile_commands.json')

$elf = Join-Path $Bin 'motor.elf'
$linkArgs = @(
    '-mcpu=cortex-m3','-mthumb','-mfloat-abi=soft','-T',(Join-Path $Root 'STM32F103C8.ld'),
    '-Wl,--gc-sections',('-Wl,-Map=' + (Join-Path $Bin 'motor.map')),
    '-Wl,--print-memory-usage','-specs=nano.specs','-specs=nosys.specs','-u','_printf_float',
    '-o',$elf
) + $objects + @('-lm')
Write-Host 'LINK motor.elf'
& $gcc @linkArgs
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
& $objcopy -O ihex $elf (Join-Path $Bin 'motor.hex')
& $objcopy -O binary $elf (Join-Path $Bin 'motor.bin')
& $size $elf
Write-Host "Output: $Bin"
