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

## 2026-11-18 — integration review and deterministic lifecycle checks

- Reviewed development through a73f70c and lifecycle work through 2b74f22 without rewriting the coauthor's commits. Added negative evidence cases after reproducing acceptance of shutdown before missions and samples during an unresumed pause.
- The analyzer now rejects initialization/loading after shutdown and position samples during pause. Strict acceptance requires sampled mission closures and a menu return before the first shutdown, in addition to the existing native-exit/profile evidence.
- The actual adapter now retains its paused state until game_unpaused. An ordinary tick cannot republish position during pause. This adds no new engine calls or game-memory writes; fresh Mafia II runtime verification remains pending.
- The original native fixture intermittently failed at fixed-address allocation and at a real-time Sleep-based expectation. The fixture now supplies owned singleton storage and controlled time only in its separate test build. The production DLL retains the audited address and Windows clock; its symbol table contains no fixture substitution functions.
- Build passed with warnings treated as errors. Python evidence, factory and four PE checks passed, as did all nine loader/profile checks and the network fixture suite. The deterministic production-adapter fixture passed 100 consecutive process runs. This is regression evidence, not automated movement, a second avatar, a second game client or Radmin gameplay.

## 2026-11-18 — window messages versus buffered DirectInput

- Added an optional standalone input_host, separate from Mafia II and not launched by automated loader tests. It monitors only Enter/Up/Down in its own foreground window, with no input generation, global hook or text capture.
- A real medium-token process initialized DirectInput successfully and reported a ready event buffer. sky.press_key Down and Return produced WM_KEYDOWN/WM_KEYUP with scan=0 but no matching buffered DirectInput events. Buffering rules out loss solely between GetDeviceState polling intervals. The earlier polling-only diagnostic agreed.
- Both owned input diagnostic processes closed normally through Alt+F4 with native child_exit=0. This does not prove autonomous menu navigation, gameplay, or a successful save load. It identifies a limitation of the tested automation path; see docs/INPUT_DIAGNOSTIC.md and the updated coauthor handoff.

## 2026-11-18 — story context candidate audit

- Rehashed the installed EXE; it matches the pinned GOG SHA256. Static CGame Open/parser analysis plus local read-only GMP resource headers identified four CGame strings as asset roots, excluding another false mission-ID interpretation. Original archives were not changed or published.
- Added bounded read-only inspection of resident GMP roots and the native save/load interface's +0x68 value, with a save-catalogue comparison. The console omits raw roots, values and addresses; the private result always marks identity_verified=false. No candidate is sent as a network mission ID.
- Synthetic memory tests passed valid roots, missing memory, menu/not-open, mismatched pointers, wrong version, invalid sizes, invalid root text, extra data, missing NUL, address overflow and absent singleton/value cases. Factory tests and four PE checks passed. Inspector syntax/import integration was checked. No game-memory write or new game API call was added; actual current chapter/checkpoint interpretation remains unverified.

## 2026-11-18 — authorized native menu input

User explicitly authorized a separate native scan-code helper for arrows and Enter in our diagnostic Mafia II. coop_input validates full EXE SHA256, explicit PID, process creation time, live process and foreground owner. It holds one key for 100 ms and always releases it. SendInput is system input: foreground checking is not atomic with focus changes; stop and reobserve after any focus/error. No parallel senders, arbitrary text, hooks or OS changes.

Actual game verification: The Story opened; Down selected Load Chapter, Up returned to Continue Story; Enter on Continue Story loaded the isolated save to a visible snowy street with Vito. Internal Continue was not enabled. The observer produced active player samples. A passive ENet loopback receiver received 190 active samples in 10 seconds, all axis ranges zero. Autonomous movement, a remote avatar, two game clients and Radmin gameplay remain unverified. The game remains open; native exit and final original-profile comparison are pending.

Usage: coop_input inspect PID, then coop_input key PID CREATION up|down|left|right|enter. Use only our launcher's PID and inspect's creation value. Pinned clang warnings-as-errors build passed; wrong creation, invalid key and zero PID refused input.

User requested faster entry. Next runs should omit the completed diagnostic Down/Up detour and rebuilds when code is unchanged. Use lifecycle logs to wait for loading, screenshots at Continue selection and ready scene, and never retry Enter when its outcome is unknown. A fresh end-to-end timing measurement remains pending.
## 2026-11-18 — autonomous movement and concrete Human lifetime

The primary agent remains on codex/story-context-audit. Native Up held for one second moved Vito in the real loaded story scene. A passive loopback receiver recorded 571 active samples over 30 seconds, axis ranges 0.25519 / 4.07849 / 0.00497. This is autonomous game movement carried over the existing network path, not a second game client. sky Escape did not open pause; do not claim pause/unpause acceptance from this session. The game is still open; final original-profile/exit checks remain pending.

coop_input now accepts optional hold_ms 50..2000, default 100. During hold it polls process/focus every 5 ms and releases early on loss; this reduces the exposure window but does not make system input atomic. Invalid durations 0, 49, 2001, negative and overflowing values were rejected. Native helper compiled with warnings-as-errors.

Concrete Human ownership audit now verifies the actual Human slot-zero destructor 0x999160 -> body 0x990770 -> conditional deallocation 0x4019f0. Release's immediate path calls this slot with argument 1. The alternate manager handoff remains untraced. See docs/REMOTE_AVATAR.md and scripts/audit_avatar.py. Never append destructor/Free after Release. Next work: trace manager +0x4c and model clone/setup ownership before real avatar calls. Read-only runtime inspection after normal Continue found 48 factory entries and valid map resource roots; the save/load candidate value was 0 and did not match the catalogue, so it remains unusable as a world ID.
## 2026-11-18 — Release sweep and model status traced

Read-only inspection of our still-live story game resolved global 0x1abe8dc to vtable 0x190af18, slots +0x2c=0x5b5b50 and +0x4c=0x5b5b90. Correction to earlier interpretation: +0x4c does not enqueue/store the passed pointer. It only sets CGame flag +4 |= 0x10. Sweep 0x5c2530 consumes that flag, enumerates entities through global 0x1abfe20, collects entities marked +0x20 & 0x400 and invokes deleting destructors. CGame registers this sweep for events 3 and 4 with priority 10000. Actual timing relative to our callbacks and mission teardown remains unverified.

