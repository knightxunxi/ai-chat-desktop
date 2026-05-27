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
    $readyFile = Join-Path $tempRoot "smoke-ready.txt"
    New-Item -ItemType Directory -Force -Path $env:APPDATA, $env:LOCALAPPDATA | Out-Null

    $process = Start-Process -FilePath $exe -ArgumentList @("--smoke-test", "--smoke-test-ready-file", $readyFile) -WorkingDirectory $workingDirectory -PassThru

    $windowReady = $false
    for ($i = 0; $i -lt 80; $i++) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()

        if ($process.HasExited) {
            throw "Application exited before a main window was available. ExitCode=$($process.ExitCode)"
        }

        if (Test-Path -LiteralPath $readyFile) {
            $windowReady = $true
            break
        }
    }

    if (-not $windowReady) {
        throw "Application main window was not detected. PID=$($process.Id)"
    }

    for ($i = 0; $i -lt 80; $i++) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()

        if ($process.HasExited) {
            if ($process.ExitCode -ne 0) {
                throw "Application exited with code $($process.ExitCode). PID=$($process.Id)"
            }

            Write-Output "App launch/close smoke test passed. PID=$($process.Id)"
            exit 0
        }
    }

    throw "Application was still running after smoke-test close timeout. PID=$($process.Id)"
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
