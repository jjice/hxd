Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoUrl = 'https://github.com/jjice/hxed'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

function Read-Value {
    param(
        [string]$Prompt,
        [string]$Default
    )

    $value = Read-Host "$Prompt [$Default]"
    if ([string]::IsNullOrWhiteSpace($value)) {
        return $Default
    }

    return $value
}

function Read-YesNo {
    param(
        [string]$Prompt,
        [bool]$Default = $true
    )

    $suffix = if ($Default) { '[Y/n]' } else { '[y/N]' }

    while ($true) {
        $answer = Read-Host "$Prompt $suffix"
        if ([string]::IsNullOrWhiteSpace($answer)) {
            return $Default
        }

        switch ($answer.Trim().ToLowerInvariant()) {
            'y' { return $true }
            'yes' { return $true }
            'n' { return $false }
            'no' { return $false }
        }

        Write-Host 'Please answer y or n.'
    }
}

function Read-InstallMode {
    while ($true) {
        Write-Host 'Installation source:'
        Write-Host '  1) Build from source in this repository'
        Write-Host '  2) Download the latest release binary from GitHub'

        $choice = Read-Host 'Choose 1 or 2 [2]'
        if ([string]::IsNullOrWhiteSpace($choice)) {
            $choice = '2'
        }

        switch ($choice) {
            '1' { return 'build' }
            '2' { return 'release' }
        }

        Write-Host 'Please answer with 1 or 2.'
    }
}

function Require-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command not found: $Name"
    }
}

function Resolve-BuiltBinary {
    $candidates = @(
        (Join-Path $RepoRoot 'build\bin\hxed.exe'),
        (Join-Path $RepoRoot 'build\bin\Release\hxed.exe')
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw 'Build finished, but no hxed.exe was found in build\bin.'
}

function Ensure-UserPath {
    param([string]$InstallDir)

    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $entries = @()

    if (-not [string]::IsNullOrWhiteSpace($userPath)) {
        $entries = $userPath -split ';'
    }

    if ($entries -contains $InstallDir) {
        Write-Host "PATH already contains $InstallDir."
        return
    }

    if (-not (Read-YesNo "Add $InstallDir to your user PATH?" $true)) {
        Write-Host 'Skipped PATH update.'
        return
    }

    $newPath = if ([string]::IsNullOrWhiteSpace($userPath)) {
        $InstallDir
    }
    else {
        "$userPath;$InstallDir"
    }

    [Environment]::SetEnvironmentVariable('Path', $newPath, 'User')
    if (-not (($env:Path -split ';') -contains $InstallDir)) {
        $env:Path = "$env:Path;$InstallDir"
    }

    Write-Host 'User PATH updated. Open a new terminal to pick it up everywhere.'
}

function Install-PowerShellCompletion {
    param([string]$InstallDir)

    $completionSource = Join-Path $RepoRoot 'completions\hxed.ps1'
    if (-not (Test-Path $completionSource)) {
        throw "Completion file not found: $completionSource"
    }

    $completionTarget = Join-Path $InstallDir 'hxed.ps1'
    Copy-Item $completionSource $completionTarget -Force

    $profileDir = Split-Path -Parent $PROFILE
    if (-not (Test-Path $profileDir)) {
        New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
    }
    if (-not (Test-Path $PROFILE)) {
        New-Item -ItemType File -Path $PROFILE -Force | Out-Null
    }

    $line = ". `"$completionTarget`""
    if (-not (Select-String -Path $PROFILE -SimpleMatch $line -Quiet)) {
        Add-Content -Path $PROFILE -Value "`n$line"
    }

    Write-Host "PowerShell completion installed via $PROFILE."
}

$defaultInstallDir = Join-Path $env:LOCALAPPDATA 'Programs\hxed'
$installDir = Read-Value 'Install hxed into which directory?' $defaultInstallDir
New-Item -ItemType Directory -Path $installDir -Force | Out-Null

$installMode = Read-InstallMode
$targetExe = Join-Path $installDir 'hxed.exe'

if ($installMode -eq 'build') {
    if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt'))) {
        throw 'Build mode requires a repository checkout with CMakeLists.txt next to this script.'
    }

    Require-Command 'cmake'

    Write-Host 'Configuring Release build...'
    & cmake -S $RepoRoot -B (Join-Path $RepoRoot 'build') -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        throw 'CMake configure failed.'
    }

    Write-Host 'Building hxed...'
    & cmake --build (Join-Path $RepoRoot 'build') --config Release
    if ($LASTEXITCODE -ne 0) {
        throw 'Build failed.'
    }

    Copy-Item (Resolve-BuiltBinary) $targetExe -Force
}
else {
    $releaseUrl = "$RepoUrl/releases/latest/download/hxed-windows-x64.exe"
    Write-Host 'Downloading latest release binary...'
    Invoke-WebRequest -Uri $releaseUrl -OutFile $targetExe
}

Ensure-UserPath -InstallDir $installDir

if (Read-YesNo 'Install PowerShell completion as well?' $true) {
    Install-PowerShellCompletion -InstallDir $installDir
}

Write-Host ''
Write-Host "hxed was installed to $targetExe"