Human SetModel 0x1164d00 is a raw +0x60 exchange returning the old pointer, with no reference increment or prior release. CloneHierarchy 0x14ec8f0 returns signed status: null source gives 0xfffffc1e, otherwise it forwards cloning status. The upstream void declaration must not hide clone failure in our adapter. The pinned exact-byte audit now verifies these paths and always leaves safe_spawn_or_removal_verified=false.

Added read-only release_context_candidate resolution to inspect_runtime.py; raw targets remain in private JSON while console reports only presence/executable slots. Exact-byte audit passed on the installed pinned EXE and refused an unknown image. Factory/story audit regressions and inspector syntax passed. No creation/removal/model call, game-memory write or new game DLL was introduced. Next work is clone ownership/failure cleanup, Setup/Activate validation and runtime sweep ordering; actual avatar creation remains pending.
## 2026-11-18 — real opt-in model clone/release verified

Implemented src/game/model_test.c and -TestModelClone. Disabled by default; enabled only by an explicit isolated-profile launch. The parent observer's whole-EXE gate and model entry signatures apply. It waits for init/load completion and an unpaused tick, checks profile redirection and the concrete source model class, allocates one model, clones the local player's appearance and releases exactly its own single reference. No entity is spawned or player's model replaced. Unexpected allocation contracts remain incomplete rather than triggering speculative cleanup.

Read-only audit verified the stdcall model factory 0x14ec940 (one allocator argument, callee pops 4), base constructor's initial reference of 1 and release-to-zero deallocation. In a real normal-menu Continue run, the game-thread log reported allocated=1, clone_status=0, release_remaining=0, source_reference_intact=1 and completed=1. Subsequent Up movement was visible; a passive receiver got 57 active samples over three seconds with axis ranges 0.29520 / 1.07001 / 0.00174. This validates one success-path resource lifecycle, not error cleanup, avatar placement, two clients or Radmin acceptance.

The preceding movement session closed normally with child_exit=0 and zero original-profile changes. Its actual Alt+F4 dialog generated pause; a passive receiver got six inactive snapshots and zero active samples over three seconds. Enter resumed the event stream then exited normally. The model-test session also closed normally with child_exit=0 and zero original-profile changes. No Mafia II test remains intentionally running.

Pinned full build with warnings-as-errors passed; production-adapter fixture and all nine loader/profile checks passed; complete wire/session/network-worker regression suite passed. The coauthor's history remains preserved. Next work is a separately gated Human creation test with model ownership transfer, Setup/Activate, positioning and single engine removal; do not claim an avatar until observed in game. Failure cleanup and mission/reload coverage remain required.
## 2026-11-18 — visible local Human and registry-confirmed removal

Implemented the explicitly gated -TestHuman / src/game/human_test.c path, disabled by default and mutually exclusive with the model-only launch switch. It creates one ordinary Human with a separately owned clone, transfers the one model reference using the audited 0x800 ownership flag, runs Setup/SetPosition/Activate and requests one Release after 60 seconds or an earlier unload/done/shutdown. The handle is cleared before Release; no subsequent entity read or explicit destructor/Free follows. This is a local development test, not a remote network player.

Three real normal-menu Continue launches were performed. The first two reached Setup/active and returned from Release, but did not establish a visible actor. Position evidence showed an early temporary checkpoint/model transform; clearing bit 0x40 alone did not establish visibility. The final code waits three seconds after loaded ticks and uses the audited physics position. That run visibly produced a Human beside Vito, with matching model/physics coordinates at creation and at five seconds; Setup retained the transferred model. The actor then moved under game simulation, so AI ownership/control remains an open requirement. The independent role of bit 0x40 has not been proved.

Added bounded read-only GUID lookup (scripts/entity_audit.py; inspect_runtime.py --entity-guid), traced from the actual entity manager slot +0x1c=0x5a0c70 through 0x59b130/0x58ebe0. After Release, the local player's GUID was present and the Human's GUID absent. Collision-chain, absence, mismatch, cycle, bounds, overflow-vector and unreadable-memory checks passed. Raw pointers/IDs remain in private JSON; normal console output exposes lookup status only.

Pinned exact-byte audit passed for Human factory, Setup, SetPosition slot +0x7c, ownership destructor and Activate. Full builds passed with warnings-as-errors; the native telemetry fixture and all nine loader/profile checks passed. A passive peer received 380 active local-player snapshots over 20 seconds during this session; this does not prove remote control. All three game sessions exited normally with native child_exit=0 and original_profile_changed_files=0. No test game remains running.

Next primary work: promote the local lifecycle into a controlled adapter with registry-validated handles, establish AI ownership and repeat creation/removal across mission reloads, then apply real remote snapshots and test with a second game client. See docs/HUMAN_TEST.md. No two-PC Radmin mission acceptance or full campaign completion is claimed.
## 2026-11-18 — быстрые Enter-вход/выход и распространяемый сейв

User explicitly authorized repeated Enter from startup until loading and immediate
Enter after Alt+F4. Added enter-story.ps1: owned isolated session/PID checks,
helper SHA256+creation+foreground checks per key, 100 ms Enter, 200 ms gap with
50 ms loading-event polling, 30-second menu and 60-second telemetry deadlines.
It stops sending keys on load_started/mission-open before awaiting loaded player
telemetry. Already-loaded and wrong-PID calls refused input. Native quit validates
before Alt+F4, then starts repeated Enter immediately inside the same invocation;
it stops on focus/process loss or five-second deadline and releases keys.
The window-disappearance/process-signal race was reproduced and fixed with a
bounded no-input native-handle wait. No force termination is used.

Real repeated-Enter runs (loading detected ms / first player sample ms / key count):
2167 / 4295 / 6; 1821 / 3256 / 5; 3818 / 5355 / 10; 1404 / 4168 / 4;
1584 / 3100 / 4. These measure script-loop time, excluding EXE startup/tool overhead.
Screenshots confirmed the snowy-street Vito save. Native immediate quit from story
completed in 2765 ms (12 Enter keys) and 3141 ms (15 keys), exit 0; launcher native
wait independently confirmed exit 0 and zero original-profile changed files.
An intermediate launch spanning an environment restart lacks final launcher
result; do not infer its exit code. Other quoted evidence comes from completed runs.

