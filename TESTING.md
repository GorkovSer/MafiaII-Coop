# Проверки

## 2026-11-17 — первоначальный аудит

- Текущий каталог не имел `.git` и исходников. Существующую историю переписывать не потребовалось.
- Создан отдельный приватный GitHub-репозиторий `GorkovSer/MafiaII-Coop`; используется `development`.
- Git доступен; настроен credential helper `manager`. Чтение нового приватного репозитория через существующую Git-авторизацию прошло без вывода токенов.
- Клон m2o-reborn получен и зафиксирован по commit; обнаружен принудительный запуск freeride.
- Найден целевой GOG EXE, снят SHA-256. SDK-совместимость не доказана.

## Не выполнено

Второй персонаж, сетевой игровой мир, миссии, сохранения, производительность и любые проверки на двух компьютерах через Radmin VPN. Нет готового кооперативного мода или оснований для релиза.

Для будущих игровых испытаний фиксировать commit/сборку, точный EXE, стороны host/guest, условия, действия, ожидаемый/реальный результат и обезличенные свидетельства. Логи, дампы, личные сохранения и пути остаются локально.

## Диагностический код 0.0.1 — фактические результаты

- LLVM-MinGW 20261104 / clang 23.1.5 загружен с GitHub автора, SHA256 архива сверён с digest GitHub API. Zig download не удался; Zig не является зависимостью проекта.
- `scripts/build-probe.ps1`: x86 DLL, launcher и собственный test host собраны с `-Wall -Wextra -Werror`, ошибок/предупреждений нет.
- `tests/test_audit_game.py`: 4 проверки границ PE/повреждённых заголовков прошли. Тесты не подтверждают валидность игровых функций.
- Реальный test host: DLL загружена, экспорт вызван вне DllMain, локальный лог содержит совпадающий PID, PE machine 014c и 32-bit pointers. Игра в этом тесте не участвует.
- Целевая Mafia II GOG: DLL загрузилась и выполнила экспорт в процессе EXE с ожидаемыми image base / entry RVA. `game_api_calls=0`, `game_patches=0`. Лог находится только в игнорируемом `work/probe-sessions`.
- Первоначальная проверка из ограниченного окружения не видела игровой процесс. Контрольный повторный запуск без DLL вернул 0; это не доказывает, что исходная игра завершилась или не работает. Пользователь сообщает, что вручную игра запускается. Раннее заключение о немедленном закрытии было преждевременным.
- В проверенных трёх стандартных каталогах сохранений данных не найдено; скрипт зафиксировал их отсутствие. Это не доказывает отсутствие сохранений в других каталогах. До сюжетных тестов определить реальный save path.
- После диагностической пробы игровые API/адреса не вызывались и код EXE на диске не патчился. Сравнение 12 baseline files (EXE/DLL/config верхнего уровня pc) не выявило изменений. Это не проверка всех игровых ресурсов или сохранений.
- В тестовом PowerShell наблюдение `Get-Process.ExitCode` дало неоднозначный результат. Добавлен нативный `--wait`, который удерживает исходный process handle. Повторный тест собственного host: `GetExitCodeProcess` вернул 0. Проверки существующего log и отсутствующей DLL также прошли. Всего 4 интеграционные проверки загрузчика.
- Повторная проба в игре с `--wait` создала новый внутрипроцессный лог, процесс наблюдался через живую exec session. До подтверждения окна/меню это только доказательство жизненного цикла, не игровая приёмка.
- После игрового запуска появился стандартный каталог `Local/2K Games/Mafia II` с `Data` и `Saves`. Он должен быть сохранён перед дальнейшими испытаниями; прежняя запись об отсутствии относится только к моменту первого запуска.
- Внешняя проверка конкретного PID вне ограниченного окружения подтвердила: целевой `pc/mafia2.exe` работает, окно называется `Mafia II`, `Responding=true`. Он оставался активным несколько минут под нативным `--wait`. В следующей проверке найден только этот игровой процесс. Меню/сюжетный gameplay ещё не подтверждены.
- Найденные Data/Saves скопированы в локальный игнорируемый `work/backups`: 4 файла. Их содержимое и пути не выгружались в GitHub.
- `scripts/inspect_runtime.py` успешно прочитал candidate globals только в процессе с совпадающим SHA256: reborn game/manager имеют читаемые цепочки предполагаемой vtable с первой функцией в исполняемой секции EXE; RC4 candidate такую проверку не прошёл. `memory_writes=0`, `game_api_calls=0`. Это read-only наблюдение, не тест вызова SDK.

