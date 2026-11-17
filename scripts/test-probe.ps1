$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root 'build/probe'
$dir = Join-Path $root ('work/loader-test/' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$launcher = Join-Path $out 'probe_launcher.exe'
$hostExe = Join-Path $out 'probe_host.exe'
$dll = Join-Path $out 'coop_probe.dll'
$log = Join-Path $dir 'probe.log'
$result = (& $launcher $hostExe $dll $log --wait) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Real x86 DLL loading failed' }
$fields = Get-Content -LiteralPath $log | ConvertFrom-StringData
if ($fields.pointer_bits -ne '32' -or $fields.pe_machine -ne '014c' -or
    $fields.status -ne 'loaded_inside_process' -or $fields.game_patches -ne '0') {
    throw 'Unexpected in-process evidence'
}
if ($result -notmatch "pid=$($fields.process_id) ") { throw 'PID evidence mismatch' }
if ($result -notmatch 'child_exit=0') { throw 'Test host did not exit normally' }
Write-Output 'PASS real x86 load, explicit export invocation, matching PID and log'
Write-Output 'PASS native process handle reports normal child exit'
$before = (Get-FileHash -LiteralPath $log).Hash
& $launcher $hostExe $dll $log 2>$null
if ($LASTEXITCODE -ne 4 -or (Get-FileHash -LiteralPath $log).Hash -ne $before) {
    throw 'Existing evidence was not protected'
}
Write-Output 'PASS refusal to overwrite existing log'
& $launcher $hostExe (Join-Path $dir 'missing.dll') (Join-Path $dir 'missing.log') 2>$null
if ($LASTEXITCODE -ne 5 -or (Test-Path -LiteralPath (Join-Path $dir 'missing.log'))) {
    throw 'Missing DLL should fail before target launch'
}
Write-Output 'PASS missing DLL rejected before target launch'
