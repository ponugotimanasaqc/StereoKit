param(
    [switch]$upload = $false,
    [switch]$fast = $false,
    [switch]$pack = $true,
    [switch]$buildWindows    = $true,
    [switch]$buildWindowsUWP = $true,
    [switch]$buildLinux      = $true,
    [switch]$buildAndroid    = $true,
    [string]$key = ''
)

Import-Module $PSScriptRoot/Build-Utils.psm1

# Only allow an upload when everything is getting built
$upload = $upload -and $pack -and $buildWindows -and $buildWindowsUWP -and $buildLinux -and $buildAndroid

###########################################
## Functions                             ##
###########################################

function Build {
    param([parameter(Mandatory)][string] $mode, [parameter(Mandatory)][string] $project)
    & $vsExe 'StereoKit.sln' '/Build' $mode '/Project' $project | Write-Host
    return $LASTEXITCODE
}

###########################################

function Test {
    & $vsExe 'StereoKit.sln' '/Build' 'Release|X64' '/Project' 'StereoKitTest' | Out-Null
    if ($LASTEXITCODE -ne 0) {
        return $LASTEXITCODE
    }
    Push-Location -Path "$PSScriptRoot\..\bin\x64_Release\StereoKitTest"
    & '.\StereoKitTest.exe' '-test' '-screenfolder' "$PSScriptRoot/Screenshots/" | Write-Host
    Pop-Location
    return $LASTEXITCODE
}

###########################################

function Get-Key {
    if ($key -ne '') {
        Set-Content -path '.nugetkey' -value $key.Trim()
    } elseif (Test-Path '.nugetkey') {
        $key = Get-Content -path '.nugetkey' -Raw
    } else {
        $key = Read-Host "Please enter the NuGet API key, or n to cancel"
        if ($key -eq 'n') {
            $key = ''
        } else {
            Set-Content -path '.nugetkey' -value $key.Trim()
        }
    }
    return $key.Trim()
}

###########################################

function Build-Sizes {
    $size_x64        = (Get-Item "bin/distribute/bin/Win32/x64/Release/StereoKitC.dll").length
    $size_x64_linux  = (Get-Item "bin/distribute/bin/linux/x64/release/libStereoKitC.so").length
    $size_x64_uwp    = (Get-Item "bin/distribute/bin/UWP/x64/Release/StereoKitC.dll").length
    $size_arm64      = (Get-Item "bin/distribute/bin/android/arm64-v8a/release/libStereoKitC.so").length
    $size_arm64_linux= (Get-Item "bin/distribute/bin/linux/arm64/release/libStereoKitC.so").length
    $size_arm64_uwp  = (Get-Item "bin/distribute/bin/UWP/ARM64/Release/StereoKitC.dll").length
    $size_arm_uwp    = (Get-Item "bin/distribute/bin/UWP/ARM/Release/StereoKitC.dll").length

    $text = (@"
## Build Sizes:

| Platform | Arch  | Size, kb | Size, bytes |
| -------- | ----- | -------- | ----------- |
| Win32    | x64   | {0,8:N0} | {1,11:N0} |
| UWP      | x64   | {2,8:N0} | {3,11:N0} |
| UWP      | ARM64 | {4,8:N0} | {5,11:N0} |
| UWP      | ARM   | {6,8:N0} | {7,11:N0} |
| Linux    | x64   | {8,8:N0} | {9,11:N0} |
| Linux    | ARM64 | {10,8:N0} | {11,11:N0} |
| Android  | ARM64 | {12,8:N0} | {13,11:N0} |
"@ -f ([math]::Round($size_x64        /1kb), $size_x64,
       [math]::Round($size_x64_uwp    /1kb), $size_x64_uwp,
       [math]::Round($size_arm64_uwp  /1kb), $size_arm64_uwp,
       [math]::Round($size_arm_uwp    /1kb), $size_arm_uwp,
       [math]::Round($size_x64_linux  /1kb), $size_x64_linux,
       [math]::Round($size_arm64_linux/1kb), $size_arm64_linux,
       [math]::Round($size_arm64      /1kb), $size_arm64
       ))

    return $text
}