The user separately authorized publishing their tested save. Packaged only seven
profile files into testdata/story-save.zip (868438 bytes), SHA256 recorded in the
adjacent manifest; no EXE/DLL/game resources/logs/backups included. Profile metadata
and isolated video configuration are intentionally retained. install-test-save.ps1
verified the archive and installed a new isolated seed. The final real launch used
that newly installed seed: correct scene, loading at 1584 ms, player at 3100 ms,
normal immediate quit at 3141 ms and original profile unchanged. Wrong native quit
creation was rejected before input. Pinned clang 23.1.5 helper build passed with
-Wall -Wextra -Werror. No production DLL/network code changed in this stage.

See docs/GAME_UI.md and docs/TEST_SAVE.md for the coauthor's mandatory workflow.
This is local tooling/save acceptance, not two-client/Radmin/campaign acceptance.

## 2026-11-18 — native registry handle guard (runtime deferred)

Added a read-only production registry reader with a supplied bounded-memory
callback. The algorithm mirrors the audited 32-bit registry, checks full GUID
on results and zeros the output pointer on all non-present statuses. Invalid
reads/arithmetic/cycles/table bounds remain distinct from confirmed absence.
The Human adapter validates the manager lookup slot, freshly resolves GUID and
matches the owned address before accessing it. It verifies registration on
creation and polls registry-only state after Release; no freed-pointer reads.
A ten-second timeout records incomplete removal, never success.

Pinned clang 23.1.5 full build passed with warnings-as-errors. The production
registry fixture passed its independent key=1/bucket=1440 case, collision chain,
absence, generation mismatch, deleted value, unreadable GUID, cycle, index 6000,
overflow-vector reverse precedence, misalignment, length, reverse bounds, address
overflow and zero-GUID cases. Production telemetry fixture also passed.
The full loopback network suite completed successfully. The loader suite was
interrupted when the user asked to defer tests for Cyberpunk: observed passes
were real x86 load/export/PID/log, native child exit, existing-log refusal and
missing-DLL refusal. It is not a complete nine-check loader/profile pass.

No Mafia II was launched and no native/UI game input sent during this stage.
The new adapter has no real-game creation/removal evidence yet. Further suites
and game runs are deferred by explicit user request. Earlier visible Human and
registry-removal evidence applies to the previous version. AI control, remote
avatar application and repeated world lifetime remain open.

## 2026-11-18 — static movement-command analysis, no tests resumed

While the user's Cyberpunk test hold remained active, only the installed EXE
file and existing upstream source were inspected. The whole EXE SHA256 still
matched the pinned value. New audit_human_commands.py checked bounded exact-byte
sites for native MoveDir allocation/class/destructor, the incompatible upstream
class, AddCommand dispatch, queue insertion and intrusive reference retention/
release. Its read-only report was produced under ignored work/control-audit.
No game API calls, process memory writes, game launches or test suites occurred.

The upstream 0x58/0x19661bc combination is rejected for this build: its destructor
accesses +0x78 and its actual engine allocation is 0x98. The actual MoveDir
0x58 class is 0x18f14b0 with destructor 0x42d3e0. Native candidate 0x95b0f0 creates
and enqueues type 0/1 and returns an owned output reference; AddCommand consumes
its stack argument reference. Native queue/free-list interpretation is documented
in docs/HUMAN_CONTROL.md. These are static contract findings only. No runtime
safe-control, AI arbitration, allocation-failure recovery, command cancellation
or movement/animation acceptance is claimed. The observer is unchanged here.

## 2026-11-18 — native command-queue reader, compile-only stage

Implemented CoopReadCommandQueue using a bounded read callback and the pinned
layout from the prior static audit. It validates all slot/head ranges and list
partitioning, rejects cycles/cross-list aliases/orphan/free-owned references,
requires readable occupied headers with positive signed reference counts and
queue flag 0x400, and rejects changed raw topology on its second read. Failure
returns an all-zero output. This validates observable shape only; it does not
verify dynamic class methods, AI ownership, race-free concurrency or enqueue
permission. A valid full queue reports zero available slots.

The opt-in Human path now records queue status/free/used counts only after fresh
registry validation, immediately after activation and at five seconds. It adds
no game calls or writes to commands. A production-source fixture was authored
for empty/full, cyclic/overlapping/orphan topology, reference/header failures,
address overflow and changed-snapshot cases. Both observer and fixture compiled
in the full pinned clang 23.1.5 warnings-as-errors build. The fixture was NOT
executed, and no suite/game/input was run: the user's Cyberpunk test hold remains
in force. Consequently no PASS/runtime queue result is claimed for this stage.

## 2026-11-18 — command dispatcher and retained-reference observation

Static read-only EXE analysis identified Human vtable+0xc8 -> 0x9818a0 calling
0x94d9a0. Exact-byte command audit now covers descriptor dispatch, conditional
pop/reference release/free-list recycling, separate retention at Human+0x1a0,
ClearMove's cancellation/physics path and general clear's retained reset.
It also confirms the SDK AI NewCommand payload dereferences a separate counter
address; a raw MoveDir intrusive object is incompatible with that payload.
The static report still leaves AI ownership and runtime control unverified.

Extended production queue reading with retained header/membership, allowing a
retained command no longer queued, and a second retained-pointer comparison.
Opt-in Human logs add non-address retained presence/membership and raw dispatch
skip flag. No engine command call, refcount change or memory write was added.
The full pinned clang 23.1.5 build completed with warnings-as-errors. Fixture
cases for retained membership, retained after dequeue, invalid retained count
and changed retained pointer compiled but were NOT executed. No tests/game/UI
input were run due to the continuing user hold. Physical stopping on disconnect,
command cancellation, AI/script ownership and cleanup remain unverified.

## 2026-11-18 — incoming expiry and consumer validity (no execution)

Code review found that only the local producer had two-second expiry, while
incoming remote_active relied on the ten-second peer deadline. Added receiver
accepted-state time after identity/sequence/epoch acceptance; remote activity
and positions are invalidated at COOP_STATE_MAX_AGE_MS=2000 without clearing
sequence/epoch or forcing disconnect. Invalid/stale packets cannot renew this
validity. Unsigned ENet-clock subtraction handles normal uint32 rollover.

The mailbox adds receiver-local timestamps and connection/accepted-sequence
metadata. CoopRemoteSnapshotAt filters each consumer copy for stale worker,
stale state, no accepted state, inactive state and invalid coordinates/metadata.
CoopNetworkReadRemote now zeros outputs on no worker/stop and rejects stale
mailbox data even if the worker itself is frozen. Wire encoding stays unchanged;
this does not establish world identity or physically stop a Human.