## 2026-11-18 — early loader и профиль тестирования

- Сборка в отдельный `build/probe-next` прошла с `-Wall -Wextra -Werror`; загруженный в предыдущую игру DLL не перезаписывался.
- `scripts/test-probe.ps1 -BuildDirectory build/probe-next`: 8 интеграционных проверок прошли. Включают обычную загрузку, нормальный выход, защиту log, отсутствие DLL, выполнение до test host entry, отказ observer неизвестному EXE, реальную запись test host в изолированный LocalAppData и сохранение поведения CSIDL_WINDOWS.
- После обычного закрытия предыдущей игры выполнен ранний запуск observer на целевом GOG EXE: стартовые пять байт восстановлены, все 22 AddAction приняты. Получены реальные callbacks меню, mission_before_open/open, game_init, tick/render, load_finished, pause/unpause, game_done и mission_before_close/close. Регистрация и наблюдавшиеся callbacks выполнялись на одном игровом потоке. Это не подтверждает ещё не вызванные события restore/shutdown и поведение гостевого персонажа.
- Пользователь подтвердил нормальную работу после просьбы проверить меню/управление и начать игру. Затем по инструкции вышел и сообщил о закрытии. Нативный process handle зафиксировал `child_exit=0`; оставшихся процессов Mafia II нет.
- По указанию пользователя тест выполнен в окне 1280×720 на DISPLAY2; монитор и client size проверены загрузчиком. Добавлено ожидание стабильного размера окна перед центрированием; эта небольшая последующая правка пока прошла только сборку.
- Redirect LocalAppData сработал внутри игры. Число файлов в копии профиля увеличилось с 4 до 6. После завершения исходный профиль полностью совпал с backup (0 изменённых/новых/удалённых файлов), все 12 файлов baseline EXE/DLL/config также неизменны. Полное прохождение главы, восстановление checkpoint и сохранение кампании этим коротким тестом не доказаны.
- В скрипт добавлено автоматическое сравнение исходных профилей после завершения; в описанном запуске сравнение выполнено отдельной проверкой. Личные пути, профили и сырые журналы остаются только в игнорируемом work.

## 2026-11-18 — чтение сюжетного игрока

- Повторный запуск проверенной сборки с ожиданием стабильного размера окна: DISPLAY2, client 1280×720, нормальное центрирование. Пользователь подтвердил нахождение в игре. Внешнее read-only наблюдение дало 75 снимков за 15 секунд, один GUID, изменяющиеся физические координаты. Игровые функции для чтения не вызывались.
- После обычного закрытия этого экземпляра нативный handle вернул 0. Автоматическое сравнение исходного профиля после выхода не выявило изменений.
- Первая новая сборка с `telemetry.c` получила реальные снимки из зарегистрированного игрового callback. Выявлен дефект готовности: после game_init до load_finished позиция временно равна нулю. Такие снимки нельзя считать пригодными для синхронизации.
- Исправлено ожидание game_init + load_finished; load_started и завершение миссии запрещают чтение. Добавлены epoch, проверки конкретных virtual targets, читаемости и конечности координат. Указатели не хранятся между тиками.
- Исправленная сборка `build/telemetry-loaded` собрана без предупреждений; 8 интеграционных проверок загрузчика прошли. 4 проверки PE также прошли. Анализатор runtime evidence принимает корректное движение и отвергает 7 случаев: отсутствие снимков, чтение до загрузки, после game_done, неверная epoch, повтор времени, NaN и несовпадение потока.
- Повторная игровая приёмка исправления ещё ожидается. Текущий тестовый процесс подтверждён живым/Responding и не закрывался принудительно. Два ПК, Radmin VPN и второй персонаж по-прежнему не проверены.

## 2026-11-18 — ENet и автономное тестирование

