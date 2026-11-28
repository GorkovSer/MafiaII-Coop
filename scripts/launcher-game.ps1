# Inspect known installation locations only; never recursively crawl a drive.
function Resolve-CoopGameDirectory([string]$Path) {
    if([string]::IsNullOrWhiteSpace($Path)){return $null}
    try {$candidate=[IO.Path]::GetFullPath($Path.Trim().Trim('"')).TrimEnd('\','/')} catch {return $null}
    if(Test-Path -LiteralPath $candidate -PathType Leaf) {
        if([IO.Path]::GetFileName($candidate) -ine 'mafia2.exe'){return $null}
        $candidate=Split-Path $candidate -Parent
    }
    if([IO.Path]::GetFileName($candidate.TrimEnd('\','/')) -ieq 'pc' -and
        (Test-Path -LiteralPath (Join-Path $candidate 'mafia2.exe') -PathType Leaf)) {
        $candidate=Split-Path $candidate -Parent
    }
    if(Test-Path -LiteralPath (Join-Path $candidate 'pc/mafia2.exe') -PathType Leaf){return $candidate}
    return $null
}
function Get-CoopSteamLibraryRoots([string]$SteamRoot) {
    if(!$SteamRoot){return}
    $SteamRoot
    $vdf=Join-Path $SteamRoot 'steamapps/libraryfolders.vdf'
    if(!(Test-Path -LiteralPath $vdf) -or (Get-Item -LiteralPath $vdf).Length -gt 1MB){return}
    foreach($match in [regex]::Matches([IO.File]::ReadAllText($vdf),'"path"\s+"([^"\r\n]+)"')) {
        $match.Groups[1].Value.Replace('\\','\')
    }
}
function Find-CoopGameDirectories([string[]]$Candidates) {
    if(!$Candidates) {
        $known=New-Object 'System.Collections.Generic.List[string]'
        foreach($drive in [IO.DriveInfo]::GetDrives()) {
            if($drive.DriveType -eq 'Fixed' -and $drive.IsReady) {
                foreach($name in @('Mafia II Classic','Mafia II','Mafia 2')) {
                    $known.Add((Join-Path $drive.RootDirectory.FullName "Games/$name"))
                }
            }
        }
        $steamRoots=@()
        foreach($key in @('HKCU:/Software/Valve/Steam','HKLM:/SOFTWARE/WOW6432Node/Valve/Steam')) {
            $entry=Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
            if($entry.SteamPath){$steamRoots+=$entry.SteamPath}
            if($entry.InstallPath){$steamRoots+=$entry.InstallPath}
        }
        foreach($base in @(${env:ProgramFiles(x86)},$env:ProgramFiles)) {
            if($base){$steamRoots+=(Join-Path $base Steam)}
        }
        foreach($steam in @($steamRoots|Select-Object -Unique)) {
            foreach($library in @(Get-CoopSteamLibraryRoots $steam)) {
                foreach($name in @('Mafia II','Mafia II Classic')) {
                    $known.Add((Join-Path $library "steamapps/common/$name"))
                }
            }
        }
        $Candidates=$known.ToArray()
    }
    $found=New-Object 'System.Collections.Generic.List[string]'
    foreach($candidate in $Candidates) {
        $resolved=Resolve-CoopGameDirectory $candidate
        if($resolved -and !@($found | Where-Object {$_ -ieq $resolved}).Count){$found.Add($resolved)}
    }
    return $found.ToArray()
}
