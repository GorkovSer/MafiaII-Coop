# Evidence is emitted only after the owned launcher's native process wait.
# Missing or duplicate child-exit lines stay unknown; never infer a zero exit.
function Get-CoopLaunchResult {
    param([AllowEmptyString()][string]$LauncherOutput, [int]$LauncherExit,
          [bool]$WaitedForExit, [int]$OriginalProfileChanges)
    $exitMatch = [regex]::Matches($LauncherOutput, '(?m)^child_exit=(\d+)\r?$')
    $childExit = if($exitMatch.Count -eq 1) {[long]::Parse($exitMatch[0].Groups[1].Value)} else {$null}
    return [pscustomobject]@{schema=1; launcher_exit=$LauncherExit; waited_for_exit=$WaitedForExit;
        child_exit_code=$childExit; original_profile_changed_files=$OriginalProfileChanges}
}