###########################################
## Main                                  ##
###########################################

# Notify about our upload flag status 
if ($fast -eq $true -and $upload -eq $true) {
    Write-Host "Let's not upload a fast build, just in case! Try again without the fast flag :)" -ForegroundColor yellow
    exit
}
Write-Host 'Building... ' -NoNewline
if ($upload -eq $false) {
    Write-Host 'local only.' -ForegroundColor White
} else {
    Write-Host 'AND UPLOADING!' -ForegroundColor Green
}
if ($fast -eq $true) {
    Write-Host 'Making a "fast" build, incremental build issues may be present.'
}

# Switch to the right folder

Push-Location -Path "$PSScriptRoot\.."

#### Update Version #######################

# Print version, so we know we're building the right version right away
$sk = Get-SKVersion

# Notify of build, and output the version
Write-Host @"
   _____ _                      _  __  _   
  / ____| |                    | |/ ( ) |  
 | (___ | |_ ___ _ __ ___  ___ |   / _| |_ 
  \___ \| __/ _ \  __/ _ \/   \|  < | | __|
  ____) | ||  __/ | |  __/ ( ) |   \| | |_ 
 |_____/ \__\___|_|  \___|\___/|_|\_\_|\__| 
"@ -NoNewline -ForegroundColor White
Write-Host "v$($sk.version)`n" -ForegroundColor Cyan

# Ensure the version string for the package matches the StereoKit version
Update-SKVersion($sk)

#### Get Visual Studio ####################

# Get the Visual Studio executable for building
$vs = Get-VSInfo
if (!$vs.exe) {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Valid Visual Studio version not found!" -ForegroundColor red
    exit 1
}
Write-Host "Using $($vs.generator)" -ForegroundColor green
$vsExe = [io.path]::ChangeExtension($vs.Exe, '.com')

#### Clean Project ########################

# Clean out the old files, do a full build
if (!(Test-Path 'bin')) {
    Write-Host 'No bin to clean, skipping clean step.'
} else {
    if (Test-Path 'bin\distribute') {
        Remove-Item 'bin\distribute' -Recurse
    }
    Write-Host 'Cleaning old files...'
    if ($fast -eq $false) {
        & $vsExe 'StereoKit.sln' '/Clean' 'Release|X64' | Out-Null
        Write-Host '..cleaned Release x64'
        & $vsExe 'StereoKit.sln' '/Clean' 'Release|ARM64' | Out-Null
        Write-Host '..cleaned Release ARM64'
        & $vsExe 'StereoKit.sln' '/Clean' 'Release|ARM' | Out-Null
        Write-Host '..cleaned Release ARM'
    }
    Write-Host 'Cleaned'
}

#### Build Windows ########################

