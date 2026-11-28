param([string]$BuildDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-diagnostics.ps1')
. (Join-Path $PSScriptRoot 'launcher-status.ps1')
trap {
    [void](Write-CoopFailureReport -Root $root -Failure $_ -Kind 'launcher-unhandled')
    break
}
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()
$settingsPath=Join-Path $root 'work/launcher-settings.json'
$crashDirectory=Join-Path $root 'work/crashlogs'
$script:sessionProcess=$null
$script:runDirectory=$null
$background=[Drawing.ColorTranslator]::FromHtml('#111214')
$surface=[Drawing.ColorTranslator]::FromHtml('#242529')
$muted=[Drawing.ColorTranslator]::FromHtml('#b9b9bd')
$accent=[Drawing.ColorTranslator]::FromHtml('#962b39')

function New-Text([string]$text) {
    $label=New-Object Windows.Forms.Label
    $label.Text=$text;$label.AutoSize=$true;$label.ForeColor=[Drawing.Color]::White
    $label.BackColor=[Drawing.Color]::Transparent
    $label.Margin=New-Object Windows.Forms.Padding(0,8,0,5)
    return $label
}
function Style-Input($control) {
    $control.Dock='Fill';$control.BackColor=$surface;$control.ForeColor=[Drawing.Color]::White
    $control.Margin=New-Object Windows.Forms.Padding(0,0,0,8)
}
function Show-Problem([string]$message) {
    $status.Text=$message;$status.ForeColor=[Drawing.ColorTranslator]::FromHtml('#ffa8ad')
}
function Get-BuildDirectory {
    if($BuildDirectory){return $BuildDirectory}
    $manifest=Join-Path $root 'visual-preview-manifest.json'
    if(Test-Path -LiteralPath $manifest) {
        $data=Get-Content -LiteralPath $manifest -Raw -Encoding UTF8 | ConvertFrom-Json
        if(!$data.build_directory){throw 'В архиве отсутствует каталог сборки. Распакуй полный preview заново.'}
        return [string]$data.build_directory
    }
    throw 'Открой лаунчер из полного архива preview. В исходниках каталог сборки нужно указать явно.'
}
function Get-HostAddresses {
    $addresses=@()
    foreach($adapter in [Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces()) {
        if($adapter.OperationalStatus -ne 'Up'){continue}
        foreach($entry in $adapter.GetIPProperties().UnicastAddresses) {
            if($entry.Address.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork -or
               [Net.IPAddress]::IsLoopback($entry.Address)){continue}
            $addresses += [pscustomobject]@{ip=$entry.Address.IPAddressToString;
                priority=[int]($adapter.Name -notmatch 'Radmin' -and $adapter.Description -notmatch 'Radmin')}
        }
    }
    return @($addresses | Sort-Object priority,ip | Select-Object -ExpandProperty ip -Unique)
}

$form=New-Object Windows.Forms.Form
$form.Text='Mafia II Coop — подключение'
$form.ClientSize=New-Object Drawing.Size(700,740)
$form.MinimumSize=New-Object Drawing.Size(690,680)
$form.StartPosition='CenterScreen';$form.BackColor=$background
$form.ForeColor=[Drawing.Color]::White
$form.Font=New-Object Drawing.Font('Segoe UI',10)
$form.AutoScaleMode='Dpi'
$exeIcon=Join-Path $root 'MafiaII-Coop.exe'
if(Test-Path -LiteralPath $exeIcon){try {$form.Icon=[Drawing.Icon]::ExtractAssociatedIcon($exeIcon)} catch {}}
$form.add_Paint({
    param($sender,$paint)
    $bounds=$sender.ClientRectangle
    if($bounds.Width -lt 1 -or $bounds.Height -lt 1){return}
    $shade=New-Object Drawing.Drawing2D.LinearGradientBrush($bounds,
        [Drawing.Color]::FromArgb(37,38,42),$background,125.0)
    $rule=New-Object Drawing.Pen([Drawing.Color]::FromArgb(66,67,72),1)
    $red=New-Object Drawing.SolidBrush($accent)
    try {
        $paint.Graphics.FillRectangle($shade,$bounds)
        $paint.Graphics.FillRectangle($red,0,0,5,$bounds.Height)
        $paint.Graphics.DrawRectangle($rule,15,15,$bounds.Width-31,$bounds.Height-31)
        $paint.Graphics.DrawLine($rule,34,114,$bounds.Width-35,114)
        # Restrained geometric lines rather than borrowed Mafia II assets.
        for($n=0;$n -lt 5;$n++){
            $x=$bounds.Width-110+$n*15
            $paint.Graphics.DrawLine($rule,$x,22,$bounds.Width-22,110-$n*15)
        }
    } finally {$shade.Dispose();$rule.Dispose();$red.Dispose()}
})

$layout=New-Object Windows.Forms.TableLayoutPanel
$layout.Dock='Fill';$layout.AutoScroll=$true
$layout.BackColor=[Drawing.Color]::Transparent
$layout.Padding=New-Object Windows.Forms.Padding(34,24,34,24)
$layout.ColumnCount=1;$layout.RowCount=10
for($i=0;$i -lt 9;$i++){[void]$layout.RowStyles.Add((New-Object Windows.Forms.RowStyle('AutoSize')))}
[void]$layout.RowStyles.Add((New-Object Windows.Forms.RowStyle('Percent',100)))
$form.Controls.Add($layout)
$title=New-Text 'MAFIA II'
$title.Font=New-Object Drawing.Font('Georgia',30,[Drawing.FontStyle]::Bold)
$layout.Controls.Add($title,0,0)
$subtitle=New-Text 'COOPERATIVE  /  ПОДКЛЮЧЕНИЕ'
$subtitle.Font=New-Object Drawing.Font('Segoe UI Semibold',10)
$subtitle.Margin=New-Object Windows.Forms.Padding(0,0,0,24)
$subtitle.ForeColor=$muted;$layout.Controls.Add($subtitle,0,1)

$grid=New-Object Windows.Forms.TableLayoutPanel
$grid.Dock='Top';$grid.AutoSize=$true;$grid.ColumnCount=2;$grid.RowCount=3
$grid.BackColor=[Drawing.Color]::Transparent
[void]$grid.ColumnStyles.Add((New-Object Windows.Forms.ColumnStyle('Absolute',165)))
[void]$grid.ColumnStyles.Add((New-Object Windows.Forms.ColumnStyle('Percent',100)))
$role=New-Object Windows.Forms.ComboBox
$role.DropDownStyle='DropDownList';[void]$role.Items.Add('Хост — создать сессию');[void]$role.Items.Add('Гость — подключиться')
$role.FlatStyle='Flat'
Style-Input $role
$address=New-Object Windows.Forms.ComboBox
$address.DropDownStyle='DropDown';Style-Input $address
$address.FlatStyle='Flat'
$addressLabel=New-Text 'IP хоста в Radmin'
$pathRow=New-Object Windows.Forms.TableLayoutPanel
$pathRow.Dock='Fill';$pathRow.AutoSize=$true;$pathRow.ColumnCount=2
$pathRow.BackColor=[Drawing.Color]::Transparent
[void]$pathRow.ColumnStyles.Add((New-Object Windows.Forms.ColumnStyle('Percent',100)))
[void]$pathRow.ColumnStyles.Add((New-Object Windows.Forms.ColumnStyle('Absolute',42)))
$gamePath=New-Object Windows.Forms.TextBox
$gamePath.Text='C:\Games\Mafia II Classic';Style-Input $gamePath
$browse=New-Object Windows.Forms.Button
$browse.Text='…';$browse.Dock='Fill';$browse.FlatStyle='Flat';$browse.BackColor=$surface
$browse.AccessibleName='Выбрать папку Mafia II Classic'
$pathRow.Controls.Add($gamePath,0,0);$pathRow.Controls.Add($browse,1,0)
$grid.Controls.Add((New-Text 'Твоя роль'),0,0);$grid.Controls.Add($role,1,0)
$grid.Controls.Add($addressLabel,0,1);$grid.Controls.Add($address,1,1)
$grid.Controls.Add((New-Text 'Папка игры'),0,2);$grid.Controls.Add($pathRow,1,2)
$layout.Controls.Add($grid,0,2)
$help=New-Text ''
$help.MaximumSize=New-Object Drawing.Size(620,0);$help.ForeColor=$muted
$layout.Controls.Add($help,0,3)
$scope=New-Text 'В игре оба выбирают «Продолжить». Используется контрольное сохранение; текущий прогресс хоста не передаётся. Общие миссии и бой пока не поддерживаются.'
$scope.MaximumSize=New-Object Drawing.Size(620,0);$scope.ForeColor=$muted
$layout.Controls.Add($scope,0,4)
$start=New-Object Windows.Forms.Button
$start.Dock='Fill';$start.Height=46;$start.FlatStyle='Flat';$start.BackColor=$accent
$start.FlatAppearance.BorderSize=0
$start.FlatAppearance.MouseOverBackColor=[Drawing.ColorTranslator]::FromHtml('#b63b4b')
$start.FlatAppearance.MouseDownBackColor=[Drawing.ColorTranslator]::FromHtml('#6d1924')
$start.Font=New-Object Drawing.Font('Segoe UI Semibold',11)
$start.Cursor=[Windows.Forms.Cursors]::Hand
$start.Margin=New-Object Windows.Forms.Padding(0,15,0,8)
$layout.Controls.Add($start,0,5)
$status=New-Text 'Выбери роль и укажи IP хоста из Radmin VPN.'
$status.MaximumSize=New-Object Drawing.Size(620,0);$status.ForeColor=$muted
$layout.Controls.Add($status,0,6)
$logButtons=New-Object Windows.Forms.FlowLayoutPanel
$logButtons.Dock='Fill';$logButtons.AutoSize=$true;$logButtons.WrapContents=$true
$logButtons.BackColor=[Drawing.Color]::Transparent
$logs=New-Object Windows.Forms.Button
$logs.Text='Папка журнала запуска';$logs.AutoSize=$true;$logs.FlatStyle='Flat'
$logs.BackColor=$surface;$logs.Enabled=$false
$crashes=New-Object Windows.Forms.Button
$crashes.Text='Крашлоги';$crashes.AutoSize=$true;$crashes.FlatStyle='Flat'
$crashes.BackColor=$surface;$crashes.AccessibleName='Открыть папку крашлогов'
foreach($button in @($browse,$logs,$crashes)) {
    $button.FlatAppearance.BorderColor=[Drawing.ColorTranslator]::FromHtml('#55565b')
    $button.FlatAppearance.MouseOverBackColor=[Drawing.Color]::Black
    $button.Cursor=[Windows.Forms.Cursors]::Hand
}
$logButtons.Controls.Add($logs);$logButtons.Controls.Add($crashes)
$layout.Controls.Add($logButtons,0,7)
$closeNote=New-Text 'Закрытие лаунчера не завершает игру. Выходи из Mafia II обычным способом.'
$closeNote.ForeColor=$muted;$closeNote.MaximumSize=New-Object Drawing.Size(620,0)
$layout.Controls.Add($closeNote,0,8)

$saved=$null
try {if(Test-Path -LiteralPath $settingsPath){$saved=Get-Content -LiteralPath $settingsPath -Raw -Encoding UTF8 | ConvertFrom-Json}} catch {$saved=$null}
$script:hostAddress='';$script:guestAddress=''
if($saved -and $saved.schema -eq 1) {
    if($saved.game_directory){$gamePath.Text=[string]$saved.game_directory}
    if($saved.role -eq 'guest'){$script:guestAddress=[string]$saved.address}
    else {$script:hostAddress=[string]$saved.address}
}
$script:previousRole=-1
$role.add_SelectedIndexChanged({
    if($script:previousRole -eq 0){$script:hostAddress=$address.Text}
    elseif($script:previousRole -eq 1){$script:guestAddress=$address.Text}
    $address.Items.Clear()
    if($role.SelectedIndex -eq 0) {
        foreach($ip in @(Get-HostAddresses)){[void]$address.Items.Add($ip)}
        $address.Text=$script:hostAddress
        if(!$address.Text -and $address.Items.Count){$address.SelectedIndex=0}
        $addressLabel.Text='Твой IP в Radmin'
        $help.Text='Ты хост: выбери свой IP из Radmin и передай его другу. Друг вводит этот же адрес у себя. Оба должны быть в одной сети Radmin.'
        $start.Text='Создать сессию и запустить игру'
    } else {
        $address.Text=$script:guestAddress;$addressLabel.Text='IP друга-хоста'
        $help.Text='Ты гость: введи IP друга из Radmin. Друг сначала запускает хост. После загрузки одинаковой сцены соединение происходит автоматически.'
        $start.Text='Подключиться и запустить игру'
    }
    $script:previousRole=$role.SelectedIndex
})
$role.SelectedIndex=if($saved -and $saved.role -eq 'guest'){1}else{0}
$browse.add_Click({
    $picker=New-Object Windows.Forms.FolderBrowserDialog
    $picker.Description='Выбери папку Mafia II Classic, внутри которой находится pc\mafia2.exe'
    $picker.ShowNewFolderButton=$false
    try {if($picker.ShowDialog($form) -eq 'OK'){$gamePath.Text=$picker.SelectedPath}} finally {$picker.Dispose()}
})
$logs.add_Click({if($script:runDirectory){Start-Process explorer.exe -ArgumentList ('"'+$script:runDirectory+'"')}})
$crashes.add_Click({
    try {
        New-Item -ItemType Directory -Path $crashDirectory -Force | Out-Null
        Start-Process explorer.exe -ArgumentList ('"'+$crashDirectory+'"')
    } catch {Show-Problem ('Не удалось открыть крашлоги: '+$_.Exception.Message)}
})
$timer=New-Object Windows.Forms.Timer
$timer.Interval=600
$start.add_Click({
    try {
        if($script:sessionProcess -and !$script:sessionProcess.HasExited){return}
        $ip=$null;$ipText=$address.Text.Trim()
        if($ipText -notmatch '^(0|[1-9]\d{0,2})(\.(0|[1-9]\d{0,2})){3}$' -or
           ![Net.IPAddress]::TryParse($ipText,[ref]$ip) -or $ip.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {
            throw 'Введи IPv4 хоста из Radmin, например 26.123.45.67.'
        }
        $octets=$ip.GetAddressBytes()
        if($octets[0] -eq 0 -or $octets[0] -ge 224){throw 'Нужен обычный IPv4-адрес хоста.'}
        if($role.SelectedIndex -eq 0 -and @(Get-HostAddresses) -notcontains $ipText) {
            throw 'Этот IP не принадлежит твоему ПК. Для хоста выбери свой адрес из Radmin.'
        }
        if($role.SelectedIndex -eq 1 -and @(Get-HostAddresses) -contains $ipText) {
            throw 'Ты указал IP своего ПК. Для гостя нужен IP друга-хоста из Radmin.'
        }
        $directory=[IO.Path]::GetFullPath($gamePath.Text.Trim())
        if(!(Test-Path -LiteralPath (Join-Path $directory 'pc/mafia2.exe') -PathType Leaf)) {
            throw 'Не найдена pc\mafia2.exe. Выбери папку Mafia II Classic.'
        }
        $build=Get-BuildDirectory
        foreach($file in @('probe_launcher.exe','coop_observer.dll')) {
            if(!(Test-Path -LiteralPath (Join-Path (Join-Path $root $build) $file) -PathType Leaf)) {
                throw 'В этой папке нет собранного мода. Нужен полный архив preview, а не только исходники GitHub.'
            }
        }
        if(Get-Process mafia2 -ErrorAction SilentlyContinue){throw 'Сначала выйди из уже открытой Mafia II обычным способом.'}
        $roleName=if($role.SelectedIndex -eq 0){'host'}else{'guest'}
        $stamp=(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8)
        $script:runDirectory=Join-Path $root "work/launcher-runs/$stamp"
        New-Item -ItemType Directory -Path $script:runDirectory -Force | Out-Null
        $request=@{schema=1;role=$roleName;address=$ipText;
            game_directory=$directory;build_directory=$build}
        $requestPath=Join-Path $script:runDirectory 'request.json'
        $request | ConvertTo-Json | Set-Content -LiteralPath $requestPath -Encoding UTF8
        $request | ConvertTo-Json | Set-Content -LiteralPath $settingsPath -Encoding UTF8
        $powerShell=Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
        $info=New-Object Diagnostics.ProcessStartInfo
        $info.FileName=$powerShell;$info.UseShellExecute=$false;$info.CreateNoWindow=$true
        $info.WorkingDirectory=$root
        $info.Arguments='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+(Join-Path $PSScriptRoot 'launcher-session.ps1')+'" -RequestPath "'+$requestPath+'"'
        $info.EnvironmentVariables['PSModulePath']=(Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/Modules')+';'+(Join-Path $env:ProgramFiles 'WindowsPowerShell/Modules')
        $script:sessionProcess=[Diagnostics.Process]::Start($info)
        $script:liveState=New-CoopLiveState
        $script:statusReadFailed=$false
        foreach($control in @($start,$role,$address,$gamePath,$browse)){$control.Enabled=$false}
        $logs.Enabled=$true;$status.ForeColor=$muted
        $status.Text='Запуск мода… В игре пропусти интро двумя Escape и выбери «Продолжить». Сам запуск ещё не подтверждает соединение с другом.'
        $timer.Start()
    } catch {
        [void](Write-CoopFailureReport -Root $root -Failure $_ -Kind 'launcher-validation-or-start' -SessionDirectory $script:runDirectory)
        Show-Problem $_.Exception.Message
    }
})
$timer.add_Tick({
    if(!$script:sessionProcess){return}
    if(!$script:sessionProcess.HasExited) {
        if(!$script:statusReadFailed) {
            try {
                Read-CoopLiveState -Root $root -RunDirectory $script:runDirectory -State $script:liveState
                $status.Text=switch(Get-CoopLiveCode $script:liveState) {
                    'waiting_guest' {'Хост открыт — ожидаем друга. В игре выбери «Продолжить».'}
                    'connecting' {'Подключаемся к хосту. В игре выбери «Продолжить».'}
                    'connected_loading' {'Связь установлена. Загрузи игру через «Продолжить».'}
                    'waiting_scene' {'Связь есть — ожидаем активную сцену второго игрока рядом с тобой.'}
                    'avatar_visible' {'Сетевой аватар создан. Можно проверять движение вместе с другом.'}
                    'paused' {'Игра на паузе или неактивна. Вернись в игру — аватар восстановится.'}
                    'reconnecting' {'Связи с другом нет. Гость переподключается автоматически; перезапуск пока не нужен.'}
                    'refused' {'Подключение отклонено. Сверьте версии сборки и настройки; подробности в журнале.'}
                    'scene_ended' {'Сцена перезапущена. Для коопа заново запустите preview у обоих игроков.'}
                    'stopped' {'Сетевой модуль завершён. Ожидаем обычного выхода игры.'}
                    default {'Запуск мода… Пропусти интро двумя Escape и выбери «Продолжить».'}
                }
            } catch {
                $script:statusReadFailed=$true
                $status.Text='Не удалось прочитать статус. Игра продолжает работать; подробности в крашлогах.'
                [void](Write-CoopFailureReport -Root $root -Failure $_ -Kind 'launcher-status-read' -SessionDirectory $script:runDirectory)
            }
        }
        return
    }
    $timer.Stop()
    try {
        $resultPath=Join-Path $script:runDirectory 'result.json'
        if(!(Test-Path -LiteralPath $resultPath)){throw 'Процесс запуска завершился без отчёта. Открой папку журнала.'}
        $result=Get-Content -LiteralPath $resultPath -Raw -Encoding UTF8 | ConvertFrom-Json
        if($result.status -ne 'finished' -or $script:sessionProcess.ExitCode -ne 0) {
            throw ('Запуск завершился с ошибкой: '+$result.message)
        }
        $status.ForeColor=$muted
        if($result.child_exit_code -ne 0 -or $result.launcher_exit -ne 0 -or
            $null -eq $result.original_profile_changed_files -or $result.original_profile_changed_files -ne 0) {
            throw 'Нет подтверждения корректного выхода и сохранности профиля. Открой журнал запуска.'
        }
        $status.Text=if($result.avatar_created) {
            'Игра закрыта без ошибок, исходные сейвы не изменены. Отчёт сессии сохранён.'
        } else {'Игра закрыта без ошибок, исходные сейвы не изменены. Показ аватара в этой сессии не подтверждён.'}
    } catch {
        [void](Write-CoopFailureReport -Root $root -Failure $_ -Kind 'launcher-session-result' -SessionDirectory $script:runDirectory)
        Show-Problem $_.Exception.Message
    }
    finally {
        $script:sessionProcess.Dispose();$script:sessionProcess=$null
        foreach($control in @($start,$role,$address,$gamePath,$browse)){$control.Enabled=$true}
    }
})
try {[void]$form.ShowDialog()}
finally {
    $timer.Stop();$timer.Dispose()
    # Dispose only our process handle; never terminate the worker or game.
    if($script:sessionProcess){$script:sessionProcess.Dispose()}
    $form.Dispose()
}