Pinned full clang 23.1.5 warnings-as-errors build succeeded. The first build
exposed a Windows/Winsock include-order conflict in the new fixture; the mailbox
header now uses stddef/stdint only and the implementation includes Windows itself.
Prepared fixtures cover 1999/2000-ms boundaries, live-worker/stale-state versus
stale-worker, accepted-state recovery, inactivity, missing identity, nonfinite
coordinates, future clocks, aliasing, no state and uint32 clock rollover. Added
future loopback silent-STATE/ENet-service scenario and post-Stop zero-output check.
These fixtures were compiled only. No test suite, networking fixture, game, DLL
or UI input was executed due to the user's continuing test hold. This stage has
no runtime PASS; prior transport/game results do not validate the new logic.

## 2026-11-18 — static rejection of mode enum as story identity

Read-only disassembly and the new scripts/audit_story_identity.py confirmed
the pinned EXE's exact mode-string branches: PLAYING_CHAPTER maps to constant
zero, alongside MAIN_MENU=1, IDLE_GAME=2, PAUSED_GAME=3, PLAYING=4,
BROWSING_FAMILY_ALBUM=5 and END_LIST=7. This source cannot distinguish chapters.
The audit also pins the save interface getter/caller and main-object clear:
interface this=main+4, so interface+0x68 is main+0x6c, whereas the adjacent
operation-clear branch writes main+0x68. It does not establish candidate reset.
The setter-shaped function's calling context and argument meaning remain unknown.

The static audit executed against the full-SHA256-pinned local EXE and matched
the bounded evidence. Its report retains false for candidate meaning/lifetime
and world identity. No game, DLL, test fixture, suite, sockets or UI input was
started. No runtime identity or two-PC mission acceptance is claimed. Updated
STORY_IDENTITY.md removes the obsolete input-block statement and records the
continuing Cyberpunk test hold plus the actual native UI instructions.

## 2026-11-18 — trace asynchronous Continue load request (static only)

Pinned file analysis traced constructor 0xb66690 installing interface vtable
0x1974668, slot+0x30 -> 0xb2d020 and the existing 0x450ca3 virtual load call.
The wrapper stores its mode argument through 0xb2c740 in interface+0x64 and
sets phase 8; it creates a 12-byte operation of vtable 0x197343c with the save
ID at +8, then installs the pointer through 0xb2c9c0. Extended exact-byte audit
matched these sites against the full pinned EXE hash. This is an asynchronous
request path, not proof of current loaded world/checkpoint or load completion.

Added a bounded external load-operation reader to story_audit. Unknown class,
unreadable/overflow/misaligned pointers, unsupported phase/mode/ID and visible
changes cannot produce an observed request. The reader acquires no references;
rereading does not prove atomicity or prevent ABA. IDs/modes remain private;
console output exposes status and loaded_world_verified=false only. Existing
unverified candidate and asset-root diagnostics remain distinct.

Authored synthetic cases for recognized request, unknown interface/operation,
no operation, bad pointers, unsupported phase/mode/ID and changed reads. They
were NOT executed under the continuing test hold. Only static file auditing
and Python source compilation were performed; no live process reads, game,
DLL, suite, fixture, networking or UI input occurred in this stage. No runtime
PASS or loaded mission identity is claimed.

## 2026-11-18 — candidate write precedes SAVE result (static only)

Further pinned disassembly found a direct writer to candidate 0x1cc39c4:
operation vtable 0x1973444 -> 0xb407f0. In local state zero, byte+4 zero and
nonzero input+8, 0xb40835 stores that input before the later SAVE data interface
call/result branch at 0xb40850..0xb40865. Both subsequent branches follow the
write. This establishes a concrete reason to reject candidate equality as
completion evidence, independent of the previously unknown setter's callers.

Traced a retry/continuation chain from 0xb3f770 installing vtable 0x19740d8,
slot+8 -> 0xb2ce40, whose local state 0x10 path reinstalls a 0x1973444 operation
with the saved ID/byte and state zero. Exact semantics of SAVE outcomes, other
writers and later world restoration remain unverified. AL return values and
local operation states are not mission/checkpoint identity.

Extended scripts/audit_story_identity.py with bounded exact instruction/vtable/
literal checks, executed read-only against the full-SHA256 pinned file; all
matched. story_audit/inspect_runtime now publish the non-sensitive qualifier
candidate_may_precede_completion=true alongside the still-unverified candidate.
Source compilation completed without executing modules. No suite, fixture,
game, DLL, live process read, network peer or UI input was started. Runtime
checks remain deferred under the Cyberpunk hold; no gameplay PASS is claimed.

## 2026-11-18 — CGame Restore callback audit (static file only)

Traced CGame registration at 0x5c9e8b: manager accessor 0x1167210, AddAction
slot+0x1c, event 19, priority 500, singleton object and callback 0x5977a0.
The callback forwards to virtual slot+0x30. The installed table 0x190af18
(constructor write 0x5c21dd, Open slot+0x14 -> 0x5cd760) maps this slot to
0x58a740, an empty ret-4 handler. This particular handler cannot provide a
mission/checkpoint ID. Other modules' restore handlers, the event producer
and payload layout remain unverified; no conclusion about them is implied.

Added bounded exact-byte/vtable checks to audit_story_identity.py and ran only
that read-only file audit on the full-SHA256 pinned EXE; all matched. Source
compilation was performed without executing the module. No code was added to
dereference event context, call engine functions or change observer behavior.
No game, DLL, live process reads, suites/fixtures, sockets or input were run.
The Cyberpunk test hold remains active. Next static work targets the SAVE data
consumer and actual restoration rather than this empty CGame notification.

## 2026-11-18 — SAVE service and read-layer dispatch (static only)

Resolved the object behind global 0x1ac0c48 through allocation/constructor
0x11cafd0 and installed vtable 0x190c70c. Slots +0x0c/+0x10 resolve to
0x11c55f0/0x11c59a0 respectively. Read-side 0x11c59a0 checks parsed header
version 24/25 and requested ID, then processes stream layers. This is only
part of the path, not proof of restored/current mission or checkpoint.

Mapped layer names from 0x11bc8f0 and the 11-layer initialization loop: indices
0..10 plus LAYER_LAST=11 marker. Layer lookup selects node+0x10, reads layer+0x1c
and calls 0x11c4600. That dispatcher selects a registered callback object and
invokes vtable+4 with a stack context. Concrete callback registrations, field
restoration, ownership/lifetime and overall completion remain unaudited.