if ($buildWindows) {
    Write-Host @"

__      __          _               
\ \    / ( )_ _  __| |_____ __ _____
 \ \/\/ /| | ' \/ _  / _ \ V  V (_-<
  \_/\_/ |_|_||_\__ _\___/\_/\_//__/

"@ -ForegroundColor White

    # Platform specific shader compile for shaders bundled in the platform binary!
    Write-Host "--- Compiling shaders as Windows only ---" -ForegroundColor green
    & 'Tools/skshaderc.exe' '-O3' '-h' '-z' '-f' '-t' 'x' '-i' 'Tools/include' 'StereoKitC/shaders_builtin/*.hlsl'

    # Build Win32 first
    Write-Host "--- Beginning build: Win32 x64 ---" -ForegroundColor green
    $result = Build -mode "Release|X64" -project "StereoKitC"
    if ($result -ne 0) {
        Write-Host '--- Win32 x64 build failed! Stopping build! ---' -ForegroundColor red
        Pop-Location
        exit 1
    }
    Write-Host "--- Finished building: Win32 x64 ---" -ForegroundColor green
    #Write-Host "--- Beginning build: Win32 ARM64 ---" -ForegroundColor green
    #$result = Build -mode "Release|ARM64" -project "StereoKitC"
    #if ($result -ne 0) {
    #    Write-Host '--- Win32 ARM64 build failed! Stopping build! ---' -ForegroundColor red
    #    exit
    #}
    #Write-Host "--- Finished building: Win32 ARM64 ---" -ForegroundColor green

    # Build UWP next
    if ($buildWindowsUWP) {
        Write-Host "--- Beginning build: UWP x64 ---" -ForegroundColor green
        $result = Build -mode "Release|X64" -project "StereoKitC_UWP"
        if ($result -ne 0) {
            Write-Host '--- UWP x64 build failed! Stopping build! ---' -ForegroundColor red
            Pop-Location
            exit 1
        }
        Write-Host "--- Finished building: UWP x64 ---" -ForegroundColor green
        Write-Host "--- Beginning build: UWP ARM64 ---" -ForegroundColor green
        $result = Build -mode "Release|ARM64" -project "StereoKitC_UWP"
        if ($result -ne 0) {
            Write-Host '--- UWP ARM64 build failed! Stopping build! ---' -ForegroundColor red
            Pop-Location
            exit 1
        }
        Write-Host "--- Finished building: UWP ARM64 ---" -ForegroundColor green
        Write-Host "--- Beginning build: UWP ARM ---" -ForegroundColor green
        $result = Build -mode "Release|ARM" -project "StereoKitC_UWP"
        if ($result -ne 0) {
            Write-Host '--- UWP ARM build failed! Stopping build! ---' -ForegroundColor red
            Pop-Location
            exit 1
        }
        Write-Host "--- Finished building: UWP ARM ---" -ForegroundColor green
    }

    #### Execute Windows Tests ########################

    # Run tests!
    if ($fast -eq $false) {
        Write-Host "`nRunning Windows Tests!"
        if ( Test -ne 0 ) {
            Write-Host '--- Tests failed! Stopping build! ---' -ForegroundColor red
            Pop-Location
            exit 1
        }
        Write-Host 'Tests passed!' -ForegroundColor green
    } else {
        Write-Host "`nSkipping tests for fast build!" -ForegroundColor yellow
    }
}

