param(
    [string]$ProjectPath = "",
    [string]$ExportDir = "",
    [switch]$SkipExport,
    [string]$BaselinePath = "Plugins/BlueprintSerializer/REGRESSION_BASELINE.json",
    [string]$EngineRoot = "C:/Program Files/Epic Games/UE_5.6",
    [int]$TimeoutSeconds = 1800,
    [bool]$KillOnTimeout = $true
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-AbsolutePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue,
        [Parameter(Mandatory = $true)]
        [string]$BaseDir
    )

    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return ""
    }

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return [System.IO.Path]::GetFullPath($PathValue)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BaseDir $PathValue))
}

function Convert-ToProjectRelativeCommandPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$AbsolutePath,
        [Parameter(Mandatory = $true)]
        [string]$ProjectRoot
    )

    $fullPath = [System.IO.Path]::GetFullPath($AbsolutePath)
    $fullRoot = [System.IO.Path]::GetFullPath($ProjectRoot)

    if (-not $fullRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $fullRoot = $fullRoot + [System.IO.Path]::DirectorySeparatorChar
    }

    $comparison = [System.StringComparison]::OrdinalIgnoreCase
    if ($fullPath.StartsWith($fullRoot, $comparison)) {
        $rootUri = New-Object System.Uri($fullRoot)
        $pathUri = New-Object System.Uri($fullPath)
        $relativeUri = $rootUri.MakeRelativeUri($pathUri)
        $relative = [System.Uri]::UnescapeDataString($relativeUri.ToString())
        return ($relative -replace "\\", "/")
    }

    return ($fullPath -replace "\\", "/")
}

$scriptDir = $PSScriptRoot
$pluginDir = Split-Path -Parent $scriptDir
$pluginsDir = Split-Path -Parent $pluginDir
$projectDir = Split-Path -Parent $pluginsDir

if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $uproject = Get-ChildItem -Path $projectDir -Filter "*.uproject" | Select-Object -First 1
    if (-not $uproject) {
        throw "Could not auto-detect .uproject under $projectDir"
    }
    $ProjectPath = $uproject.FullName
}

$ProjectPath = Resolve-AbsolutePath -PathValue $ProjectPath -BaseDir $projectDir
$BaselinePath = Resolve-AbsolutePath -PathValue $BaselinePath -BaseDir $projectDir

if (-not (Test-Path -Path $ProjectPath -PathType Leaf)) {
    throw "Project path does not exist: $ProjectPath"
}

if (-not (Test-Path -Path $BaselinePath -PathType Leaf)) {
    throw "Baseline path does not exist: $BaselinePath"
}

if ([string]::IsNullOrWhiteSpace($ExportDir)) {
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $ExportDir = "Saved/BlueprintExports/BP_SLZR_All_$timestamp"
}

$ExportDir = Resolve-AbsolutePath -PathValue $ExportDir -BaseDir $projectDir
$exportsRoot = Join-Path $projectDir "Saved/BlueprintExports"

if (-not (Test-Path -Path $exportsRoot -PathType Container)) {
    New-Item -ItemType Directory -Path $exportsRoot | Out-Null
}

$editorCmd = Join-Path $EngineRoot "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
if (-not (Test-Path -Path $editorCmd -PathType Leaf)) {
    throw "UnrealEditor-Cmd not found: $editorCmd"
}

$skipText = if ($SkipExport.IsPresent) { "true" } else { "false" }
$exportArg = Convert-ToProjectRelativeCommandPath -AbsolutePath $ExportDir -ProjectRoot $projectDir
$baselineArg = Convert-ToProjectRelativeCommandPath -AbsolutePath $BaselinePath -ProjectRoot $projectDir
$execCmd = "BP_SLZR.RunRegressionSuite $exportArg $skipText $baselineArg;Quit"
$argString = '"' + $ProjectPath + '" -nullrhi -unattended -nop4 -nosplash -ExecCmds="' + $execCmd + '"'

Write-Host "[Regression] ProjectPath: $ProjectPath"
Write-Host "[Regression] ExportDir:   $ExportDir"
Write-Host "[Regression] Baseline:    $BaselinePath"
Write-Host "[Regression] SkipExport:  $skipText"
Write-Host "[Regression] TimeoutSec:  $TimeoutSeconds"

$startTime = Get-Date
$proc = $null
try {
    $proc = Start-Process -FilePath $editorCmd -ArgumentList $argString -PassThru -WindowStyle Hidden

    Write-Host "[Regression] Launched UnrealEditor-Cmd PID=$($proc.Id)"

    $deadline = $startTime.AddSeconds($TimeoutSeconds)
    $suiteReportPath = ""

    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2

        $latestReport = Get-ChildItem -Path $exportsRoot -Filter "BP_SLZR_RegressionRun_*.json" -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1

        if ($null -ne $latestReport -and $latestReport.LastWriteTime -ge $startTime.AddSeconds(-10)) {
            $suiteReportPath = $latestReport.FullName
            Write-Host "[Regression] Detected suite report: $suiteReportPath"
            break
        }

        if ($proc.HasExited) {
            break
        }
    }

    if ([string]::IsNullOrWhiteSpace($suiteReportPath)) {
        $candidateAfterExit = Get-ChildItem -Path $exportsRoot -Filter "BP_SLZR_RegressionRun_*.json" -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1
        if ($null -ne $candidateAfterExit -and $candidateAfterExit.LastWriteTime -ge $startTime.AddSeconds(-10)) {
            $suiteReportPath = $candidateAfterExit.FullName
            Write-Host "[Regression] Found suite report after process exit: $suiteReportPath"
        }
    }

    if ([string]::IsNullOrWhiteSpace($suiteReportPath)) {
        if (-not $proc.HasExited -and $KillOnTimeout) {
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
            Write-Host "[Regression] Timeout reached; force-closed UnrealEditor-Cmd PID=$($proc.Id)"
        }
        throw "Regression suite timed out or produced no suite report within $TimeoutSeconds seconds"
    }

    $suite = Get-Content -Path $suiteReportPath -Raw | ConvertFrom-Json

    $suitePass = $false
    if ($null -ne $suite.suitePass) {
        $suitePass = [bool]$suite.suitePass
    }

    Write-Host "[Regression] suitePass=$suitePass"
    Write-Host "[Regression] validationReportPath=$($suite.validationReportPath)"
    Write-Host "[Regression] curveAuditReportPath=$($suite.curveAuditReportPath)"

    if (-not $suitePass) {
        if ($null -ne $suite.failures) {
            foreach ($failure in $suite.failures) {
                Write-Host "[Regression] failure: $failure"
            }
        }
        exit 2
    }

    exit 0
}
finally {
    if ($null -ne $proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        Write-Host "[Regression] Closed UnrealEditor-Cmd PID=$($proc.Id)"
    }
}