Extended pinned exact-byte file audit covers factory/vtable, read validation,
lookup/dispatch sites and all enum branches/strings; it matched the local EXE.
Python source compilation only otherwise. No game, live process inspection,
DLL/API calls, test fixture/suite, networking or UI input were executed. The
user's test hold continues. SAVE_RESTORE.md records the next source-level
reverse-engineering targets without claiming runtime/two-PC acceptance.

## 2026-11-18 — missionNumber/missionPart script progress candidate

Resolved a concrete Application callback: constructor installs subobject+0x94
vtable 0x18f4f54; 0x473810 registers it for layers 10/0/1/8/4/9; read slot+4
forwards via 0x507f60 to Application method 0x507b30. Its INIT branch restores
missionNumber and missionPart through global 0x1abfe24 / setter 0x5981d0.
Writer 0x473910 reads the same names with 0x598040. Static exact-byte audit now
checks this chain plus type/stride/count/name-pool access and the native 100-slot
limit. All bounded pinned-file checks matched. This is static provenance;
chapter/checkpoint mapping, lifecycle freshness and completeness remain unknown.

Added production external reader script_progress_audit.py to story inspection.
It validates pointer/count, bounds records/name reads, requires type-2 finite
integral values 0..65535, preserves native reverse-lookup duplicate priority,
and rejects visible changes via rereads. The pool cap of 4096 is a conservative
diagnostic budget, not a claimed engine capacity. Outputs remain empty on missing
or invalid data. Readback is not atomic and does not exclude ABA or stale values.
Public output contains only status/false verification flags; raw pair is private.

Authored synthetic valid pair, missing/count, nonfinite/fractional/range/type,
name-offset, duplicate-priority, changed-record and bad-manager cases. They were
NOT executed. Source compilation completed; no module invocation except the
read-only EXE audit, no game/live process reads, DLL calls, input, network or
test suites/fixtures. Runtime reader checks and real campaign correlation are
deferred under the continuing Cyberpunk hold; no new gameplay PASS is claimed.

## 2026-11-18 — native script progress observation (compile only)

Implemented the production C bounded script-variable reader using an injected
read callback: at most 100 records / 200 name fragments, conservative 4096-byte
name budget, type-2 finite integral 0..65535 values, native reverse-lookup
duplicate priority, and repeated singleton/count/record/name comparisons.
Non-observed results always zero the output. No retained engine pointers,
game API calls, writes, world authorization or network protocol changes.

Observer telemetry now logs private script_progress records on relevant
lifecycle/save/load events and at most 2 Hz during tick/paused/menu tick. Each
includes event/epoch/initialized/loaded/paused and explicit false world/checkpoint
flags; missing/invalid states omit the numeric pair. Menu/boundary observations
may contain stale script values and exist to correlate/reset their lifetime.
Telemetry fixture uses a zero script slot and cannot read game singleton data.

Full pinned clang 23.1.5 warnings-as-errors build completed in build/script-progress.
The new production-source C fixture was compiled only; prepared cases cover
normal pair, count bounds and full 100-slot scan, bad type/numeric/name bounds,
missing/duplicate precedence, changed names/records/singleton, unreadable data,
bad pointers and cleared output. test-probe now includes this fixture for future
authorized runs; it was NOT invoked. The existing Python reader cases also
remain deferred. Rechecked static exact-byte EXE audit; evidence matched.
No game, DLL/fixture execution, test suite, live process read, sockets or input
was performed. Full in-game correlation and two-PC acceptance remain unverified.

## 2026-11-19 — static chapter-3 Lua inspection

Added scripts/read_lua51.py and docs/SCRIPT_CHECKPOINTS.md. Parsed six local
original Lua chunks as data: four from m03_main and one each from m01part2 and
m02_mission_end. Manually inspected bounded prototype instructions for chapter-3
part selection, five setters, 11 save event entries, SaveGame branches and the
message-field forwarding call. Hashes and precise prototype/PC evidence are in
the new document. Final reference reports for M03Parts/M03Saves retained false
execution/reachability flags. Python source compilation completed.

This was static asset research, not a test suite or runtime acceptance. No Lua
VM/game/DLL/fixture execution, live process read, UI input or networking. No
original assets or extracted chunks are published. The Cyberpunk test hold
remains active. Checkpoint uniqueness, runtime reachability, other chapters and
two-PC mission/failure/saved-progress acceptance remain unverified.

## 2026-11-19 — native Lua SaveGame request trace

Inspected bounded file disassembly and registration/vtable data under the full
pinned EXE hash. Traced binding 0xad6620, slots 0x34/0x38, generated ticket counter,
record insertion/copy and lookup. Extended audit_story_identity.py exact-byte
checks matched the original EXE; public output keeps durable completion false.
The numeric Lua argument is distinct from its generated request ticket. The
Lua C function's return 1 counts results; it does not prove save completion.

This is static evidence only. No game/Lua VM/DLL/fixture/suite execution, live
memory, network or UI input. Completion-object semantics, queued-save consumer,
actual disk commit and runtime/two-PC acceptance remain unverified. User test
hold remains in force.

## 2026-11-19 — static queue notification trace

Read bounded pinned EXE disassembly/data and extended exact-byte audit for
main update/dispatch slots, operation creation, state-9 target, managed-object
notification, queue erase/next-record forwarding and object flag mutation.
The audit matched the original EXE hash and new sites. No game/test suite/
fixture execution, live process read, network or UI input was performed.

The observed object flag transition and queue completion do not establish disk
success. Backend commit/error, listener semantics and full waiter protocol
remain unverified. Main offsets differ by four from interface offsets; the
new SAVE_RESTORE.md section uses main offsets throughout. Runtime correlation
and two-PC mission/failure/host-progress acceptance remain pending under the
continuing user test hold.

## 2026-11-19 — backend stream writes and error returns

Pinned static analysis identified factory/table 0x11cb760/0x190c6b4 for global
0x1ac0c50 and writer 0x11ca700. Three stream-write byte counts are checked;
observed returns are -2 for setup/open/preflight failure, -1 for count mismatch,
and 0 on the normal close/catalogue-update path. Operation 0xb40948 branches
on nonzero. Detailed addresses/limitations are in SAVE_RESTORE.md.

New exact-byte audit initially refused one span whose start was specified
inside an instruction; corrected to 0x11ca811, then the complete pinned-file
audit matched. Python source compilation completed. No game, fixture/suite,
live process read, input or network was executed. No runtime PASS is claimed.
Concrete filesystem close/flush, trailer meaning, catalogue contract and final
error delivery are unverified; durable completion flags remain false. The user
hold continues and two-PC mission/failure/saved-progress acceptance is pending.