#### Build Linux ##########################
if ($buildLinux) {
    Write-Host @"

  _                   
 | |  (_)_ _ _  ___ __
 | |__| | ' \ || \ \ /
 |____|_|_||_\_,_/_\_\
                      
"@ -ForegroundColor White

    # Platform specific shader compile for shaders bundled in the platform binary!
    Write-Host "--- Compiling shaders as Linux only ---" -ForegroundColor green
    & 'Tools/skshaderc.exe' '-O3' '-h' '-z' '-f' '-t' 'g' '-i' 'Tools/include' 'StereoKitC/shaders_builtin/*.hlsl'

    # Find the correct WSL folder
    $linux_folder = ''+$PSScriptRoot
    $linux_folder = $linux_folder.replace('\', '/')
    $linux_folder = $linux_folder.replace(':', '')
    $linux_folder = '/mnt/'+$linux_folder
    Write-Output $linux_folder

    # Linux, via WSL
    Write-Host '--- Beginning WSL build: Linux ARM64 ---' -ForegroundColor green
    if ($fast -eq $false) {
        cmd /c "wsl cd '${linux_folder}' ; xmake f -p linux -a arm64 -m release -y ; xmake -r"
    } else {
        cmd /c "wsl cd '${linux_folder}' ; xmake f -p linux -a arm64 -m release -y ; xmake"
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Host '--- Linux build failed! Stopping build! ---' -ForegroundColor red
        Pop-Location
        exit 1
    }
    Write-Host '--- Finished building: Linux ARM64 ---' -ForegroundColor green


    Write-Host '--- Beginning WSL build: Linux x64 ---' -ForegroundColor green
    if ($fast -eq $false) {
        cmd /c "wsl cd '${linux_folder}' ; xmake f -p linux -a x64 -m release -y ; xmake -r"
    } else {
        cmd /c "wsl cd '${linux_folder}' ; xmake f -p linux -a x64 -m release -y ; xmake"
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Host '--- Linux build failed! Stopping build! ---' -ForegroundColor red
        Pop-Location
        exit 1
    }
    Write-Host '--- Finished building: Linux x64 ---' -ForegroundColor green
}

#### Build Android ########################

if ($buildAndroid) {
    Write-Host @"

    _           _              _ 
   /_\  _ _  __| |_ _ ___( )__| |
  / _ \| ' \/ _' | '_/ _ \ / _' |
 /_/ \_\_||_\__,_|_| \___/_\__,_|
                      
"@ -ForegroundColor White

    # Platform specific shader compile for shaders bundled in the platform binary!
    Write-Host "--- Compiling shaders as Android only ---" -ForegroundColor green
    & 'Tools/skshaderc.exe' '-O3' '-h' '-z' '-f' '-t' 'e' '-i' 'Tools/include' 'StereoKitC/shaders_builtin/*.hlsl'

    # Do cross platform build code first
    Write-Host '--- Beginning build: Android arm64-v8a' -ForegroundColor green
    xmake f -p android -a arm64-v8a -m release -y --oculus-openxr=y
    if ($fast -eq $false) {
        xmake -r
    } else {
        xmake
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Host '--- Android build failed! Stopping build! ---' -ForegroundColor red
        Pop-Location
        exit 1
    }
    Write-Host '--- Finished building: Android arm64-v8a ---' -ForegroundColor green
}

#### Assemble NuGet Package ###############

if ($pack) {
    Write-Host @"

  _  _       ___     _   
 | \| |_  _ / __|___| |_ 
 |    | || | (_ / -_)  _|
 |_|\_|\___|\___\___|\__|
                      
"@ -ForegroundColor White

    Write-Host "--- Beginning build: NuGet package ---" -ForegroundColor green
    # Turn on NuGet package generation, build, then turn it off again
    $packageOff = '<GeneratePackageOnBuild>false</GeneratePackageOnBuild>'
    $packageOn  = '<GeneratePackageOnBuild>true</GeneratePackageOnBuild>'
    Update-File -file 'StereoKit\StereoKit.csproj' -text $packageOff -with $packageOn
    $result = Build -mode "Release|Any CPU" -project "StereoKit"
    Update-File -file 'StereoKit\StereoKit.csproj' -text $packageOn -with $packageOff
    if ($result -ne 0) {
        Write-Host '--- NuGet build failed! Stopping build! ---' -ForegroundColor red
        Pop-Location
        exit 1
    }
    Write-Host "--- Finished building: NuGet package ---"-ForegroundColor green

    #### Create Build Info File ###############

    $build_size = Build-Sizes
    $build_info = "# StereoKit v$version Build Information

    $build_size"
    Set-Content -path 'Tools\BuildInfo.md' -value $build_info
    Write-Host $build_info

    #### Upload NuGet Package #################

    if ($upload) {
        $key = Get-Key
        if ($key -ne '') {
            & dotnet nuget push "bin\StereoKit.$version.nupkg" -k $key -s https://api.nuget.org/v3/index.json
        } else {
            Write-Host 'No key, cancelling upload'
        }
    }
}

#### Restore project ######################

# Put the shaders back to cross-platform to make dev a little nicer!
Write-Host "--- Restoring shaders to portable format for dev ---" -ForegroundColor green
& 'Tools/skshaderc.exe' '-O3' '-h' '-z' '-f' '-t' 'xge' '-i' 'Tools/include' 'StereoKitC/shaders_builtin/*.hlsl' | Out-Null

Write-Host "Done!" -ForegroundColor green

Pop-Location