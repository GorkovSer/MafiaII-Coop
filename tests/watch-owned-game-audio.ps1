param([Parameter(Mandatory=$true)][string]$PreviewRoot,
      [Parameter(Mandatory=$true)][string]$RunDirectory,
      [switch]$PreflightOnly)
$ErrorActionPreference='Stop'
# A Windows PowerShell child can inherit PS7's incompatible PSModulePath.
# Load the matching inbox modules before advertising readiness for game launch.
foreach($module in @('Microsoft.PowerShell.Utility','Microsoft.PowerShell.Management')) {
    Import-Module (Join-Path $PSHOME ('Modules/'+$module+'/'+$module+'.psd1')) -ErrorAction Stop
}
foreach($command in @('Get-FileHash','ConvertFrom-Json','Get-Content','Select-String','Get-Process','Test-Path','Start-Sleep')) {
    [void](Get-Command $command -ErrorAction Stop)
}
. (Join-Path $PSScriptRoot 'mute-owned-game.ps1') -InitializeOnly
if($PreflightOnly){Write-Output 'PASS owned audio watcher dependencies and CoreAudio initialization';return}
# Signal only after CoreAudio interop has compiled, before the game is started.
[IO.File]::WriteAllText((Join-Path $RunDirectory 'audio-watcher-ready.txt'),'ready')
$watch=[Diagnostics.Stopwatch]::StartNew()
$game=$null
while($watch.Elapsed.TotalSeconds -lt 60) {
    $link=Join-Path $RunDirectory 'session-info.json'
    if(Test-Path -LiteralPath $link) {
        $info=Get-Content -LiteralPath $link -Raw | ConvertFrom-Json
        if($info.schema -ne 1 -or $info.session_id -notmatch '^\d{8}-\d{6}-\d{3}$'){throw 'Invalid session link'}
        $log=Join-Path $PreviewRoot "work/probe-sessions/$($info.session_id)/probe.log"
        if(Test-Path -LiteralPath $log) {
            $match=Get-Content -LiteralPath $log -TotalCount 4 | Select-String '^process_id=(\d+)$'
            if($match) {$game=Get-Process -Id ([int]$match.Matches[0].Groups[1].Value) -ErrorAction SilentlyContinue}
        }
    }
    if($game){break}
    Start-Sleep -Milliseconds 20
}
if(!$game){throw 'No owned game appeared; no audio changed'}
if((Get-FileHash -LiteralPath $game.Path).Hash.ToLowerInvariant() -ne '16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f' -or
    $game.StartTime.ToUniversalTime() -lt [DateTime]::UtcNow.AddMinutes(-2)){throw 'Not a fresh pinned game'}
$muted=0
while(!$game.HasExited -and $watch.Elapsed.TotalMinutes -lt 30) {
    $muted=[CoopAudioMute]::Apply([uint32]$game.Id)
    if($muted -gt 0 -and !(Test-Path (Join-Path $RunDirectory 'audio-muted.txt'))) {
        [IO.File]::WriteAllText((Join-Path $RunDirectory 'audio-muted.txt'),"pid=$($game.Id) sessions=$muted")
    }
    Start-Sleep -Milliseconds 20
}
Write-Output "Owned audio watcher finished pid=$($game.Id) last_sessions=$muted"
