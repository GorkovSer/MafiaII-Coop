param([string]$BundlePath, [string]$OutputRoot)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!$BundlePath){$BundlePath=Join-Path $root 'testdata/story-save.zip'}
if(!$OutputRoot){$OutputRoot=Join-Path $root 'work/probe-sessions'}
if(Get-Process mafia2 -ErrorAction SilentlyContinue){throw 'Exit Mafia II normally before installing a test profile'}
$manifest=Get-Content -LiteralPath (Join-Path $root 'testdata/story-save.json') -Raw | ConvertFrom-Json
if((Get-FileHash -LiteralPath $BundlePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.sha256){throw 'Save archive hash mismatch'}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$destination=Join-Path $OutputRoot $stamp
if(Test-Path -LiteralPath $destination){throw 'Destination already exists; no overwrite performed'}
$profileParent=Join-Path $destination 'profile-localappdata/2K Games'
New-Item -ItemType Directory -Path $profileParent -Force | Out-Null
Expand-Archive -LiteralPath $BundlePath -DestinationPath $profileParent
$profile=Join-Path $profileParent 'Mafia II'
$id=([IO.File]::ReadAllText((Join-Path $profile 'Saves/last.dat'))).Trim()
if($id -notmatch '^\d{18}$'){throw 'Unexpected save profile format'}
$files=@(Get-ChildItem -LiteralPath $profile -Recurse -File)
if($files.Count -ne $manifest.file_count -or !(Test-Path -LiteralPath (Join-Path $profile "Saves/$id/sav1.dat"))){throw 'Incomplete save bundle'}
[pscustomobject]@{schema=1;archive_sha256=$manifest.sha256;original_profile_written=$false} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destination 'save-install.json')
Write-Output "Installed isolated save. TestProfileFrom=$stamp"
Write-Output 'Original LOCALAPPDATA profile was not modified. Use this ID with start-probe.ps1 -TestProfileFrom.'
