param(
    [ValidateSet('x86','x64','ARM','ARM64')]
    [string]$arch = 'ARM64',
    [ValidateSet('Win32','UWP','Linux','Android')]
    [string]$plat = 'Android',
    [ValidateSet('Release','Debug')]
    [string]$config = 'Release'
)

Import-Module $PSScriptRoot/Build-Utils.psm1

$build_folder = "$($plat)_$($arch)_$($config)"

#############################################
######## Check required build tools #########
#############################################

$vs = Get-VSInfo
if (!$vs.exe -and $plat -ne 'Linux') {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Valid Visual Studio version not found!" -ForegroundColor red
    exit 1
}
Write-Host "Using $($vs.generator)" -ForegroundColor green

$cmake = Get-Cmake
if (!$cmake.exe) {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Cmake not detected! It is needed to build dependencies, please install or add to Path!" -ForegroundColor red
    Start-Process 'https://cmake.org/'
    exit 1
} elseif ( $cmake.major -lt 3 -or ($cmake.major -eq 3 -and $cmake.minor -lt 21)) {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Cmake version must be greater than 3.21! Found $($cmake.version). Please update and try again!" -ForegroundColor red
    Start-Process 'https://cmake.org/'
    exit 1
}
Write-Host "Using Cmake version: $($cmake.version)" -ForegroundColor green

########################################

Push-Location "$PSScriptRoot/.."
if (!(Test-Path -Path 'build_cmake')) {
    New-Item -Path . -Name 'build_cmake' -ItemType "directory" | Out-Null
}
Push-Location 'build_cmake'
if (!(Test-Path -Path $build_folder)) {
    New-Item -Path . -Name $build_folder -ItemType "directory" | Out-Null
}
Push-Location $build_folder

Write-Host "Configuring $arch $plat" -ForegroundColor green
$vsArch = $arch
if ($vsArch -eq 'x86') {
    $vsArch = 'Win32'
}
if ($plat -eq 'UWP') {
    & $cmake.exe -G $vs.generator -A $vsArch "-DCMAKE_BUILD_TYPE=$config" '-DCMAKE_CXX_FLAGS=/MP' '-DCMAKE_SYSTEM_NAME=WindowsStore' '-DCMAKE_SYSTEM_VERSION=10.0' '-DDYNAMIC_LOADER=ON' '-Wno-deprecated' '-Wno-dev' ../..
} elseif ($plat -eq 'Android') {
    # https://developer.android.com/ndk/guides/cmake

    if ($null -eq $Env:NDK) {
        Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: NDK not found! Make sure the NDK environment variable is defined." -ForegroundColor red
        exit 1
    }
    Write-Host "Using NDK: $Env:NDK" -ForegroundColor green

    $androidArch = $arch
    if     ($arch -eq 'x64')   { $androidArch = 'x86_64'}
    elseif ($arch -eq 'ARM64') { $androidArch = 'arm64-v8a' }

    & $cmake.exe -G Ninja `
        "-DCMAKE_BUILD_TYPE=$config" `
        "-DANDROID_ABI=$androidArch" `
        "-DCMAKE_ANDROID_ARCH_ABI=$androidArch" `
        "-DCMAKE_TOOLCHAIN_FILE=$Env:NDK/build/cmake/android.toolchain.cmake" `
        "-DANDROID_NDK=$Env:NDK" `
        "-DCMAKE_ANDROID_NDK=$Env:NDK" `
        '-DANDROID_PLATFORM=android-21' `
        '-DCMAKE_SYSTEM_VERSION=21' `
        '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON' `
        '-DCMAKE_SYSTEM_NAME=Android' `
        '-DCMAKE_MAKE_PROGRAM=C:/Users/progr/AppData/Local/Android/Sdk/cmake/3.18.1/bin/ninja.exe' `
        '-DDYNAMIC_LOADER=ON' `
        '-Wno-deprecated' `
        '-Wno-dev' `
        ../..

} elseif ($plat -eq 'Linux') {
    & $cmake.exe -A $vsArch "-DCMAKE_BUILD_TYPE=$config" '-DCMAKE_CXX_FLAGS=/MP' '-DDYNAMIC_LOADER=ON' '-Wno-deprecated' '-Wno-dev' ../..
} else {
    & $cmake.exe -G $vs.generator -A $vsArch "-DCMAKE_BUILD_TYPE=$config" '-DCMAKE_CXX_FLAGS=/MP' '-DDYNAMIC_LOADER=ON' '-Wno-deprecated' '-Wno-dev' ../..
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Cmake configuration failed! Error code: $LASTEXITCODE" -ForegroundColor red
    exit 1
}

Write-Host "Building $arch $plat" -ForegroundColor green
& $cmake.exe --build . --config $config --

Pop-Location
Pop-Location
Pop-Location