## 2026-11-21 — external coop-anything research (no execution)

User requested review of cranebeaker/coop-anything. Read current web-served
README/directory, effect-capture design, ownership README/header/implementation,
flight README/header, chaos/hot-reload docs and upstream MIT license. Also read
linked JustAddMP/MafiaDE README/developer notes and core README. No immutable
upstream commit obtained. COOP_ANYTHING_REVIEW.md separates source inspection,
documentation claims, unverified applicability and next integration steps.

The escalated clone call expired during automatic approval review; retry in the
sandbox failed DNS. Continued read-only using web sources. No installation,
upstream code import, game/test suite/fixture/MCP execution, live memory or UI
input. Compatibility with our pinned x86 build and two-PC Radmin acceptance
remain unverified. Existing Cyberpunk hold remains active.

## 2026-11-21 — world agreement implementation, compile only

Implemented pure C OFFER/ACK/COMMIT state machine and integrated it into ENet
session, fixed-width wire codec and worker/game mailboxes. Protocol 2/module
0.0.4 is incompatible with old builds. Controls carry a host generation, local
load epoch and 32-byte supplied identity; STATE carries host generation. Added
strict read API for actor consumers and read-time world/local/remote freshness
checks. Publication revision invalidates old worker reads and preserves a
withdrawal/replacement coalesced between polls. No game-world producer is
invented: actual observer cannot authorize actors until one is verified.

Full build-probe with pinned LLVM-MinGW 20261104 / clang 23.1.5 i686, -Wall
-Wextra -Werror completed in build/world-agreement after the final code change.
New test_world_sync executable compiled; its cases cover handshake ordering,
withdrawal, different world, stale generation, guest load between ACK/COMMIT,
role/token errors and exhaustion. Wire fixtures cover 64-byte controls and
52-byte STATE; remote snapshot fixtures include world TTL/future timestamps.
Authored a two-process synthetic world scenario in session_peer mode 5 and
wired it into test-network. NONE of these tests/suites/fixtures was executed.
Reviewed diff and encoding/role/epoch/worker publication paths statically.

No game, test program, live process read, socket session or UI input was run.
There is no new runtime PASS. World identity provenance, guest load, actual
Human control and all two-PC mission/failure/saved-progress acceptance remain
unverified. User's Cyberpunk test hold remains active.

## 2026-11-21 — remote actor controller and MoveDir field provenance

Implemented backend-driven remote_actor lifecycle/motion controller and compiled
it into observer. No game callback attachment or native engine backend is present.
Authored/compiled test_remote_actor with owned callback data for world gating,
velocity cap/hysteresis, far-target refusal, retirement on identity replacement,
delayed removal without repeated release, refused stop, invalid/stale source and
creation retry interval. Wired it into the deferred test-probe suite. It was
NOT executed. Controller status does not prove native motion or physical stop.

Pinned static EXE inspection resolved BSS descriptor 0x1cbebb0's initializer
0xef5ad0, start 0x9807a0 and tick 0x937050, plus vector/multiplier/mode-byte
transfers and the separate 0x300 control mask. Expanded audit_human_commands.py
matched the installed full EXE hash and all new bytes; runtime_control_verified
and ai_ownership_verified remain false. Full pinned i686 clang 23.1.5 build
with -Wall -Wextra -Werror completed in build/remote-actor. A final source comment
clarifies velocity units; no executable behavior changed afterward.

No game, fixture/suite, engine call, live process read, input or network session
was run. Safe native backend, actual loaded-world producer and two-PC Radmin
mission/failure/host-save acceptance are still unverified under the test hold.

## 2026-11-21 — native command binding candidate, compile/static only

Added guarded MoveDir owner with real pinned native bindings, separate null
allocation check, explicit caller and consumed argument references, conservative
queue guards, restricted field updates, cancel request and caller-only release.
No game callback calls it. It is not an enabled remote actor backend.

Full i686 clang 23.1.5 warnings-as-errors build completed in build/native-move.
The full-hash read-only audit matched new allocator/ref constructor/ClearMove
sites. Inspected emitted CoopMoveCommandCreate in the built PE: allocator cdecl
caller cleanup, ref/init thiscall ECX, and AddCommand ECX=Human with reference
then type pushes and callee cleanup. No program/DLL/fixture/suite was executed.

Reserved-storage zero initialization, pre-initialization before AddCommand's
initializer, OOM/exception behavior, actual reference balance, exclusive AI
ownership, cancellation/physical stop and unload remain unverified. Cancel and
caller ref release are explicitly not destruction/stop evidence. There were no
live process reads, game/input/network sessions. Cyberpunk hold remains active;
full campaign and two-PC mission/failure/save acceptance are not achieved.
# 2026-11-21 — Human AI lifetime/update static evidence

Extended scripts/audit_human_commands.py with constructor population of +0xa8,
controller owner/vtable, preparation, embedded update and unconditional teardown
dereference sites. Read-only full-SHA audit succeeded on the pinned original
EXE; output remains ignored at work/story-metadata/human-ai-audit.json. Bounded
disassembly identified the controller+0x44 update and vtable+0x14 subsystem path.
This rules out nulling +0xa8 as an ownership policy, but does not establish a
safe native AI pause, exclusive movement ownership or physical stop.

Only Python static file inspection/disassembly ran. No game, suite, fixture,
native binding, live process read, input or network session executed; Cyberpunk
hold continues. No production actor/AI behavior changed. Existing native build
was not rebuilt because changes are audit/documentation only. Two-PC story,
failure/restart and host-progress persistence acceptance remain outstanding.

## 2026-11-21 — native command owner reentry barrier (compile only)

Added per-owner barrier around Create/Input/Cancel/Release, covering guard and
native callback reentry before creation's first ref and after release transfers
the ref. Recursive operations refuse before guards and preserve all owner fields.
Added a production-linked refusing-guard regression fixture for all four outer
operations and wired compile/run scripts; fixture NOT executed under user hold.

Pinned full x86 clang 23.1.5 warnings-as-errors compilation completed in
build/move-reentry. No game/fixture/suite/live-memory/input/network execution.
Reviewed return paths: every normal internal return flows through barrier
release. This is not a cross-thread lock or native exception recovery. Actual
allocation, reference lifetime, AI exclusion and physical stop remain unverified.
Further static tracing found C_AIControlMessage/OnAIControl registration names,
but no verified reversible AI disable operation; no guessed binding was added.
Full campaign and two-PC mission/failure/host-save acceptance remain outstanding.

