# Bind packaged native artifacts to the source bytes actually present during build.
function Get-CoopBuildInputs([string]$Root,[ValidateSet('runtime','frontend')][string]$Kind) {
    $boundary=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    $directories=if($Kind -eq 'runtime') {@('src','third_party/enet')} else {@('src/launcher')}
    $paths=@(foreach($directory in $directories) {
        Get-ChildItem -LiteralPath (Join-Path $Root $directory) -Recurse -File |
            Where-Object {$_.Extension -in @('.c','.h','.inc','.def','.rc','.manifest')} |
            ForEach-Object {
                $relative=$_.FullName.Substring($boundary.Length).Replace('\','/')
                # probe_launcher/window/user_process build the runtime launcher.
                # Only the separate GUI frontend belongs exclusively to frontend.
                if($Kind -ne 'runtime' -or $relative -notin @(
                    'src/launcher/frontend.c','src/launcher/frontend.manifest')) {$relative}
            }
    })
    $paths+='scripts/build-provenance.ps1'
    if($Kind -eq 'runtime') {$paths+=@('scripts/build-probe.ps1','scripts/build-enet.ps1')}
    else {$paths+='scripts/build-launcher.ps1'}
    return @(foreach($path in @($paths|Sort-Object -Unique)) {
        [pscustomobject]@{path=$path;sha256=(Get-FileHash -LiteralPath (Join-Path $Root $path)).Hash.ToLowerInvariant()}
    })
}
function Assert-CoopBuildInputs($Expected,$Actual) {
    if(@($Expected).Count -ne @($Actual).Count){throw 'Native build inputs changed; rebuild before packaging'}
    for($i=0;$i -lt @($Actual).Count;$i++) {
        if($Expected[$i].path -cne $Actual[$i].path -or $Expected[$i].sha256 -cne $Actual[$i].sha256) {
            throw 'Native build inputs changed; rebuild before packaging'
        }
    }
}
function Start-CoopBuildProvenance([string]$Root,[string]$OutputDirectory,[string]$Kind) {
    $inputs=@(Get-CoopBuildInputs $Root $Kind)
    $record=[pscustomobject]@{schema=1;kind=$Kind;complete=$false;inputs=$inputs;artifacts=@()}
    $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'build-provenance.json') -Encoding UTF8
    return $record
}
function Complete-CoopBuildProvenance([string]$Root,[string]$OutputDirectory,$Record,[string[]]$Artifacts) {
    Assert-CoopBuildInputs $Record.inputs @(Get-CoopBuildInputs $Root $Record.kind)
    $Record.artifacts=@(foreach($name in $Artifacts) {
        [pscustomobject]@{name=$name;sha256=(Get-FileHash -LiteralPath (Join-Path $OutputDirectory $name)).Hash.ToLowerInvariant()}
    })
    $Record.complete=$true
    $path=Join-Path $OutputDirectory 'build-provenance.json'
    $Record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath ($path+'.tmp') -Encoding UTF8
    Move-Item -LiteralPath ($path+'.tmp') -Destination $path -Force
}
function Assert-CoopBuildProvenance([string]$Root,[string]$OutputDirectory,[string]$Kind,[string[]]$Artifacts) {
    $path=Join-Path $OutputDirectory 'build-provenance.json'
    if(!(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -gt 4MB) {
        throw 'Native build provenance is missing; rebuild with the current build scripts'
    }
    $record=Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json
    if($record.schema -ne 1 -or $record.kind -ne $Kind -or $record.complete -ne $true) {
        throw 'Native build did not complete successfully; rebuild before packaging'
    }
    Assert-CoopBuildInputs $record.inputs @(Get-CoopBuildInputs $Root $Kind)
    foreach($name in $Artifacts) {
        $entry=@($record.artifacts | Where-Object {$_.name -ceq $name})
        if($entry.Count -ne 1 -or
            (Get-FileHash -LiteralPath (Join-Path $OutputDirectory $name)).Hash.ToLowerInvariant() -cne $entry[0].sha256) {
            throw 'Native artifact differs from its completed build record; rebuild before packaging'
        }
    }
    return $record
}
