# Shared by launcher workers from different extracted preview directories.
# Ownership remains on this worker's thread until exit/profile checks finish.
function Enter-CoopLaunchGuard {
    $sid=[Security.Principal.WindowsIdentity]::GetCurrent().User.Value
    $mutex=New-Object Threading.Mutex($false,('Local\MafiaIICoop.Session.'+$sid))
    $owned=$false
    try {
        try {$owned=$mutex.WaitOne(0)}
        catch [Threading.AbandonedMutexException] {$owned=$true}
        if(!$owned -or @(Get-Process mafia2 -ErrorAction SilentlyContinue).Count) {
            $failure=New-Object InvalidOperationException('Another co-op worker or Mafia II process is already running.')
            $failure.Data['CoopFailureCode']='session_busy'
            throw $failure
        }
        return [pscustomobject]@{mutex=$mutex;owned=$true}
    } catch {
        if($owned){$mutex.ReleaseMutex()}
        $mutex.Dispose()
        throw
    }
}
function Exit-CoopLaunchGuard($Guard) {
    if(!$Guard){return}
    try {if($Guard.owned){$Guard.mutex.ReleaseMutex();$Guard.owned=$false}}
    finally {$Guard.mutex.Dispose()}
}