## 2026-11-21 — AI lifecycle pairing and bounded reader

Pinned read-only audit passed after adding Human activation -> controller+0x10,
paired deactivation -> controller+0x14, adjacent physics deactivation and
ScriptSetControlStyleStr -> concrete Human stub sites. Output is ignored at
work/story-metadata/human-ai-lifecycle.json. Static evidence rules out whole-Human
deactivation and this SDK control-style wrapper as AI-only movement policies.

Added production human_ai.c/.h read-only snapshot with full GUID/type, vtable,
two back-pointer, arithmetic/readability and repeated relevant-state checks;
refused output clears. Opt-in -TestHuman logs statuses at Setup/Activate/release,
agent presence and raw mode flags. These do not prove AI exclusion or stop.
Full pinned x86 clang 23.1.5 warnings-as-errors build completed in build/human-ai.
Source/error paths reviewed; no new or existing fixture/suite executed. No game,
live memory, input, network or native binding was run. No live result is claimed
for these new observations. User hold remains; actual Human backend, loaded-world
producer and two-PC story/failure/restart/host-save acceptance are outstanding.

## 2026-11-21 — scripted AI movement candidate

Compared local Mafia2-Online network spawn/movement sources and m2o-reborn's
ScrMoveV binding with pinned EXE disassembly. Confirmed Human network path,
five-argument thiscall/ret-20, 0x8c AI command creation, correct two-word command
submission through NewCommand, Sync user data and separate intrusive Sync ref.
Extended full-SHA read-only byte audit passed; JSON stays ignored at
work/story-metadata/script-move-audit.json. No upstream implementation copied.

Added script_move.c/.h Start and caller-ref Release candidate, guarded and
reentry-protected, compiled into observer without any callback/caller. Full
pinned x86 clang 23.1.5 warnings-as-errors build completed in build/script-move.
Generated PE inspection confirms ECX=script and five native argument pushes
with callee cleanup. No game, suite/fixture, native call, live memory, input or
network execution. Handle creation does not prove AI acceptance or movement;
Release is not cancellation/stop. OOM, supersession, arbitration, cancellation,
ref balance, physical stop and teardown still need acceptance. User hold remains;
two-PC story/failure/restart/host-save and full campaign are unverified.

## 2026-11-21 — scripted movement state observation, compile/static only

Traced command state setter/conditional changer, owned Sync-wrapper release,
single four-byte AI refcounter allocation and typed userdata destruction.
Identified target-specific task/queue removal at 0xf95100; its concrete owner
and physical stop are not proven, so no cancel binding was added. Flag+0x0d
serialization evidence prevents assuming the old raw write is a valid cancel.
Extended full-SHA read-only byte audit passed; output is ignored at
work/story-metadata/script-state-audit.json.

Added pure callback state reader and guarded/reentry-protected Observe API,
with pointer/type/counter/link checks, repeated snapshots and cleared refusal
output. Sync flags report byte+8 only, avoiding uninitialized adjacent bytes.
Authored/compiled synthetic fixture for raw states, malformed ownership chain,
overflow/unreadable counter and changing snapshots. Fixture NOT executed.
Full pinned x86 warnings-as-errors compilation completed in build/script-state.
No game/native binding/test suite/fixture/live memory/input/network execution.
No actual command cancellation, physical stop or two-PC gameplay claimed; user
hold continues and all campaign acceptance requirements remain outstanding.

## 2026-11-21 — scripted command end request candidate

Added guarded/reentry-protected CoopScriptMoveRequestEnd using pinned native
conditional command state change to 3. Full typed owned Sync/command chain and
remaining Sync link are required for raw state 0. Existing states 1..3 stay
intact and return TERMINAL_OBSERVED. Caller-owned Sync ref is preserved; no
queue/global pointers, raw +0x0d or counters are written directly. No callback
calls the candidate and neither result is accepted as physical stop.

Extended read-only full-SHA byte audit passed for native task result propagation,
message teardown's state-3 request and secondary-interface adjustment in the
alternate targeted-removal path. JSON remains ignored at
work/story-metadata/script-end-audit.json. Full pinned x86 clang 23.1.5
warnings-as-errors build completed in build/script-end; emitted PE confirmed
ECX=command, push 3, callee cleanup for native 0xf5d290.

No game/tests/fixtures/native calls/live memory/input/network executed. The
state-0 native branch only writes state 3, so scheduler cleanup, Sync Done,
reference balance and physical stop after this request are unverified. Targeted
removal owner remains unverified/unbound. Hold continues; no actual backend or
two-PC story/failure/restart/host-save/full-campaign acceptance is claimed.

## 2026-11-23 — tests resumed and current Human accepted locally

