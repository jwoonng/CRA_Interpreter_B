<#
.SYNOPSIS
    CodeFab Demo Runner

.DESCRIPTION
    Feeds demo scripts line by line to factory.exe with timed delays.

.PARAMETER Mode
    repl  : Feed lines to REPL one by one
    run   : Print source then execute via file mode
    debug : Feed debugger commands one by one

.PARAMETER File
    Script file (.txt for repl, .cb for run/debug)

.PARAMETER Commands
    [debug only] Debugger command file (.txt)

.PARAMETER Delay
    Milliseconds between lines (default: 1000)

.PARAMETER Auto
    If set, feeds lines automatically with Delay interval instead of waiting for Enter

.EXAMPLE
    .\demo_runner.ps1 -Mode repl  -File demo1_repl.txt
    .\demo_runner.ps1 -Mode run   -File demo2_algorithm.cb
    .\demo_runner.ps1 -Mode run   -File demo3_error.cb
    .\demo_runner.ps1 -Mode debug -File demo4_debug.cb -Commands demo4_commands.txt
    .\demo_runner.ps1 -Mode debug -File demo4_debug.cb -Commands demo4_commands.txt -Auto
#>
param(
    [Parameter(Mandatory)][ValidateSet("repl","run","debug")][string]$Mode,
    [Parameter(Mandatory)][string]$File,
    [string]$Commands = "",
    [int]$Delay = 1000,
    [switch]$Auto
)

$ErrorActionPreference = "Stop"

$exe      = Join-Path $PSScriptRoot "factory.exe"
$filePath = Join-Path $PSScriptRoot $File

if (-not (Test-Path $exe)) {
    Write-Host "[ERROR] factory.exe not found: $exe" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path $filePath)) {
    Write-Host "[ERROR] Script not found: $filePath" -ForegroundColor Red
    exit 1
}

# ── Output queue shared with event handler ──────────────────────────
$script:outQueue    = [System.Collections.Concurrent.ConcurrentQueue[string]]::new()
$script:stdinWriter = $null
$asciiEncoding      = [System.Text.Encoding]::ASCII

function Drain-Output {
    $item = $null
    while ($script:outQueue.TryDequeue([ref]$item)) { }
}

function Flush-Output {
    $item = $null
    while ($script:outQueue.TryDequeue([ref]$item)) {
        # Strip accumulated REPL/debug prompts ("> ", "... ") from the front
        $clean = ($item -replace '^(\s*\.{3}\s*|\s*>\s*)+', '').TrimStart()
        if ($clean) {
            Write-Host "  $clean" -ForegroundColor Cyan
        }
    }
}

function New-FactoryProcess ([string]$Arguments = "") {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName               = $exe
    if ($Arguments) { $psi.Arguments = $Arguments }
    $psi.RedirectStandardInput  = $true
    $psi.RedirectStandardOutput = $true
    $psi.UseShellExecute        = $false

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    $proc.Start() | Out-Null

    # Use ASCII for stdin — no BOM, all demo content is ASCII
    $script:stdinWriter = New-Object System.IO.StreamWriter($proc.StandardInput.BaseStream, $asciiEncoding)
    $script:stdinWriter.AutoFlush = $true
    # Send one silent empty line to consume any encoding preamble, then drain output silently
    $script:stdinWriter.WriteLine("")
    Start-Sleep -Milliseconds 600
    Drain-Output

    Register-ObjectEvent -InputObject $proc -EventName "OutputDataReceived" -Action {
        if ($null -ne $EventArgs.Data) { $Event.MessageData.Enqueue($EventArgs.Data) }
    } -MessageData $script:outQueue | Out-Null

    $proc.BeginOutputReadLine()
    return $proc
}

function Wait-Enter {
    do { $key = [Console]::ReadKey($true) } while ($key.Key -ne [ConsoleKey]::Enter)
}

function Send-Lines ([System.Diagnostics.Process]$proc, [string[]]$lines, [bool]$ManualMode = $false) {
    foreach ($line in $lines) {
        if ($ManualMode) {
            Wait-Enter
            if ($line -notmatch '^\s*$') {
                Write-Host "> " -NoNewline -ForegroundColor Green
                Write-Host $line -ForegroundColor White
            }
            $script:stdinWriter.WriteLine($line)
            Start-Sleep -Milliseconds 400
            Flush-Output
        } else {
            Start-Sleep -Milliseconds 300
            Flush-Output
            if ($line -match '^\s*$') {
                Write-Host ""
            } else {
                Write-Host "> " -NoNewline -ForegroundColor Green
                Write-Host $line -ForegroundColor White
            }
            $script:stdinWriter.WriteLine($line)
            Start-Sleep -Milliseconds $Delay
        }
    }

    $script:stdinWriter.Close()
    $proc.WaitForExit(5000) | Out-Null
    Start-Sleep -Milliseconds 400
    Flush-Output
}

$divider = "-" * 42

switch ($Mode) {

    "run" {
        Write-Host ""
        Write-Host "[ Source: $File ]" -ForegroundColor Yellow
        Write-Host $divider -ForegroundColor DarkGray
        [System.IO.File]::ReadAllLines($filePath, (New-Object System.Text.UTF8Encoding $false)) | ForEach-Object { Write-Host "  $_" -ForegroundColor DarkGray }
        Write-Host $divider -ForegroundColor DarkGray
        Start-Sleep -Seconds 1
        Write-Host ""
        Write-Host "Running: factory run $File" -ForegroundColor Green
        Write-Host ""
        & $exe run $filePath
        Write-Host ""
    }

    "repl" {
        Write-Host ""
        Write-Host "[ CodeFab REPL ]" -ForegroundColor Yellow
        Write-Host $divider -ForegroundColor DarkGray
        $proc = New-FactoryProcess
        Start-Sleep -Milliseconds 700
        Drain-Output   # Discard initialization noise (BOM flush, prompts)
        Send-Lines $proc ([System.IO.File]::ReadAllLines($filePath, [System.Text.Encoding]::ASCII)) (-not $Auto.IsPresent)
        Write-Host ""
    }

    "debug" {
        if (-not $Commands) {
            Write-Host "[ERROR] -Commands is required for debug mode." -ForegroundColor Red
            exit 1
        }
        $cmdPath = Join-Path $PSScriptRoot $Commands
        if (-not (Test-Path $cmdPath)) {
            Write-Host "[ERROR] Commands file not found: $cmdPath" -ForegroundColor Red
            exit 1
        }

        Write-Host ""
        Write-Host "[ CodeFab Debugger: $File ]" -ForegroundColor Yellow
        Write-Host $divider -ForegroundColor DarkGray
        $debugArgs = "debug ""$filePath"""
        $proc = New-FactoryProcess $debugArgs
        Start-Sleep -Milliseconds 700
        Drain-Output   # Discard BOM flush noise
        Flush-Output   # Show [DEBUG] loaded source message
        Send-Lines $proc ([System.IO.File]::ReadAllLines($cmdPath, [System.Text.Encoding]::ASCII)) (-not $Auto.IsPresent)
        Write-Host ""
    }
}