- Пользователь передал агенту дальнейшее управление и все тесты, уходя спать. Старый тестовый экземпляр закрыт пользователем; его исходный process handle вернул 0. В журнале есть два открытия/закрытия миссии и реальные меняющиеся координаты. Это старая сборка до исправления load_finished: её ранние нулевые снимки не считаются пригодными.
- Добавлен ENet transport, wire codec, host/guest handshake, назначение ID, state/epoch/sequence validation, отключение/rejoin, отдельный worker и ограниченные mailbox. См. docs/NETWORK.md; игровое применение remote state отсутствует.
- Финальная сборка прошла с `-Wall -Wextra -Werror` для собственного кода. ENet исходники собраны отдельно и проверены по manifest. Девять интеграционных проверок загрузчика прошли, включая запуск собственного host с ограниченным токеном и нормальным выходом. Прямое чтение token integrity подтвердило 0x2000 у тестового child; затем тот же уровень подтверждён у реальной Mafia II. Глобальные настройки безопасности не изменялись.
- Проверки wire: точный порядок байтов, усечённые/лишние данные, version/reserved/type, NaN, inactive с координатами, sequence wrap. Loopback в двух процессах: двусторонний обмен, очистка inactive, отказ несовместимым game build/capabilities, повторное подключение, неверный IPv4. Worker/mailbox проверен отдельно: остановка producer не останавливает транспорт, устаревшая позиция очищается.
- Первый loopback внутри сетевой песочницы не соединился; без изменения кода вне неё тест прошёл. Нельзя считать эту первоначальную неудачу доказательством дефекта ENet.
- В реальной игре пройдены observer startup и сетевой handshake с пассивным приёмником: 598 inactive-сообщений за пять минут, активных снимков 0, netcheck exit 4. Это частичное игровое свидетельство, не прохождение теста передачи позиции. Скриншоты подтвердили меню, окно на DISPLAY2 и 1280×720.
- Windows computer-use прочитал меню, но автоматические нажатия не переключили сюжетный пункт. Нормальный уровень прав сам по себе не решил проблему. Собственные диагностические экземпляры в меню останавливались для перезапусков; такие завершения отмечаются отдельно и не считаются нормальным выходом игры. Исходный профиль проверяется по хешам независимо от результата запуска.
- Добавлена отдельная версия модуля 0.0.3 в HELLO/WELCOME, точная проверка совместимости и дополнительный отказ несовместимому module version в loopback fixture. Игровая проба выше была до расширения этого формата.
- Не проверены: последний handshake внутри игры, таймаут на настоящем разрыве Radmin, две игры/два ПК, реальная передача активной позиции текущей сборкой, remote персонаж и совместная миссия. Работа продолжается автономно; ручные действия пользователя не запрашиваются.
- Последняя диагностическая игра завершена контролируемым принудительным остановом из меню, а не засчитана как normal exit. Автоматическое сравнение исходного профиля: 0 изменений; отдельное сравнение 12 baseline-файлов EXE/DLL/config: 0 изменений. Все 18 vendored ENet files совпали с закреплённым upstream после нормализации LF.

## 2026-11-18 — opt-in story Continue diagnostic

- Added an explicit isolated-test-profile copy and opt-in game-thread Continue request. Default behavior remains disabled. Full EXE hash, code prefixes, profile redirection and save-catalogue bounds are checked. See docs/STORY_TEST.md.
- The catalogue-only attempt returned 1 but the GUI reported missing DLC while Checking for downloadable content was visible. This was not a successful playable load. Our owned diagnostic instance was forcibly stopped after UI exit confirmation did not complete; original-profile comparison reported 0 changed files.
- The subsequent main-menu-gated attempt returned 1, emitted load_started, mission_open, game_init and load_finished, and external read-only inspection found the local player and three save catalogue entries. The main menu remained visible and tick_paused was observed. No automated movement or visible gameplay is claimed.
- The module-version-3 passive receiver connected to this actual game, but received only inactive state while paused. This is not acceptance of active position transmission or a second game client.
- Windows input observations must distinguish the game's own cursor from the system cursor shown by the automation helper. Previous coordinate clicks alone did not prove that the game cursor reached a button. Keyboard/UI operation remains under investigation.
- Both diagnostic builds compiled with warnings treated as errors. Loader/profile fixtures passed 9 integration checks; telemetry evidence checks passed one valid and seven invalid cases. The final build passed wire tests and two-process loopback handshake, state reset, compatibility rejection, reconnect and stalled-producer checks. These fixtures are not Radmin/two-PC gameplay.
- Game GUI resource metadata was inspected locally through a read-only SDS19 adapter based on zlib-licensed Gibbed.Illusion b166d9fb8854620ee9eba08246b86c0d1109c948. Extracted original resources and the local adapter remain ignored and are not distributed. No game files were edited by this research.