- The user explicitly resumed work/tests and authorized needed tests on the second display. This supersedes the earlier Cyberpunk testing hold. Baseline: development 607c678. Work branch: codex/resume-20261123-tests.
- Revalidated the installed GOG EXE hash and the avatar, Human-command and story-identity pinned byte audits. Rebuilt all x86 targets with the pinned LLVM-MinGW compiler and warnings treated as errors. All six Python test scripts, eight production C fixtures and all nine loader/profile checks passed. The previously compile-only world, remote-actor, command queue, script progress/state and reentry fixtures have now executed successfully; fixtures remain distinct from game acceptance.
- First complete network run failed at the final worker/mailbox summary: the guest had already observed active then inactive data, but its peer exited before the guest's deadline. ReadRemote correctly zeroed its output. The fixture printed that final zeroed received counter and was rejected. Production networking was unchanged. The fixture now preserves the highest accepted counter observed during a valid snapshot, separately from the current snapshot. Its zero-output-after-stop assertion is retained. After rebuilding, the complete network suite passed, including synthetic world mismatch/recovery, receiver expiry while transport stays live, compatibility rejection, reconnect and producer-stall handling.
- Installed the repository's authorized test save into a fresh isolated profile. A normal-menu Enter series loaded the snowy story scene, visually confirmed with Vito/HUD. Window placement reported DISPLAY2, client 1280x720 and full work-area containment. Loading began after 6.811 seconds / 17 keys; the first player sample was detected after 8.247 seconds from the Enter series, not EXE launch.
- The current -TestHuman build reported creation_registry_confirmed, Setup/Activate, valid AI status 3 after Setup/activation/before release, and valid queue status 4 both after activation and at five seconds (7 free / 1 used). The retained movement reference changed from absent to present/queued while the game simulated the actor. These observations do not establish AI suppression or remote control. The captured screenshots establish the playable scene; this run does not independently establish visual identification of the created Human among pedestrians.
- One Release was requested and returned, followed by registry status PRESENT then ABSENT and removal_registry_confirmed. No lost-handle or removal-timeout result occurred. No raw movement or scripted-command binding was called. Vito moved under authorized bounded native Up input. The game log contains 174 player samples, position ranges 8.21225 / 9.16119 / 0.06980 and one game thread, all within loaded mission state.
- Two passive loopback windows received 287 and 385 active samples. Their position ranges were near-stationary (0.01756 / 0.01416 / 0.00003 and 0 / 0 / 0); they did not overlap the meaningful native movement. Do not claim this run carried that movement to the passive peer. Earlier movement-transfer evidence remains historical. Neither peer is a second game client.
- Native immediate quit completed in 1.656 seconds. Launcher and child exit codes were zero; original_profile_changed_files=0. No diagnostic Mafia II process remains running. One sampled epoch does not pass the strict two-load lifecycle acceptance.
- Current full-campaign readiness estimate is approximately 10%, a subjective engineering estimate rather than a measured completion rate or delivery forecast. A controlled remote avatar, verified loaded-world producer, two-PC Radmin gameplay, combat/vehicles/NPC authority, mission failure/reload and durable host campaign progress remain incomplete.

## 2026-11-23 — second isolated Human lifetime and overlapping motion receiver

- Reused the unchanged accepted build and authorized isolated seed in another
  game process on DISPLAY2. Normal-menu entry used 16 Enter keys; load started
  after 6.414 seconds and the first sample appeared after 7.732 seconds from
  that series. A fresh screenshot confirmed Vito and HUD in the snowy scene.
- Creation/Setup/Activate and one Release again passed registry identity and
  eventual absence. AI status was 3 at all three phases (agent present; raw
  flags 200). Both command-queue snapshots were valid, 7 free / 1 used;
  the retained reference appeared/was queued by five seconds. Raw flags do
  not establish AI suppression. No new native movement/end call was made.
- A six-second passive loopback peer was started and its ready_guest output
  observed before bounded native Up input (1000 ms), all in one sequential
  orchestration to guarantee overlap. Native input reported released=1 and
  focus_retained=1. The peer exited 0 with one connection, received=104,
  active_samples=104 and axis_ranges=1.33240,4.84363,0.00959. The following
  screenshot confirmed Vito's changed position. This is real moving-player
  telemetry into a passive peer, not two game clients or remote actor control.
- Immediate native Alt+F4/Enter quit completed in 1.594 seconds / nine Enter
  keys. Launcher and child exits were 0; original_profile_changed_files=0.
  Two separate process lifetimes do not prove in-process mission reload,
  physical cancellation, reference balance or complete campaign acceptance.
- Updated current instructions to supersede the older testing hold and to
  separate executed fixtures from still-unexecuted native movement bindings.

## 2026-11-23 — concrete script task and native abort request correction

Pinned SHA256 was revalidated. Static tracing connected command type 4 through
the registered C_AIAction_SetMoveToBBRec descriptor/factory to task vtable
0x197ce28. Its update reaches 0x40f340, which reads command+0x0d through
0xf3c9b0 and requests task result 3 when that byte is zero. Native task result
propagation, scheduler cleanup slot+0x1c, child listener/record cleanup and
command reference destruction were traced; see SCRIPT_MOVE.md for exact sites.

RequestEnd previously changed command state to 3, which does not clear that
abort predicate. It now verifies/calls the native setter 0xf3c9a0 on the same
guarded, linked raw-state-0 owned command, preserving command state and owned
Sync for native asynchronous propagation. It remains unwired and unexecuted in
game. END_REQUESTED is not physical stop, cancellation completion or deletion.

Full pinned clang 23.1.5 warnings-as-errors build passed. The extended full-SHA
byte audit passed. Emitted wrapper inspection confirmed command in ECX and
the no-stack-argument call to 0xf3c9a0. New production-linked refusing-guard
fixture passed all four outer/recursive operations and reference preservation;
it makes no native calls. Updated state-reader fixture passed zero abort byte
with still-linked raw-state-0 command and refusal when that byte changes between
snapshots. No real-game native command or two-PC test was performed in this stage.
The production fixture group (nine executables, including the new owner test)
and all nine loader/profile checks passed. The changed state-reader fixture
was rebuilt and rerun after adding the abort-byte snapshot cases. Network
code/protocol did not change; the previously passed network suite was not rerun.

## 2026-11-23 — first offline ScrMoveV/abort/Sync release in game

Added explicit -TestHuman -TestScriptMove acceptance with networking forbidden
in launcher and DLL config. Local guard requires redirected profile, pinned
base/game thread, loaded unpaused lifetime and owned registry Human; it grants
no shared-world authority. First early launch refused because redirection had
not yet been observed before the game's profile query. The check now remains at
creation/each native guard, rather than the early export. No game input was sent
to that refused launch. Full pinned rebuild and targeted state/reentry fixtures
passed; child-command copy/update byte audit passed.

The actual game run used the isolated seed on DISPLAY2. Initial menu input
stopped on lost focus/user input; after reobserving the main menu, three Enter
keys reached loading in 1.125 s and first player data in 2.486 s. Screenshot
confirmed the snowy story scene. One owned Human requested mode-2 movement with
a six-unit X offset at five seconds; native Start returned HANDLE_CREATED.
Eight samples before the 1.5-second abort span 0.59958 / 1.28016 / 0 units.
End returned END_REQUESTED; command state became 2 and abort byte 0. However,
42 samples at >=3.5 seconds span 0.16166 / 8.35553 / 0.04629 units: physical stop
FAILED acceptance. Do not infer that remaining movement is necessarily the same
command; resumed autonomous AI and remaining child work are not distinguished.

On retirement, End returned TERMINAL_OBSERVED, Human Release was followed by
registry-confirmed absence, and owned Sync release returned REFERENCE_RELEASED.
Native immediate quit took 1.609 s / nine Enter keys. Launcher/child exits 0;
original_profile_changed_files=0. No two-PC/native remote backend test occurred.
