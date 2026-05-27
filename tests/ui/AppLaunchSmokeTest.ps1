param(
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $ExecutablePath)) {
    throw "Executable was not found: $ExecutablePath"
}

$exe = (Resolve-Path -LiteralPath $ExecutablePath).Path
$workingDirectory = Split-Path -Parent $exe
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("AIChatDesktopSmoke-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

$oldAppData = $env:APPDATA
$oldLocalAppData = $env:LOCALAPPDATA
$process = $null

try {
    $env:APPDATA = Join-Path $tempRoot "Roaming"
    $env:LOCALAPPDATA = Join-Path $tempRoot "Local"
    New-Item -ItemType Directory -Force -Path $env:APPDATA, $env:LOCALAPPDATA | Out-Null

    $process = Start-Process -FilePath $exe -WorkingDirectory $workingDirectory -PassThru

    $windowReady = $false
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()

        if ($process.HasExited) {
            throw "Application exited before a main window was available. ExitCode=$($process.ExitCode)"
        }

        if ($process.MainWindowHandle -ne 0) {
            $windowReady = $true
            break
        }
    }

    if (-not $windowReady) {
        throw "Application main window was not detected. PID=$($process.Id)"
    }

    if (-not $process.CloseMainWindow()) {
        throw "CloseMainWindow returned false. PID=$($process.Id)"
    }

    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()

        if ($process.HasExited) {
            Write-Output "App launch/close smoke test passed. PID=$($process.Id)"
            exit 0
        }
    }

    throw "Application was still running after the main window was closed. PID=$($process.Id)"
}
finally {
    if ($process -ne $null) {
        $process.Refresh()
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
        }
    }

    $env:APPDATA = $oldAppData
    $env:LOCALAPPDATA = $oldLocalAppData

    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
