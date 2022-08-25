param(
    [ValidateSet('x86','x64','ARM','ARM64')]
    [string]$arch = 'x64',
    [ValidateSet('Win32','UWP','Linux','Android')]
    [string]$plat = 'UWP',
    [ValidateSet('Release','Debug')]
    [string]$config = 'Release'
)

$build_folder = "$($plat)_$($arch)_$($config)"

#############################################
######## Check Visual Studio version ########
#############################################

# Get the Visual Studio executable for building
$vsWhere        = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vsVersionRange = '[16.0,18.0)'
$vsExe          = & $vsWhere -latest -prerelease -property productPath -version $vsVersionRange
if (!$vsExe) {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Valid Visual Studio version not found!" -ForegroundColor red
    exit 
}
$vsYear          = & $vsWhere -latest -property catalog_productLineVersion -version $vsVersionRange
$vsVersion       = & $vsWhere -latest -property catalog_buildVersion -version $vsVersionRange
$vsVersion       = $vsVersion.Split(".")[0]
$vsGeneratorName = "Visual Studio $vsVersion $vsYear"
Write-Host "Using $vsGeneratorName" -ForegroundColor green

#####################################
######## Check CMake version ########
#####################################

# Check for cmake 3.21
$cmakeCmd = 'cmake'
if (!(Get-Command "$cmakeCmd" -errorAction SilentlyContinue))
{
    # cmake is not in PATH, ask vswhere for a cmake version
    $cmakeCmd = & $vsWhere -latest -prerelease -version $vsVersionRange -requires Microsoft.VisualStudio.Component.VC.CMake.Project -find 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if ($cmakeCmd -eq "" -or $null -eq $cmakeCmd -or !(Get-Command "$cmakeCmd" -errorAction SilentlyContinue))
    {
        # No cmake available, error out and point users to cmake's website 
        Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Cmake not detected! It is needed to build dependencies, please install or add to Path!" -ForegroundColor red
        Start-Process 'https://cmake.org/'
        exit 1
    }
}
$Matches = {}
$cmakeVersion = & $cmakeCmd --version
$cmakeVersion = [string]$cmakeVersion
$cmakeVersion -match '(?<Major>\d+)\.(?<Minor>\d+)\.(?<Patch>\d+)' | Out-Null
$cmvMajor = $Matches.Major
$cmvMinor = $Matches.Minor
$cmvPatch = $Matches.Patch
if ( $cmvMajor -lt 3 -or
    ($cmvMajor -eq 3 -and $cmvMinor -lt 21)) {
    Write-Host "$(Get-ScriptName)($(Get-LineNumber),0): error: Cmake version must be greater than 3.21! Found $cmvMajor.$cmvMinor.$cmvPatch. Please update and try again!" -ForegroundColor red
    Start-Process 'https://cmake.org/'
    exit 1
} else {
    Write-Host "Using cmake version: $cmvMajor.$cmvMinor.$cmvPatch" -ForegroundColor green
}

########################################

if (!(Test-Path -Path 'build_cmake')) {
    New-Item -Path . -Name 'build_cmake' -ItemType "directory" | Out-Null
}
Push-Location 'build_cmake'
if (!(Test-Path -Path $build_folder)) {
    New-Item -Path . -Name $build_folder -ItemType "directory" | Out-Null
}
Push-Location $build_folder

Write-Host "$($dep.Name): Configuring $arch $plat" -ForegroundColor green
$vsArch = $arch
if ($vsArch -eq 'x86') {
    $vsArch = 'Win32'
}
if ($plat -eq 'UWP') {
    & $cmakeCmd -G $vsGeneratorName -A $vsArch "-DCMAKE_BUILD_TYPE=$config" $dep.CmakeOptions '-DCMAKE_CXX_FLAGS=/MP' '-DCMAKE_SYSTEM_NAME=WindowsStore' '-DCMAKE_SYSTEM_VERSION=10.0' '-DDYNAMIC_LOADER=OFF' '-Wno-deprecated' '-Wno-dev' ../..
} else {
    & $cmakeCmd -G $vsGeneratorName -A $vsArch "-DCMAKE_BUILD_TYPE=$config" $dep.CmakeOptions '-DCMAKE_CXX_FLAGS=/MP' '-DDYNAMIC_LOADER=OFF' '-Wno-deprecated' '-Wno-dev' ../..
}

Write-Host "$($dep.Name): Building $arch $plat" -ForegroundColor green
& $cmakeCmd --build . --config $config -- /m

Pop-Location
Pop-Location