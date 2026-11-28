$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'launcher-files.ps1')
$root=Split-Path $PSScriptRoot -Parent
$directory=Join-Path $root ('work/launcher-files-tests/'+[Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($directory)
$path=Join-Path $directory 'metadata.json'
$utf8=New-Object Text.UTF8Encoding($false)
function Reject-Read([string]$Reason,[int]$Limit=8192) {
    $rejected=$false
    # The reader must fail closed even when the caller allows nonterminating errors.
    $ErrorActionPreference='Continue'
    try {Read-CoopRunJson $path $Limit | Out-Null} catch {$rejected=$true}
    if(!$rejected){throw "Accepted invalid metadata: $Reason"}
}
foreach($bom in @($false,$true)) {
    [IO.File]::WriteAllText($path,'{"schema":1,"label":"valid"}',(New-Object Text.UTF8Encoding($bom)))
    $value=Read-CoopRunJson $path
    if($value.schema -ne 1 -or $value.label -cne 'valid'){throw 'UTF-8 roundtrip failed'}
}
Write-Output 'PASS UTF-8 with and without BOM'
foreach($bad in @('', ' ', 'null', '[]', '[{"schema":1}]', '1', 'true', '"text"', '{', '{"schema":')) {
    [IO.File]::WriteAllText($path,$bad,$utf8)
    Reject-Read 'empty, malformed or non-object JSON'
}
[IO.File]::WriteAllBytes($path,[byte[]]@(123,34,120,34,58,34,195,40,34,125))
Reject-Read 'invalid UTF-8'
Write-Output 'PASS empty/malformed/non-object JSON and invalid UTF-8 fail closed'
$exact='{"schema":1}'
[IO.File]::WriteAllText($path,$exact,$utf8)
if((Read-CoopRunJson $path $exact.Length).schema -ne 1){throw 'Exact limit refused'}
Reject-Read 'one byte over limit' ($exact.Length-1)
Reject-Read 'zero limit' 0
Reject-Read 'oversized limit' 4194305
Write-Output 'PASS exact byte limit and invalid limits'
Write-CoopRunJson $path @{schema=1;generation=1}
$handle=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,
    ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
try {
    Write-CoopRunJson $path @{schema=1;generation=2}
    if((Read-CoopRunJson $path).generation -ne 2){throw 'Atomic replacement failed'}
} finally {$handle.Dispose()}
if(@(Get-ChildItem -LiteralPath $directory -Filter '*.tmp').Count){throw 'Temporary file leaked'}
# Replacement failure must preserve the previous destination and clean its temp.
$handle=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
try {
    $rejected=$false
    try {Write-CoopRunJson $path @{schema=1;generation=3}} catch {$rejected=$true}
    if(!$rejected){throw 'Replacement unexpectedly bypassed destination sharing lock'}
} finally {$handle.Dispose()}
if((Read-CoopRunJson $path).generation -ne 2){throw 'Failed write destroyed old destination'}
if(@(Get-ChildItem -LiteralPath $directory -Filter '*.tmp').Count){throw 'Failed write leaked temp'}
Write-Output 'PASS atomic replacement with reader and preservation on denied replacement'
Write-Output "Fixtures=$directory"
