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
