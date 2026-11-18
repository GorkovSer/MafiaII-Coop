# Контроль передачи стрелок и Enter

`tests/input_host.c` — необязательное отдельное Windows-окно для диагностики
инструмента управления. Это не игровой клиент и не модификация Mafia II.
Собирается `scripts/build-probe.ps1` как `input_host.exe`, но автоматически
проверками загрузчика не запускается.

Окно сравнивает два пути: WM_KEYDOWN/WM_KEYUP и буфер DirectInput 8. Буфер
сохраняет короткие события между опросами, поэтому отсутствие события нельзя
объяснить только слишком быстрым отпусканием клавиши. Для DirectInput требуется
успешный setup и строка `directinput_buffer_ready=1`.

Записываются только Enter и стрелки вверх/вниз при активном собственном окне.
Нет global hooks, захвата текста, ввода в другие приложения или генерации
нажатий. DirectInput использует foreground/nonexclusive keyboard. Журнал
создаётся рядом с EXE под новым именем `input-check-<pid>-<ticks>.log`; этот
локальный артефакт нельзя публиковать вместо обезличенного результата.

Пример запуска после сборки, с абсолютными путями:

```powershell
$build = (Resolve-Path '.\build\probe').Path
New-Item -ItemType Directory -Path '.\work' -Force | Out-Null
$evidence = Join-Path (Resolve-Path '.\work').Path ('input-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $evidence | Out-Null
& "$build\probe_launcher.exe" "$build\input_host.exe" "$build\coop_probe.dll" "$evidence\probe.log" --user --wait
```

Выбрать именно это окно поддерживаемым инструментом UI, отправить одну стрелку,
затем Enter и проверить журнал. При физической/иной разрешённой проверке не
вводить текст: нужны только эти три клавиши. Закрытие выполняется обычным
Alt+F4; нативный код выхода фиксирует launcher.

## Проверено в этой сессии 18 ноября 2026 года

- Компиляция pinned clang 23.1.5 с warnings-as-errors прошла.
- Собственный процесс запущен через `--user`, средний уровень прав. Настройка
  DirectInput вернула 0; буфер стал готов.
- `sky.press_key` для Down дал сообщения с vk=40, scan=0, extended=1;
  для Return — vk=13, scan=0, extended=0. События были и нажатия, и отпускания.
- Соответствующих `directinput_key` в готовом буфере не было. Исходный контроль
  GetDeviceState/GetAsyncKeyState также не наблюдал удержания этих клавиш.
- Оба диагностических процесса закрылись через обычный Alt+F4 с child_exit=0.

Это воспроизведённое ограничение данного пути автоматизации, а не доказательство
неисправности самой Mafia II или физической клавиатуры. Нулевой scan code и
разница между сообщениями окна и DirectInput объясняют, почему результат вызова
press_key нельзя считать игровым нажатием. Полный экран был проверен отдельно и
сам по себе не подтвердил работоспособность этого пути клавиш.

Для требуемой загрузки через стрелки/Enter нужен разрешённый способ, реально
поставляющий игровые клавиатурные события. Текущий документированный интерфейс
sky.press_key не предоставляет параметров scan code или удержания. Не заменять
эту приёмку внутренним Continue и не заявлять, что сохранение загружено через
меню. До появления рабочего пути ввода можно независимо продолжать разработку
жизненного цикла аватара и согласования сюжетного состояния.

## 2026-11-18 — authorized native menu input

User explicitly authorized a separate native scan-code helper for arrows and Enter in our diagnostic Mafia II. coop_input validates full EXE SHA256, explicit PID, process creation time, live process and foreground owner. It holds one key for 100 ms and always releases it. SendInput is system input: foreground checking is not atomic with focus changes; stop and reobserve after any focus/error. No parallel senders, arbitrary text, hooks or OS changes.

Actual game verification: The Story opened; Down selected Load Chapter, Up returned to Continue Story; Enter on Continue Story loaded the isolated save to a visible snowy street with Vito. Internal Continue was not enabled. The observer produced active player samples. A passive ENet loopback receiver received 190 active samples in 10 seconds, all axis ranges zero. Autonomous movement, a remote avatar, two game clients and Radmin gameplay remain unverified. The game remains open; native exit and final original-profile comparison are pending.

Usage: coop_input inspect PID, then coop_input key PID CREATION up|down|left|right|enter. Use only our launcher's PID and inspect's creation value. Pinned clang warnings-as-errors build passed; wrong creation, invalid key and zero PID refused input.

User requested faster entry. Next runs should omit the completed diagnostic Down/Up detour and rebuilds when code is unchanged. Use lifecycle logs to wait for loading, screenshots at Continue selection and ready scene, and never retry Enter when its outcome is unknown. A fresh end-to-end timing measurement remains pending.