$vsWhere        = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vsVersionRange = '[16.0,18.0)'

###########################################

function Get-LineNumber { return $MyInvocation.ScriptLineNumber }
function Get-ScriptName { return $MyInvocation.ScriptName }

###########################################

function Get-VSInfo {
    # Get the Visual Studio executable for building

    $vsExe          = & $vsWhere -latest -prerelease -property productPath -version $vsVersionRange
    if (!$vsExe) {
        return
    }
    $vsYear          = & $vsWhere -latest -property catalog_productLineVersion -version $vsVersionRange
    $vsVersion       = & $vsWhere -latest -property catalog_buildVersion -version $vsVersionRange
    $vsVersion       = $vsVersion.Split(".")[0]
    $vsGeneratorName = "Visual Studio $vsVersion $vsYear"

    [PSCustomObject]@{
        "exe"=$vsExe
        "generator"=$vsGeneratorName
    }
}

###########################################

function Get-Cmake {
    # Check for cmake 3.21
    $cmakeCmd = 'cmake'
    if (!(Get-Command "$cmakeCmd" -errorAction SilentlyContinue)) {
        # cmake is not in PATH, ask vswhere for a cmake version
        $cmakeCmd = & $vsWhere -latest -prerelease -version $vsVersionRange -requires Microsoft.VisualStudio.Component.VC.CMake.Project -find 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        if ($cmakeCmd -eq "" -or $null -eq $cmakeCmd -or !(Get-Command "$cmakeCmd" -errorAction SilentlyContinue)) {
            return;
        }
    }
    $cmakeVersion = & $cmakeCmd --version
    $cmakeVersion = [string]$cmakeVersion
    $cmakeVersion -match '(?<Major>\d+)\.(?<Minor>\d+)\.(?<Patch>\d+)' | Out-Null

    [PSCustomObject]@{
        "exe"=$cmakeCmd
        "major"=$Matches.Major
        "minor"=$Matches.Minor
        "patch"=$Matches.Patch
        "version"="$($Matches.Major).$($Matches.Minor).$($Matches.Patch)"
    }
}

###########################################

function Get-SKVersion {
    # stereokit.h is the source of truth for SK's version number
    $fileData = Get-Content -path "$PSScriptRoot\..\StereoKitC\stereokit.h" -Raw;
    $fileData -match '#define SK_VERSION_MAJOR\s+(?<ver>\d+)' | Out-Null
    $major = $Matches.ver
    $fileData -match '#define SK_VERSION_MINOR\s+(?<ver>\d+)' | Out-Null
    $minor = $Matches.ver
    $fileData -match '#define SK_VERSION_PATCH\s+(?<ver>\d+)' | Out-Null
    $patch = $Matches.ver
    $fileData -match '#define SK_VERSION_PRERELEASE\s+(?<ver>\d+)' | Out-Null
    $pre = $Matches.ver

    [PSCustomObject]@{
        "major"=$major
        "minor"=$minor
        "patch"=$patch
        "prerelease"=$pre
        "version"="$major.$minor.$patch-preview.$pre"
    }
}

###########################################

function Update-File {
    param($file, $text, $with)

    ((Get-Content -path $file) -replace $text,$with) | Set-Content -path $file
}

###########################################

function Update-SKVersion { param($sk)
    # These are the other spots that contain version numbers
    Update-File -file "$PSScriptRoot\..\StereoKit\StereoKit.csproj" -text '<Version>(.*)</Version>'  -with "<Version>$($sk.version)</Version>"
    Update-File -file "$PSScriptRoot\..\xmake.lua"                  -text 'set_version(.*)'          -with "set_version(`"$($sk.version)`")"
    Update-File -file "$PSScriptRoot\..\CMakeLists.txt"             -text 'StereoKit VERSION "(.*)"' -with "StereoKit VERSION `"$($sk.major).$($sk.minor).$($sk.patch)`""
}