## 2026-11-18 — continuation on the installed C: copy

- Revalidated the installed Classic GOG executable against the pinned SHA256. Recovered the pinned LLVM-MinGW archive, verified its SHA256 and rebuilt the current sources with warnings treated as errors.
- PE audit checks (4), telemetry evidence checks (valid movement plus 7 rejected cases), all 9 loader/profile checks and the complete network fixture suite passed. These are diagnostic and loopback checks, not two-PC gameplay.
- The first windowed launch reported DISPLAY2 and client 1280x720, but Windows capture showed the right/bottom edges outside the visible screen. The user moved the window fully into view. No active mission or moving position has yet been observed in this session.
- Added per-monitor-v2 DPI context to the launcher's own thread before monitor enumeration, with refusal before launch if unavailable. The final placement now requires the whole outer rectangle inside DISPLAY2's work area and logs both rectangles. The child and system display settings are unchanged. API reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setthreaddpiawarenesscontext
- The placement correction builds successfully; loader regression checks and a fresh DISPLAY2 runtime launch are being verified. Do not count the user's manual move as validation of the new placement code.

## 2026-11-18 — live factory inspection and remaining gameplay limits

- The original diagnostic process entered the main-story introduction. Its log has mission_open, game_init, load_finished and exactly one accepted position sample, then game_paused/tick_paused. The sample has zero observed coordinate range. This proves neither movement nor a complete lifecycle acceptance test.
- A module-version-3 passive peer connected to that real game: 120 received states over 60 seconds, 0 active samples, exit 4. Do not report active game-to-peer position acceptance, a second game client, or Radmin gameplay.
- The read-only factory audit traversed 48 registered records and found Human and Player callbacks in executable image sections. The original game process was not modified by the inspection. Factory tests cover missing head/record memory, an empty list, bounded traversal, cycles, duplicate types and nonexecutable targets.
- The DPI placement patch passed all nine loader/profile regression checks. Its two source files were published through the authenticated internal browser in commit b060b2c; fetched contents match the reviewed local sources. A fresh runtime placement check is still pending.

## 2026-11-18 — pause invalidation and explicit lifecycle acceptance

- Telemetry now publishes inactive on game_paused and tick_paused immediately; a loaded ordinary tick may publish position again. Previously the network worker could retain the last active position for its two-second producer timeout after pause. This change adds no engine calls.
- Observer logging preserves every lifecycle transition, including repeated mission_before_close, no_game_start/end and pause/unpause. Per-frame events remain limited to their first log record. This makes later menu returns observable to the acceptance checker.
- Added an isolated x86 fixture using the production telemetry adapter. It supplies test memory only inside its own process and checks pre-load rejection, immediate pause invalidation, resume, loading, changed reader target, closure, a second epoch and shutdown. It passed. Fixture coordinates and numeric vtable values are not game evidence; no game callbacks are executed by that fixture.
- The evidence summary now exposes lifecycle_acceptance=false for a lone stationary sample. Strict mode requires at least two moving sampled epochs, closure of every sampled epoch, a later menu return, shutdown and a launch-result.json proving a waited-for zero native child exit and unchanged original profile. Missing, nonzero, malformed or ambiguous exit evidence cannot pass. The Python acceptance cases and launcher-result parser cases passed.
- start-probe.ps1 saves private launcher stdout and an exit/profile result after completion. The new result writer parses successfully and its parsing is checked against native loader-fixture output. Its use in a fresh real-game session remains pending.
- The updated DLLs and telemetry fixture built with warnings treated as errors. Loader/profile and network suites passed for this build. The current previously launched game still provides only one stationary sample, so actual two-load lifecycle acceptance, new pause behavior in Mafia II and fresh DISPLAY2 placement are not claimed.

Example after an actual test session has finished:

```powershell
python scripts/summarize_telemetry.py work/probe-sessions/<session>/probe.log --require-lifecycle --launch-result work/probe-sessions/<session>/launch-result.json
```

This strict diagnostic result still does not establish two-PC gameplay, NPC synchronization or campaign support.
