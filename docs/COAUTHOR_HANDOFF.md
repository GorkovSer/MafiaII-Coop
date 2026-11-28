# Следующие действия для Codex — 2026-11-28

## Разделение работы и поиск игры

Соавтору передана отдельная ветка codex/avatar-smoothing: remote_timeline.c/h,
собственные тесты и docs/AVATAR_SMOOTHING_HANDOFF.md; без изменения launcher,
session/game_bridge/protocol и без самостоятельного merge. Этот блок меняет
только launcher/game discovery/упаковку/документацию, timeline не трогается.

Добавлен launcher-game.ps1: нормализация root/pc/EXE/кавычек и кнопка «Найти»
для ограниченного поиска Games/Steam libraries. UI проверяет закреплённый hash
до worker; гость не может ввести свой адрес или loopback. Поиск не подменяет EXE,
не сканирует диски рекурсивно. Проверки test-launcher-game.ps1 пройдены в PS7 и
Windows PowerShell5.1, реальная установка на этом ПК найдена. Новый helper включён
в упаковку. Пользователь прислал скриншот предыдущего окна со status-кодом:
элементы помещаются; новая кнопка поиска ещё отдельно визуально не проверена.

## Текущий блок: состояние подключения и автоматический итог сессии

Новый одиночный smoke завершён: окно1280×720 и контрольная сцена подтверждены
визуально, reader на реальном журнале показал waiting_guest/paused/waiting_guest.
Автоматические result/session-report сохранены, child/launcher0/profile0,
avatar_created=false, штатный quit1609мс. Подготовленный CoreAudio watcher
приглушил 1 сессию Mafia и завершился вместе с ней. См. TESTING.md.
UI текст отдельно на экране не проверен, новый пакет на двух ПК не принят.

Пользователь повторно потребовал тесты только в окне и без звука. Visual entry
теперь задаёт Windowed=true независимо от SecondScreen, меняя только video config
изолированного профиля до запуска. Для агентских тестов добавлен отдельный
tests/watch-owned-game-audio.ps1: CoreAudio инициализируется до старта, затем
приглушаются только аудиосессии свежего закреплённого PID, прочитанного из явного
журнала запуска. Наблюдатель тестовый, в пользовательский ZIP не включён.
Первый новый worker smoke в меню завершён exit0/profile0 и правильно сообщил
avatar_created=false. Он выявил fullscreen из контрольного video config; поэтому
оконная настройка теперь отделена от переноса на второй монитор.

Добавлен scripts/launcher-status.ps1: ограниченное последовательное чтение
единственного журнала по явной session-info.json ссылке worker. UI теперь
различает запуск/ожидание/связь до загрузки/созданный аватар/паузу/reconnect/
отказ/конец согласованной сцены. Гостю запрещён собственный IP в UI.
Сам reconnect в моде уже реализован; повторной независимой петли в launcher нет.
Worker сохраняет session-report.json после штатного завершения и отказывает в
успешном итоге при неизвестном exit, изменённом профиле или дисбалансе созданных
моделей. Новые вспомогательные файлы включены в package-visual-mvp.ps1.
Проверки: scripts/test-launcher-status.ps1 под PS7/Windows PowerShell5.1,
scripts/test-visual-report.ps1, production test_session_retry.exe.
Новая часть UI требует отдельного игрового прогона, не наследует приёмку старого
EXE только из успешного теста прежнего ZIP. Ранее выпущенный ZIP не заменять
вручную отдельными скриптами; для обоих ПК готовить одинаковый полный пакет.

## Сверка с хостом и следующий диагностический шаг

Четыре итоговых поля хоста теперь проверены по локальному JSON: child_exit_code=0,
launcher_exit=0, original_profile_changed_files=0,
model_lifetimes_balanced_after_exit=true, 12/12 моделей. Это совпадает с гостевой
сводкой `7005fc1` (10/10 и 5/5). Старый host журнал не показывает новых созданий
после scene_approval_ended=32; новый preview после replay не принят по журналам.
Сами гостевые журналы/JSON на GitHub не опубликованы, только сводка.

Добавлены временные отметки network событий и bounded timeline отчёта; см.
верхний раздел TESTING.md. Сессии/протокол не менялись. Тесты отчёта:
scripts/test-visual-report.ps1; интеграция: scripts/test-visual-network.ps1.
Новый build: build/network-timing-20261128 (локальный, не portable release).
Не смешивать его с готовым ZIP на одном ПК посреди парного теста.

Дальше: получить приватно старый гостевой probe.log/JSON для проверки доступных
свидетельств; для точного сопоставления disconnect/timeout провести отдельный
согласованный парный прогон одной новой сборки с синхронизированными часами.
Не исправлять reconnect вслепую по числу timeout. Не менять sandbox firewall:
обвинённое ранее offline правило фильтрует другую учётную запись, не Mafia.
Текущие игры завершены штатно. Ниже сохранена прежняя гостевая сводка.

## Гостевая приёмка двух ПК — новые результаты

Пользователь разрешил запуск командой «Старт» и сообщил об успешных визуальных
проверках на обоих ПК через Radmin: движение/поворот, пауза без дублей, выход и
повторный вход гостя, исчезновение модели при выходе хоста и перезапуск задания
с последующим новым запуском preview. Это сообщения пользователя, не независимая
запись обоих экранов. Агент проверил гостевые журналы и сохранил локальные JSON
отчёты `visual-session-report.ps1` после обычного выхода.

Для двух завершённых гостевых игровых сессий: `child_exit_code=0`,
`launcher_exit=0`, `original_profile_changed_files=0`,
`model_lifetimes_balanced_after_exit=true`. Циклы создания/удаления: 10/10 и 5/5.
Полные журналы, адреса и пути пользователя в Git не включены. См. новый раздел
2026-11-28 в TESTING.md. Версия — неизменённый launcher preview 20261127.

Следующий шаг соавтора: сопоставить эти четыре поля с отчётом ХОСТА и проверить
его локальные наблюдения/запись. Повторный свежий запуск после перезапуска задания
подтверждён пользователем, но отдельная новая гостевая сессия для него не найдена;
не представлять его как независимую журнальную приёмку. В последней гостевой
сессии также были disconnect/retry/timeout; причина и длительность относительно
действий игроков не установлены, непрерывная безошибочная связь не заявляется.
Без новых свидетельств не считать весь парный прогон независимо проверенным,
не повышать сборку до принятого релиза и не объявлять сюжетный кооператив готовым.
Никакой новой игры/фонового теста для оформления отчёта не запускали.

Далее сохранён исторический план 2026-11-27: его запрет запуска и отсутствие
результатов больше не описывают этот завершённый гостевой прогон. Проверка
свойств нового EXE-лаунчера целиком по docs/LAUNCHER.md всё ещё не завершена.

**Для установки/парного теста на другом ПК:** docs/CODEX_TWO_PC_RUNBOOK.md.
Пользователь поручает другому Codex поставить готовый ZIP с проверкой хешей,
подготовить роль гостя и ждать общей команды «Старт». Хост готовится аналогично.
Сейчас это публикация инструкции, а не запуск игр в текущем чате. Версию нельзя
незаметно менять на одном из ПК посреди приёмки; результаты собираются с обоих.

**Текущее поручение:** сначала опубликован этот план (`9f2742c`), затем локально
подготовлен отдельный GUI-лаунчер (scripts/coop-launcher.ps1 и launcher-session.ps1).
Вместо CMD добавлен нативный MafiaII-Coop.exe (GUI subsystem, asInvoker, своя иконка,
CREATE_NO_WINDOW для PowerShell), сборка scripts/build-launcher.ps1. Оформление:
белая serif-типографика, тёмная геометрия и красные акценты. Выбор порта удалён;
worker всегда передаёт 28960, старые сохранённые port/second_screen игнорируются.
Пользователь поручил опубликовать исходники и собранный архив, затем сам проведёт
проверку двух ПК. Публиковать как предварительную сборку, не как принятый релиз.
Инструкция пользователю/второму Codex: docs/TWO_PC_TEST.md. Агенту новый EXE/игру
пока не запускать; результаты пользовательской приёмки ещё не получены.
На главной README добавлены ссылка на Releases/Assets и инструкция запуска EXE;
исходники не выдаются за готовый архив, новый лаунчер остаётся непроверенным preview.
Старое окно GUI видно на скриншоте пользователя рядом с CMD. Новый EXE ещё не
запускался; сохраняется запрет открывать игру и выполнять функциональные тесты.
Галка второго монитора удалена вместе
с передачей параметра, добавлены кнопка «Крашлоги» и приватные JSON-отчёты ошибок в
work/crashlogs (это не нативные minidump). Правка выполняется только статически,
без запуска GUI/игры/функциональных тестов. Архив остаётся launcher-draft, а не
проверенным релизом. Runtime не изменён. Следующий шаг — разрешение на пакетную проверку
по docs/LAUNCHER.md; не возвращать внутриигровое меню и не заявлять соединение по
одному факту запуска скрипта. Публикация непроверенного лаунчера теперь прямо
разрешена пользователем для его самостоятельного тестирования.

База: `development`, проверенный runtime из `2e9a4a3` / PR #15–16. Цель сейчас —
приёмка визуального MVP на двух разных ПК; меню/лобби не возвращать. На одном ПК
два настоящих клиента уже прошли движение/поворот, паузу, выход/повторный вход,
перезапуск задания и чистое завершение. Это не готовый сюжетный кооператив.

1. На обоих ПК: одна и та же переносимая сборка с manifest/checksum, совместимая
   Mafia II Classic и одна сеть Radmin. Распаковать в короткий путь `C:\MafiaCoop`.
   Исходники GitHub сами по себе не заменяют собранный preview. Не менять EXE,
   исходные сейвы, защиту Windows и правила сети без необходимости.
2. Сначала хост запускает команду ниже со своим Radmin IPv4, потом гость — со
   **тем же IP хоста**. Путь игры заменить, если отличается. Порт у обоих 28960/UDP.
   На двух ПК НЕ нужны `ConcurrentHostSession` и `BackgroundVisualTest`.

   ```powershell
   .\scripts\start-visual-session.ps1 -Role host -Address 26.X.X.X -GameDirectory 'C:\Games\Mafia II Classic'
   .\scripts\start-visual-session.ps1 -Role guest -Address 26.X.X.X -GameDirectory 'C:\Games\Mafia II Classic'
   ```

3. Каждый запускает только свою строку, пропускает интро двумя Escape и выбирает
   штатное «Продолжить». Скрипт создаёт отдельный профиль из общего контрольного
   сейва. Сейчас сейв хоста НЕ передаётся: оба независимо загружают одинаковую
   сцену. Соединение автоматическое, кнопки в игре нет. Один игрок отходит от
   общей точки появления. Поздний гость допустим в той же исходной сцене.
4. За один сеанс: движение и поворот по очереди; несколько минут совместного
   наблюдения; пауза/продолжение каждого; обычный выход и повторный запуск гостя;
   выход хоста; в конце перезапуск задания. После смены сцены модели должны
   исчезнуть до нового запуска preview. Проверять дубли, зависшие модели и FPS.
5. Codex на каждом ПК наблюдает свою игру локально; доступа ко второму ПК у Евы
   автоматически нет. Согласовать «иду/поворачиваюсь/пауза», записать короткие
   видео обоих экранов с общей точкой отсчёта. Handshake/remote_active без видимой
   модели не считать успехом; пинг VPN не выдавать за задержку отображения.
6. После штатного выхода на каждом ПК выполнить `scripts/visual-session-report.ps1`
   с `-SessionDirectory <папка его сессии>`. Сопоставить видео и оба журнала:
   число созданий/удалений, отсутствие старой позы/дублей, exit 0 и profile changes 0.
   Приватные сырые логи/сейвы в публичный GitHub не добавлять. В TESTING.md записать
   краткий обезличенный результат; обновить этот handoff в том же коммите.
7. Исправлять только воспроизводимые дефекты этой приёмки. Затем удобный запуск
   и понятные ошибки соединения; анимация/никнеймы — следующий согласуемый этап.
   Общие миссии, бой, машины и передача сохранения требуют отдельной реализации.

---

> Latest checkpoint — 2026-11-27. The rejected menu/lobby experiment is absent
> from development; preserved only on local unpushed codex/native-menu-20261126.
> Two real clients now run on this one PC using explicit isolated diagnostics:
> a caller-scoped launcher mutex name and pinned focus-message handler bypass.
> Neither is enabled by normal portable launches. Default one-game guard remains.
> Native host 20261127-150020-095, guest 20261127-150051-428 and replacement guest
> 20261127-150320-411 reached visible career gameplay. Both remote models appeared;
> independent movement/facing, each player's manual pause/resume, guest exit/rejoin
> and host exit were checked. Nine creation/removal cycles balanced; all exits 0
> and original profile changes 0. See TESTING.md and VISUAL_MVP.md.
> Reduced visual MVP estimate ~85%, not a campaign co-op estimate. Still pending:
> real two-PC/Radmin routing and sustained visual quality/FPS. A follow-up pair
> (151254-545 host / 151323-601 guest, date 20261127) replayed the mission on each
> client: event 32 ended scene approval, no model respawn after reload, 1/1 owned
> lifetime each, both child/launcher 0 and profile changes 0. No game remains.
> Do not restore the menu or use backend Continue to enter gameplay.
> Next: finish those acceptance checks when a second PC is available; retain the
> fixed-scene limitation and default-off physical actor authority.
>
> Previous checkpoint — 2026-11-26 (resumed at the user's request; supersedes the
> historical pause note below). Visual guests now retry transport loss/timeout
> with a two-second delay. The local session policy is default-off outside visual
> guest mode; protocol/module remain 3/6. Observed protocol refusal blocks retries.
> Old pose, peer identity and scene agreement are cleared before reconnecting.
> One-load visual approval and physical-authority refusal remain unchanged.
>
> Pinned Werror build, new real-loopback retry fixture, full network suite and
> visual-isolation suite passed. Native session 20261126-230119-981: one actual
> guest game, three sequential synthetic hosts, late-host/replacement recovery,
> visible model shown to the user, pause/resume, nine complete ownership cycles,
> clean exit 1735 ms, child/launcher 0, original profile changes 0. No test process
> remains. Details and limitations: VISUAL_MVP.md. Simultaneous real-client and
> remote Radmin acceptance remain pending. Reduced visual MVP estimate ~75%; no
> campaign/combat co-op completion claim. No background automation was created.
> Latest checkpoint — 2026-11-24. The user requested publication after ten more
> minutes, then stopping work until later. No continuing/background work is
> scheduled. Resume only when the user asks.
>
> Published baseline: `9e1919d26934bb2faec555237d628d51e352f19c` (PR #12).
> Additional checkpoint: launcher window/cleanup diagnostics and the same-PC pair
> attempt. The second game naturally exited 0 before its own window; handshake
> alone is not two-client acceptance. Production one-game guard and original game
> files remain unchanged. Pinned Werror build, loader/profile/user-token fixtures,
> normal native host loading/exit and unchanged original profiles passed. See
> VISUAL_MVP.md for exact sessions and results. Only one PC is available; the
> next gameplay acceptance still needs a second real client. MVP estimate stays
> around 70% for the reduced visual-only scope, not a full campaign mod.
# Указания соавторам и текущее состояние

## Current portable checkpoint (2026-11-24)

Published network core: f75c1d3 / PR #10. Portable scripts now prepared/tested:
start-visual-session.ps1 (role/address, fresh isolated profile, checksums/path
limit/host bind address, PrepareOnly), package-visual-mvp.ps1 (11 files + manifest,
no original game assets/EXE, existing authorized save + ENet license).

Candidate extracted to C:\Temp\Mafia Coop 7fc562b6\MafiaII-Coop-visual-preview.
Real packaged GUEST / PID 2568 / session 20261124-044747-289 exited normally.
Visible network model accepted against synthetic HOST. Native mission replay
ended scene approval at event 32; second load finished, no new model. Exit 1641ms,
child/launcher 0, original profile changes 0. Both native roles are now separately
accepted against synthetic counterparts. NOT simultaneous two-game/two-PC proof.
Console package checks: path with spaces, host/guest/casing, Windows PowerShell5,
six bad endpoints, missing game, path-length refusal and changed-DLL refusal
before profile writes. Old overlong extraction is NOT used for native launch.

Private packaging reports: work/visual-package-candidate.json,
work/visual-package-smoke.json, work/portable-native-case.json. Dist ZIP is a
local preview artifact; final package should be regenerated at the published
source commit so its manifest points to that exact source. Synthetic helper
supports bounded 10-minute tests; only guest fixtures deliberately reconnect.

Remaining real acceptance: two actual clients on two PCs/Radmin and longer play.
User availability question about second PC remains unanswered. Assume only this
PC until answered; do not fake two-PC acceptance. Do not expand back to story,
combat, physics/AI or cars. Scope is the approved model-only first MVP.
Latest user workflow: finish code chunks, combine native checks in one session,
GitHub ~every20min with "обновила" + link; Computer Use only for concrete UI work,
continue console/code if interrupted. Do not repeatedly reread this document.

## Current accepted network-visual stage (2026-11-24)

Last published checkpoint b85afc9 / PR #9 is the offline visual lifecycle.
Local VisualNetwork code now passed one combined native batch after fixes:
session 20261124-042020-189 / PID 24020 EXITED, 88 model samples, six creations /
single releases / registry absences, zero timeline refusals. Stall/expiry/recovery,
peer reconnect, pause/resume rebind, independent local input and exit begun with
live model passed. Child/launcher exit 0, original profile changes 0. One game +
synthetic peer only; no two-PC claim. Wrong-scene refusal passed in previous batch.

Build visual-network-20261124. Baseline network suite and production fixtures
passed; visual scope fixtures passed match/mismatch, five bad configs, stable
receipt, physical authority isolation and stopped-zero. Latest pure visual event
fixture passed late paused-frame ordering and one-load scene consent. Two native
findings fixed: per-poll mixed-clock receipt timestamps and id4 relatching after
id35. New scene/reload approval now fails closed until process restart; that added
state rule has pure-event fixture acceptance, not a new level-switch game run.

Current target remains minimal visible model without animation/collision.
Next: simple portable launch package and longer combined checks; ask/await second
PC availability for genuine two-player/Radmin acceptance. An availability question
is pending; meanwhile assume only this PC. No new campaign/AI infrastructure.
GitHub every ~20 minutes, final-packet report in chat; only tested code. Computer
Use only for specific game tests/publication, never for idle code work.
See VISUAL_MVP.md for evidence and limitations.

## Current local checkpoint: native visual batch accepted (2026-11-24)

Published previous packet: 60e61bf (PR #8). New visual_test.c/.h and
-TestVisualAvatar now passed one real isolated career batch: visible model,
XY motion, in-place turn, stationary hold, registry-confirmed retirement and
unaffected local input; 30 samples, native exit 0, original profile changes 0.
Session 20261124-034633-203 / PID 18560 has exited. Build visual-mvp-20261124;
full build/loader checks and four invalid launch combinations passed. The new
visual heading reader was accepted in that same run. See VISUAL_MVP.md.
Next code chunk is network-driven visual application in one explicitly agreed
fixed scene, followed by one combined native batch, not many tiny game launches.
Current instruction: update GitHub about every 20 minutes, report each final
packet; use Computer Use only for specific game checks and publication. An
interrupted UI session does not stop console/code work. Do not reread handoffs
repeatedly. Physical Human stop remains unresolved and is outside this MVP.

## Срочная передача: упрощённый визуальный аватар согласован

Пользователь подтвердил: сначала видимая модель, двигающаяся/поворачивающаяся
по сети, БЕЗ анимации ходьбы и столкновений. Не продолжать дорогую борьбу с AI
ради первого MVP. Найден кандидат FrameWrapper type 0x37, allocation 0x88,
factory 0xac9cd0, vtable 0x196d6e0. Static byte audit прошёл; он не Human/Player.
Native visual SetPosition slot 0x7c -> 0x1164d40, SetDirection slot 0x80 ->
0x1175120. Generic setup/activate/removal требуют отдельной приёмки. НИ ОДИН
FrameWrapper native call пока не исполнялся. Следующий шаг: один изолированный
опциональный visual-avatar lifecycle test на клоне модели, затем сетевое
применение pose в фиксированной сцене. Не вызывать на нём Human Setup/AI API.
Реальное чтение нового heading также ещё ждёт игрового теста. Всё остальное
pose replication уже прошло перечисленные ниже сборку/консольные проверки.
Обновление сейчас затребовано пользователем до исчерпания лимита.

## Local MVP pose stage after published 50a6172

Protocol 3 / module 0.0.5 adds optional explicit visual heading to STATE, session,
mailbox and actor face callback. Pinned build/audit, five changed production
fixtures and network cases passed (mailbox expected-angle fixture corrected and
rerun alone, both sides 61 snapshots). Current source is under build/mvp-pose-20261123.
Visual heading reader and native face backend have not had in-game acceptance.
Do not claim a controlled guest or publish that as a playable mod. Work next on
the smallest owned visual avatar, avoiding new campaign/network infrastructure.
Computer Use only for a specific game test or one publication packet; continue
console/code work if UI is interrupted. Existing separate-player ownership and
scene authorization boundaries remain unresolved, not silently bypassed.

## Последнее решение пользователя: сократить объём до MVP

2026-11-23: сначала только два персонажа в одной фиксированной сцене,
подключение, появление, движение/поворот и штатное отключение. Бой, NPC, машины,
сюжетная синхронизация, катсцены, сейвы и reconnect отложены. Не продолжать
широкую инфраструктуру кампании. Ближайшее препятствие — управляемый отдельный
аватар; использовать готовую основу, если она действительно совместима и
сокращает работу. Приёмка MVP — два реальных ПК и видимое движение с обеих сторон.
Полный план сохранён как отложенный в ROADMAP.md; MVP не считать полной кампанией.

# Current local stage: Player control observer (2026-11-23)

After published 5bc93c3, added a read-only concrete Player control reader and
pinned class/factory/query audit. Full build, audit and loader/profile suite
passed; expanded production telemetry fixture passed. Isolated game session
20261123-231437-105 / PID 29900 was loaded manually by the user. All 100 control
samples were accepted, including modes 10, 0, 1 and 5. A one-second native Up
input moved Vito; input released and focus retained. Native quit took 2 seconds,
child/launcher exit 0, original profile changed files 0. No Player control method
or spawn was called. Native CUA window access was restored; stale internal browser
handles failed, a fresh IAB tab succeeded. Do not treat old node tool cancellation
as blocking all code work. Details: [PLAYER_CONTROL.md](PLAYER_CONTROL.md).
Full campaign readiness remains approximately 10%, subjective.

Последнее указание пользователя, 23 ноября 22:50 МСК: тесты и публикации снова
разрешены, GitHub обновлять примерно раз в 15 минут с отчётом в чат. Повторное
перечитывание инструкций и лишний расход пользователь запретил. Остановка
Computer Use не отменяет разработку/консольные проверки; не обходить остановку
UI альтернативным вводом. Публиковать только проверенный этап.

Foundation stage прошёл pinned full build, 6 Python scripts, 10 production
fixtures, 9 loader/profile checks, network suite и 4 presets/7 refusals.
Реальный LoopbackHost передал 111 движущихся player snapshots пассивному peer;
native exit 0, исходный профиль неизменен. Это не two-PC Radmin и не remote avatar.

Также подготовлен entity_map.c/.h: ограниченная таблица host network ID ->
локальный GUID для player/NPC/vehicle, без указателей/нативных вызовов. При
смене/отзыве мира старые записи уходят в retirement; новый мир не принимается
до подтверждённого отсутствия всех старых GUID. Отсутствие подтверждает внешний
registry callback, reentry на время callback закрыт. Это bookkeeping, не готовая
синхронизация NPC/машин и не разрешение на spawn. Теперь собран и проверен fixture.
Host-only allocator выдаёт session-scoped uint64 IDs без повторного использования
и отказывает при исчерпании. Входящие NPC/vehicle bindings разрешены только от
player 1 (хоста); локальные runtime GUID никогда не становятся сетевыми ID.

Текущая локальная работа: bounded receiver-local interpolation (8 snapshots,
100 ms delay, no extrapolation, immediate authority withdrawal), optional/off
actor integration, Loopback/Radmin diagnostic presets. Дополнительно сохранены
наблюдения refcounts/MoveDir после отмены и исправление resize-race размещения
окна. Основа интеграции — 167f01c. Ранее прерванный loader suite не считать
пройденным; новый полный прогон завершился успешно. Следующий участок —
проверяемая AI ownership/physical stop, затем настоящий backend и world producer.

Сводка обновлена: 23 ноября 2026 года. Основа текущего этапа:
`607c6786dbcd6812ce0756faf75a5356af56a0a0` в `development`.
Пользователь возобновил тесты и разрешил нужные проверки на втором мониторе.
World agreement прошёл fixtures, но не игровое согласование двух миров.
Этот хеш обозначает основу этапа, а не будущий HEAD документа.

Текущий участок: `codex/local-script-test-20261123`, offline scripted movement и приёмка
registry-guarded Human. Полная сборка, Python/C/loader проверки и сетевой suite
выполнены; исправлен только учёт полученных пакетов в bridge fixture после
обнуления снимка при отключении. В реальной игре подтверждены создание Human,
valid queue/AI observations и registry-confirmed удаление в двух отдельных
процессах; оба выхода 0, исходный профиль неизменен. Пассивный peer получил
104 active snapshots во время движения Вито. Offline TestScriptMove впервые вызвал
Start/End/Release: команда принялась и перешла 0 -> 2, но Human продолжил движение.
Физическая остановка НЕ принята. Далее: отличить оставшуюся дочернюю задачу от
возобновившегося автономного ИИ и добиться проверяемой отмены/остановки
scripted movement; не открывать remote/world gates по результату локального теста.

Новое статическое основание: command type 4 -> SetMoveToBBRec -> task vtable
0x197ce28; update читает command+0x0d через 0xf3c9b0 и ставит task result 3.
RequestEnd теперь вызывает штатный setter 0xf3c9a0, сохраняя command state/Sync;
прежняя запись command state 3 отменой не подтверждена и заменена. Конкретные
пути scheduler cleanup найдены, но физическая остановка/баланс ссылок в игре
ещё не проверены. См. SCRIPT_MOVE.md; backend по-прежнему не подключать.
Pinned full build/audit, девять production fixtures и девять loader/profile
checks прошли; новый guard-reentry тест не вызывает игровые функции. Новый
локальный диагностический caller работает только без сети/coop-session.ini,
на изолированном профиле и собственном registry-confirmed Human. Он не создаёт
world agreement. Native Sync release/registry removal и штатный выход 0 прошли;
исходный профиль неизменен. Следующий шаг — child/task и AI lifetime при отмене.

## Обязательно перед работой и каждым коммитом

1. Прочитать `AGENTS.md`, этот файл и [полное задание](REQUIREMENTS.md).
2. Получить актуальные удалённые ветки и проверить свою рабочую копию. При
   параллельной работе использовать отдельную ветку/worktree; для новых веток
   по умолчанию применять префикс `codex/`. Не сбрасывать чужие изменения.
3. Перед каждым коммитом сверить эти указания с фактическим результатом.
   Если меняются задача, исполнитель/участок, поведение, проверки, ограничения
   или следующий шаг, обновить соответствующие пункты **в этом же коммите**.
   Если состояние не изменилось, не придумывать обновление ради изменения файла.
4. Подробные проверенные результаты писать в [TESTING.md](../TESTING.md).
   Здесь оставлять краткое текущее состояние и следующий проверяемый результат.
5. Проверять staged diff, не добавлять игры, сохранения, логи, личные пути/IP,
   дампы, ключи, сборки и материалы без права распространения.
6. Сохранять содержательные этапы коммитами. Перед интеграцией снова сделать
   fetch, сохранить авторство/историю обоих участников, не использовать force.
   Порядок публикации и проверки удалённого хеша: [GITHUB.md](GITHUB.md).

## Цель и приёмка

Полная цель — вся основная сюжетная кампания Mafia II Classic вдвоём через
Radmin VPN: игроки, бой, транспорт, NPC, сюжетные события, контрольные точки и
прогресс у хоста. Первый контрольный результат — два разных ПК полностью
проходят одну настоящую сюжетную миссию, включая провал, восстановление и
сохранение результата. Free roam, fixture, загруженная DLL и passive peer
этот результат не заменяют. Играбельный кооператив пока не получен.

## Что уже объединено

- Коммиты `kowmap` до `a73f70c` и вся цепочка PR #1 до `2b74f22` входят в
  проверенную основу `4b17c55`. Не переносить их повторно через cherry-pick.
- Есть SHA256-gated observer, изолированный профиль, чтение координат игрока,
  ENet worker и пассивный приёмник. Нет применения удалённого состояния к миру.
- Исправления DPI/размещения окна и read-only обход фабрики сущностей включены.
  На реальной игре найдены кандидаты Human/Player; создание и безопасное
  удаление удалённого аватара ещё не проверены.
- Телеметрия на паузе немедленно становится inactive и остаётся такой до
  game_unpaused. Анализатор отклоняет снимки на паузе и новую активность после
  shutdown; строгая проверка требует закрытия миссий и меню до shutdown.
- Нативный fixture использует управляемое время и собственную память.
  Подстановка отсутствует в игровой DLL. Сборка, Python/PE, девять проверок
  загрузчика/профиля и network suite прошли; fixture прошёл 100 запусков подряд.
  Новое поведение на настоящей игре ещё требует проверки.
- Проверен отдельный [контроль ввода](INPUT_DIAGNOSTIC.md): в этой сессии
  sky.press_key передаёт Down/Return как сообщения Windows с scan=0, но готовый
  буфер DirectInput не получает соответствующих событий. Это конкретное
  ограничение проверенного пути автоматизации; не повторять те же попытки и
  не считать их входом в сюжет. Теперь разрешён и проверен native scan-code путь, см. актуальный раздел ниже.

## Ближайшая работа

| Направление | Следующий проверяемый результат | Ограничения |
| --- | --- | --- |
| Игровая проверка и ввод | Штатно загрузить тестовое сохранение через меню, получить видимую управляемую сцену, движение и реальные активные сетевые снимки | Пока не подтверждены автономное движение и текущая полная игровая приёмка |
| Удалённый персонаж | Проверить создание обычного Human, модель, размещение и единичное корректное удаление при выходе/смене миссии | Пока только исследование; не вызывать непроверенные функции и не копировать bulk SDK patches |
| Идентификатор сюжетного мира | Проследить и затем сопоставить script variables missionNumber/missionPart с главой, checkpoint и провалом; см. SAVE_RESTORE.md | Save candidate пишется до результата операции; script progress ещё не проверен в игре. World ID не установлен |

Перед одновременным изменением одних файлов указать здесь взятое направление,
ветку и фактический статус. Текущий основной участок — статический разбор
сюжетного восстановления при отложенных тестах; соавтор может независимо
продолжать исследование жизненного цикла удалённого
Human. После устранения ограничения ввода основной агент завершил
read-only аудит сюжетного контекста в `codex/story-context-audit`: изменения
scripts/inspect_runtime.py, scripts/story_audit.py и tests/test_story_audit.py.
Согласовывать пересекающиеся изменения через этот файл и Git-историю.

## Последние указания пользователя для игровых тестов

- Управление и тесты делегированы агенту; не возвращать обычные действия в меню
  пользователю по умолчанию.
- Изначально требовалось окно на втором экране. Затем пользователь явно разрешил
  полный экран для проверки несовпадения игрового и системного курсоров.
- Для текущей проверки сохранение нужно загрузить **стрелками и Enter через
  штатное меню**. Внутренний opt-in Continue не заменяет это испытание.
- Менять видеонастройки только в изолированной копии профиля. Не закрывать
  существующую пользовательскую игру ради своего запуска.
- Подтверждать действия результатом на экране и журналом. Перемещение системного
  курсора не доказывает положение игрового курсора; вызов клавиши не доказывает
  переключение пункта меню.

## Важные границы

- Адреса относятся только к EXE с SHA256
  `16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f`.
- Не копировать цепочку Deactivate/Release/destructor/Free из старого SDK:
  Release может уже удалять объект. Требуется проверенный контракт владения.
- Не отключать сюжетные системы, сохранения или ambient NPC ради простого демо.
- Не объявлять fixture/loopback вторым игровым клиентом или Radmin-прохождением.
- Не выпускать Release до фактического выполнения критериев этапа.

## 2026-11-18 — authorized native menu input

Historical stage notes follow. Current UI workflow and save installation are in
the final section; old open-session/pending statements describe their stage only.

User explicitly authorized a separate native scan-code helper for arrows and Enter in our diagnostic Mafia II. coop_input validates full EXE SHA256, explicit PID, process creation time, live process and foreground owner. It holds one key for 100 ms by default (optional 50..2000 ms), polls foreground every 5 ms and always releases it. SendInput is system input: foreground checking is not atomic with focus changes; stop and reobserve after any focus/error. No parallel senders, arbitrary text, hooks or OS changes.

Actual game verification: The Story opened; Down selected Load Chapter, Up returned to Continue Story; Enter on Continue Story loaded the isolated save to a visible snowy street with Vito. Internal Continue was not enabled. The observer produced active player samples. A passive ENet loopback receiver received 190 active samples in 10 seconds, all axis ranges zero. Autonomous arrow-driven movement is now verified below; a remote avatar, two game clients and Radmin gameplay remain unverified. The game remains open; native exit and final original-profile comparison are pending.

Usage: coop_input inspect PID, then coop_input key PID CREATION up|down|left|right|enter. Use only our launcher's PID and inspect's creation value. Pinned clang warnings-as-errors build passed; wrong creation, invalid key and zero PID refused input.

User requested faster entry. Next runs should omit the completed diagnostic Down/Up detour and rebuilds when code is unchanged. Use lifecycle logs to wait for loading, screenshots at Continue selection and ready scene, and never retry Enter when its outcome is unknown. A fresh end-to-end timing measurement remains pending.
## 2026-11-18 — autonomous movement and concrete Human lifetime

The primary agent remains on codex/story-context-audit. Native Up held for one second moved Vito in the real loaded story scene. A passive loopback receiver recorded 571 active samples over 30 seconds, axis ranges 0.25519 / 4.07849 / 0.00497. This is autonomous game movement carried over the existing network path, not a second game client. sky Escape did not open pause; do not claim pause/unpause acceptance from this session. The game is still open; final original-profile/exit checks remain pending.

coop_input now accepts optional hold_ms 50..2000, default 100. During hold it polls process/focus every 5 ms and releases early on loss; this reduces the exposure window but does not make system input atomic. Invalid durations 0, 49, 2001, negative and overflowing values were rejected. Native helper compiled with warnings-as-errors.

Concrete Human ownership audit now verifies the actual Human slot-zero destructor 0x999160 -> body 0x990770 -> conditional deallocation 0x4019f0. Release's immediate path calls this slot with argument 1. The alternate path has now been traced to a CGame flag and entity sweep (see latest section). See docs/REMOTE_AVATAR.md and scripts/audit_avatar.py. Never append destructor/Free after Release. Next work: validate model clone/setup ownership and sweep ordering before real avatar calls. Read-only runtime inspection after normal Continue found 48 factory entries and valid map resource roots; the save/load candidate value was 0 and did not match the catalogue, so it remains unusable as a world ID.
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

## 2026-11-18 — быстрый интерфейс и обязательный контрольный сейв

Перед игровым тестом прочитать [GAME_UI.md](GAME_UI.md). Пользователь прямо
потребовал повторять Enter со старта до загрузки, а для выхода — **Enter сразу
после Alt+F4**, без промежуточного снимка. Основные команды теперь
scripts/enter-story.ps1 и scripts/exit-game.ps1; последняя вызывает native
coop_input quit, где Alt+F4 и Enter идут в одном процессе. Сохраняются SHA256,
PID/creation/foreground проверки; вход прекращает ввод при load_started, выход
прекращает при завершении процесса/потере фокуса. Не возвращаться к покадровым
ожиданиям меню без причины. Непредвиденный результат сначала наблюдать.

**Соавтору: скачать и установить пользовательский контрольный сейв** по
[TEST_SAVE.md](TEST_SAVE.md), затем использовать выданный TestProfileFrom.
Пользователь явно разрешил публикацию именно этого комплекта:
testdata/story-save.zip + manifest. Это исключение из прежнего запрета на сейвы;
другие профили/логи/бэкапы не публиковать, доступы репозитория не менять.
Установщик создаёт новый изолированный профиль без перезаписи личного прогресса.
Комплект включает необходимые метаданные профиля и видеоконфигурацию.

Проверено в реальной игре: серия Enter дошла до загрузки за 1,404–3,818 с,
первые player_sample появились через 3,100–5,355 с от начала серии (не от запуска
EXE). Сцены с нужным сейвом визуально подтверждены. Нативный немедленный выход
из сюжета занял 2,765 и 3,141 с, native exit 0. В последнем тесте сейв сначала
установлен из подготовленного архива, затем реально загружен; оригинальный
профиль не изменился. Неверный creation и чужой PID отказали без ввода, повтор
enter-story в загруженной сессии отказал без ввода. В native quit исправлена
гонка: исчезновение окна может опередить сигнал process handle, после него до
секунды ждём завершения без новых клавиш. Сборка helper warnings-as-errors прошла.

Новая цель соавтора по UI — пользоваться этими скриптами, не тратить время на
повторную диагностику уже проверенных клавиш. Следующая основная разработка
остаётся прежней: registry-validated Human handles, владение AI/управлением,
реальные remote snapshots, reload lifecycle и затем два игровых клиента.
Удалённый управляемый аватар, две машины/Radmin и прохождение миссии не проверены.

## 2026-11-18 — registry guard built; tests on user hold

The user explicitly asked to defer tests while playing Cyberpunk. Do not launch
Mafia II, send UI/game input or start more suites until the user authorizes tests
again. No game was launched in this stage. Code/docs/Git work can continue.

Added src/game/entity_registry.c/h, a bounded read-only native implementation of
the previously audited GUID lookup. Human validation now resolves the full GUID
from the actual manager, requires the same address, then checks the concrete
vtable before touching the saved pointer. Creation requires registry presence.
Release still clears ownership first; subsequent callbacks poll only registry
state, logging confirmed absence or a ten-second incomplete timeout. Invalid
memory, cycles and generation mismatch are never treated as confirmed absence.
An unexpected allocation contract releases the still-owned model clone but does
not speculate about unknown entity cleanup. This does not implement AI control,
remote snapshots or repeated mission creation.

Pinned full build passed. Production registry fixture passed hash/collision,
absence, generation reuse, missing-memory, cycle, fixed-index, overflow ordering,
misalignment and range cases. Production telemetry fixture passed. Network suite
finished all existing loopback checks. The loader/profile suite was interrupted
on the user's request after load/export/PID/native-exit/log-refusal/missing-DLL
checks; do not call it a complete pass. Registry-guarded Human runtime checks
remain pending; earlier visible-Human results apply to the prior version only.

Next after explicit test resumption: finish required loader checks, run the new
-TestHuman build, establish creation_registry_confirmed and independent removal
confirmation on the game thread. Then continue AI ownership/real remote-state
application and mission reload coverage. Keep the full two-PC Radmin campaign
goal unchanged; no two-game-client mission acceptance is claimed.

## 2026-11-18 — movement command ABI and incompatible upstream class

Tests remain on the user's Cyberpunk hold. This stage only read source/the pinned
EXE file and added static-analysis code/documentation. No game/DLL/fixture was
launched and no UI input was sent. Read docs/HUMAN_CONTROL.md before movement work.

Static evidence found a concrete mismatch in the upstream pedestrian example:
its 0x58 MoveDir allocation is paired with vtable 0x19661bc, whose destructor
0x953b80 touches +0x78; the game's matching allocation path uses 0x98 bytes.
Do not copy that class/size or its manual delete. The real pinned MoveDir class
is vtable 0x18f14b0, size 0x58, deleting destructor 0x42d3e0, independently visible
in local-player initialization and command creation.

Candidate 0x95b0f0 creates/enqueues simple commands (0 Stand / 1 MoveDir) and
returns an additional owned reference through an out smart-pointer argument.
AddCommand 0x94d400 consumes its by-value argument reference; insertion retains
the queue reference, and final argument destruction decrements the reference.
The normal creator path statically balances one queue and one output reference.
Allocation-failure recovery, runtime count behavior and AI ownership remain
unverified. Do not call the new candidate from observer yet.

Human+0x18c is the free-list head, not a current-command selector. Eight pointer/
link slots start at +0x138/+0x13c with stride 8, and five queue heads at +0x178.
Do not set free_head=1 or write an arbitrary command slot as the upstream does.
Future enqueue must validate free-list/queue bounds, cycles, disjoint membership
and available capacity on a registry-validated Human game-thread callback.

Added scripts/audit_human_commands.py. A read-only analysis of the installed
full-hash-matching EXE confirmed its bounded exact-byte evidence; its output
always sets runtime_control_verified/ai_ownership_verified/allocation failure
recovery false. This is static evidence, not an executed movement or test suite.
Next static work: trace AI/script arbitration and interruption, then prepare the
controlled command adapter. In-game verification remains deferred until explicit
test resumption; the two-PC Radmin campaign acceptance is still outstanding.

## 2026-11-18 — bounded native command-queue observation (tests still held)

Added src/game/command_queue.c/h. This read-only production helper validates
all eight slot links, five queue heads and free head, requires a disjoint full
partition of the slots, catches cycles/cross-list aliasing/orphans, and checks
free null references versus readable occupied headers with positive signed
refcounts and engine flag 0x400. It rereads the raw layout to detect changes and
leaves the output zero on any failure. Aligned nonzero vtables are only shape
checks, not verified command classes. Valid/full is distinct from enqueue capacity.

The opt-in Human test now records queue status and counts after activation and
at five seconds, only after a fresh registry/owned-address/class check. No new
command API, allocation, queue mutation or AI override is called. This is
observation for the future controlled adapter, not network avatar control.

Pinned full build completed with warnings-as-errors in build/command-queue.
Fixture tests/test_command_queue.c was authored and compiled only; do not report
it as passed. No test suite, game/DLL/fixture executable or UI input was run in
this stage. The user's Cyberpunk hold remains active. Actual registry-guarded
Human and queue-layout behavior, AI arbitration, movement and cleanup are still
pending real-game acceptance.

After explicit test resumption, rebuild before running the updated test-probe
suite (it now expects test_command_queue.exe), finish required loader/profile
checks, and inspect real -TestHuman queue/registry results. Reject inconsistent
layouts rather than repairing raw lists. Continue static AI/script arbitration
work without starting tests until authorized. Full two-PC Radmin mission/failure/
save acceptance remains unachieved.

## 2026-11-18 — dispatch/lifetime and retained movement reference

Tests remain held for Cyberpunk. This stage used only static EXE reads and
compilation; no game, fixture executable, test suite or UI input was run.

Pinned static evidence now traces Human vtable+0xc8 -> 0x9818a0 -> dispatcher
0x94d9a0. The dispatcher performs descriptor-based begin/update passes, pops
completed heads, releases queue references and recycles slots. A surviving queue-0
head is separately retained at Human+0x1a0. Getter 0x94d540 returns queue head,
not that retained reference. ClearMove 0x94d8d0 drains queue 0 and calls physics
slot +0x14 but does not clear +0x1a0; the general clear has an argument-dependent
retained-reference reset. Do not equate empty queue with destroyed command,
physical stop or AI ownership. Never manually zero/release the borrowed field.
The dispatch skip bit 0x200000 also bypasses neighboring physics work and is not
an audited AI-only disable switch. AI NewCommand(void*) dereferences its second
payload word as a counter address, unlike the raw MoveDir numeric refcount;
these contracts must not be mixed. Exact AI payload ownership is still unknown.

CoopReadCommandQueue now observes the separate retained header and membership,
allows a live retained-but-dequeued command without flag 0x400, and rereads the
retained pointer to detect changes. It does not acquire references or authorize
late pointer use. Opt-in Human logs add retained presence/membership and the
observed dispatch skip flag. No queue mutation or new engine call was introduced.

Full pinned build passed with warnings-as-errors in build/command-lifetime.
New fixture cases for retained queued/dequeued refs, invalid counts and changed
retained pointers were compiled only, not executed. Read-only command audit
confirmed bounded exact-byte sites and still reports runtime/AI control false.
After test resumption, run all required deferred fixtures/loader checks before
real Human observations. Next static work is the producer/ownership/interrupt
contract for AI/script tasks; disconnect stop and unload need separate runtime
proof. No remote-avatar/two-PC Radmin mission acceptance is claimed.

## 2026-11-18 — receiver-side state expiry (compile only)

Tests remain on the explicit Cyberpunk hold. No game, UI input, sockets/test
executables or test suites were started in this stage. Full pinned compilation
finished successfully in build/receiver-expiry; the initial fixture include-order
conflict was fixed by removing the unnecessary Windows dependency from the
mailbox header and including Windows explicitly in its implementation.

Found and fixed a data-validity gap: outgoing producer state expired at two
seconds, but accepted incoming active state could survive until the ten-second
connection timeout. CoopSession now timestamps only accepted new STATE packets;
duplicates/stale sequence/old epoch do not refresh it. At two seconds it clears
remote activity/positions and logs remote_expired without forcing disconnect.
New accepted state may restore activity with the existing sequence checks.

The remote mailbox now includes accepted sequence, session ID, receiver-local
connection generation and monotonic timestamps. CoopNetworkReadRemote applies
a pure validity filter on every read: a stale worker/stop/no worker clears the
whole output and returns false; stale/missing/inactive/invalid state within a
live handshake clears active and coordinates. This covers frozen workers too.
Never retain raw mailbox output across later frames without validity checks.
Wire encoding/protocol did not change. These timestamps/counters are not story
identity, source capture time or a shared world generation. Physical stop in a
remote Human is not implemented by this data-only expiry.

Prepared/compiled, NOT executed: pure receiver clock/expiry/inactive/recovery/
rollover cases; two-process mode where a peer services ENet but ceases STATE;
and output-zero assertion after network Stop. The updated test-network suite
expects the rebuilt test_remote_snapshot.exe. Older green transport results do
not verify this version. After user authorizes tests, run the complete required
suites and real game acceptance. AI ownership/controlled movement, authoritative
world identity, mission transitions and two-PC Radmin mission/failure/save proof
remain outstanding.

## 2026-11-18 — story identity static exclusions

Current work remains on codex/story-context-audit; tests are held for Cyberpunk.
Added read-only scripts/audit_story_identity.py, executed only on the pinned EXE
file. Exact branches prove PLAYING_CHAPTER is mode constant zero, not a chapter
number (the same enum includes menu/pause/playing modes). Do not use this enum
as world identity. Save/load candidate interface+0x68 equals main+0x6c; the
neighboring operation-clear writes main+0x68 and proves no candidate reset.
See STORY_IDENTITY.md for exact sites and limitations. Setter-shaped 0xb2c220
still lacks a verified caller/argument contract. No new game call was added.

Next identity work must trace the actual loaded mission/checkpoint producer,
then correlate it with normal Continue, checkpoint, failure/reload, chapter
change and menu only after the user resumes tests. Matching static bytes or a
save catalogue entry is insufficient. Current candidate meaning/lifetime and
world identity remain unverified; the audit explicitly reports them false.
No game/test executable/suite or UI input was run in this stage.

## 2026-11-18 — asynchronous load request observation (not loaded identity)

Static follow-up traced the actual Continue load slot: derived interface vtable
0x1974668 +0x30 -> 0xb2d020, creating operation vtable 0x197343c with requested
save ID at +8. The nearby main+0x68 field is load mode on this path, written by
0xb2c740; it is not current chapter. Exact-byte audit now covers this chain.
story_audit adds bounded/repeated external reads of this known operation;
inspect_runtime exposes its ID/mode only privately and status publicly.
loaded_world_verified remains false in every case. A stable request or absent
operation must never be promoted to confirmed loaded mission/checkpoint.

Synthetic reader cases were authored but not run. Only static EXE auditing and
source compilation were performed, with no game/live-memory inspection or
test/suite/input. The Cyberpunk hold remains active. After tests resume, include
the expanded test_story_audit.py in deferred checks before live correlation.
Next static identity work: trace the load operation's completion/restore
consumer and actual mission/checkpoint producer; do not use request data for
remote-avatar authorization or world handshake yet.

## 2026-11-18 — save candidate is written before SAVE result

Static work found an actual candidate writer independent of setter 0xb2c220:
0xb407f0, operation vtable 0x1973444, writes its nonzero input+8 to 0x1cc39c4
at 0xb40835 before checking the subsequent SAVE data result. A matching candidate
therefore cannot confirm a loaded mission/checkpoint. Added this rule to AGENTS
and a public diagnostic qualifier candidate_may_precede_completion=true; IDs
remain private. Exact-byte audit now checks the writer/result branch and a
continuation chain through 0xb3f770 -> vtable 0x19740d8 -> 0xb2ce40 that can
reinstall operation 0x1973444 with its saved input and reset local state.

Only pinned EXE file reads and source compilation were performed; no game,
live-memory inspection, test fixture/suite, input or networking. The test hold
remains active. Next: trace the SAVE data consumer through actual restore and
loaded mission/checkpoint publication. Do not infer success from candidate,
operation AL, local state counters or operation absence. Existing deferred
reader/network/Human checks still need execution after authorization.

## 2026-11-18 — CGame's Game Restore notification is an empty handler

Static audit now covers event 19 registration at 0x5c9e8b, priority 500,
CGame singleton and callback 0x5977a0. That thunk forwards vtable+0x30;
installed CGame table 0x190af18 maps it to empty ret-4 handler 0x58a740.
Do not invoke it to recover a mission ID or infer a payload layout from its
name. This result is specific to CGame: other module handlers and the event
producer/context remain unaudited. Observer behavior and game calls unchanged.

Read-only exact-byte file audit matched the pinned EXE; source compilation
only otherwise. No game/test/suite/live-memory/input/network execution; hold
remains active. Continue at the actual SAVE data consumer behind interface
0x1ac0c48 and restoration callbacks. A world ID is still not established,
and the two-PC Radmin mission/failure/save acceptance remains outstanding.

## 2026-11-18 — SAVE read-layer dispatcher identified

Static work resolved SAVE service global 0x1ac0c48 -> constructor 0x11cafd0 ->
vtable 0x190c70c. Read slot+0x10 -> 0x11c59a0 validates version 24/25 and ID,
then looks up stream layers and invokes 0x11c4600. The latter invokes each
registered callback object's vtable+4. The new SAVE_RESTORE.md maps layer IDs
0..10, POSTLOAD=8 and LAST=11 end marker, and records exact next research sites.
Do not interpret these indices, a header match or callback AL as current world
identity. Concrete story callbacks, restored fields and their lifetime are
still unverified. Static audit reports save_layer_callbacks_verified=false.

Exact-byte file audit matched; source compilation only otherwise. No game,
live-memory read, engine calls, test/suite/input/network execution. Hold remains
active. Next work: identify registered story-data callbacks in those layers
and trace their read methods to mission/checkpoint storage plus overall load
completion. Existing deferred checks still need execution after user resumes
tests; no two-PC Radmin mission/failure/progress acceptance is claimed.

## 2026-11-18 — script progress candidate from Application restore

Traced concrete Application registration: subobject+0x94 vtable 0x18f4f54,
read slot+4 -> 0x507f60 -> 0x507b30, including INIT and POSTLOAD layers.
INIT restores missionNumber/missionPart through global 0x1abfe24 and setter
0x5981d0; writer 0x473910 reads the same names via 0x598040. See SAVE_RESTORE.md
for exact lookup layout and restrictions. These are stronger story candidates
than the save request ID, but current chapter/checkpoint mapping and freshness
are not proven. Never turn them into world authorization on name evidence alone.

Added bounded external script_progress_audit reader and private report values;
public console includes only status and unverified flags. It caps count at 100,
name reads within a conservative 4096-byte pool budget and finite integral values
0..65535, matches native newest-name priority, and rereads observable state.
No game calls/writes, observer changes or network identity encoding were added.

Pinned static audit matched; source compilation only otherwise. Synthetic cases
for good/missing/invalid/duplicate/changed data were authored but NOT executed.
No live game/memory/input/network or suite/fixture execution; hold still active.
Next: audit additional story callbacks and lifecycle/reset behavior; after user
resumes tests, run deferred reader checks and correlate this pair through real
Continue/checkpoint/failure/reload/chapter/menu. Two-PC Radmin acceptance remains
outstanding.

## 2026-11-18 — native script progress logs (compile only)

Added src/game/script_progress.c/.h, a pure bounded reader with cleared failure
output, 100-slot cap, finite type-2 integer pair, native reverse lookup priority
and singleton/count/record/name rereads. Telemetry invokes it only under the
observer's pinned-image/game-thread context, at lifecycle/save/load boundaries
and at most 2 Hz on active/paused/menu ticks. Private script_progress logs carry
event/local epoch/init/load/pause/status plus the pair only when observed;
world_verified=0 and checkpoint_verified=0 always. Do not use these logs or
values as world handshake authority; menu/load values may be stale.

Full pinned warnings-as-errors build succeeded in build/script-progress. New
C fixture compiled only, including full 100-slot and changed-read cases;
test-probe wiring added for future runs. Telemetry fixture uses a zero script
slot, never a real game address. Rechecked pinned static audit. No game,
fixture/suite/DLL execution, live memory, network or input; Cyberpunk hold remains.
No wire/position publication behavior changed. After test authorization, run
deferred C/Python/loader/network checks before ordinary Continue/checkpoint/
failure/reload/chapter/menu correlation. The pair's lifecycle and checkpoint
coverage, remote Human control and two-PC Radmin mission acceptance are pending.

## 2026-11-19 — chapter script checkpoint separation

Added bounded static Lua 5.1 chunk reader and SCRIPT_CHECKPOINTS.md. In inspected
M03 bytecode, missionPart selects six parts, while the saves table describes
11 story events and progress branches call SaveGame(1). Treat part, story event
and save slot separately; uniqueness of missionNumber/missionPart as checkpoint
is unproven. Static references do not establish runtime reachability or a
campaign-wide mapping. Original archives/chunks/extractor remain ignored.

Read six original chunks as data; final key reports retain executed=false and
reachability_verified=false. Python source compilation completed. No game, Lua
VM, fixture/suite, live memory, input or network execution; Cyberpunk test hold
continues. Next static work: native SaveGame binding and save-event story ID
producer. After explicit test resumption, correlate real checkpoint/failure/
reload with script pair and host transition generation. Two-PC mission
acceptance and remote Human control remain outstanding.

## 2026-11-19 — Lua save request ticket is separate from save argument

Static pinned EXE trace maps SaveGame -> 0xad6620 -> interface slots +0x34 /
+0x38 -> 0xb6e130 / 0xb4efc0. The request creates a counter ticket independently
of the numeric Lua argument and inserts a 12-byte ticket/argument/reference
record. The second method resolves an object by ticket. The Lua bridge's final
1 is its result count, not disk-save success. Queue insertion can precede a
zero-return branch. SAVE_RESTORE.md documents addresses and limits; the public
static audit now pins the relevant bytes and keeps durable completion false.

Read-only EXE audit matched. No tests, game, live process reads, input or sockets
were run; Cyberpunk hold remains. Next: queued-save consumer at main vtable+0x18
and returned object's completion/failure protocol, then actual file commit.
Do not use tickets as shared world/checkpoint IDs or saved-progress proof.

## 2026-11-19 — queued save notification and next-request dispatch

Mapped installed main update 0xb6a6d0 and state-9 branch 0xb6a8bd. After
operation processing, that branch notifies the first record's managed object,
calls a listener, erases the record and forwards the next record's numeric
argument to main slot+0x18 -> 0xb2d070 -> operation 0x1973444. Notification
0x11e7680 clears object+8 bit 0 and traverses waiters without an explicit
success/error argument. Do not interpret this bit, queue removal or the
operation's UI timing return as durable saved host progress.

Pinned read-only exact-byte audit matched; no game/test/fixture/live memory/
input/network execution. Cyberpunk hold continues. SAVE_RESTORE.md records
main-versus-interface offsets and remaining limits. Next: backend global
0x1ac0c50 slot+0x10 reached at 0xb40932, commit/error propagation, plus listener
and waiter semantics. No runtime or two-PC acceptance is claimed.

## 2026-11-19 — backend write result boundary

Resolved global 0x1ac0c50 factory/table -> writer 0x11ca700. It writes a .dat
stream, checks three returned byte counts, returns -2 on setup/open/preflight
failure, -1 on count mismatch and 0 on the normal path after close/release and
catalogue update. The save operation branches on nonzero at 0xb40948. This
is a useful local result boundary; concrete close/flush and full error delivery
remain unaudited, so durable host progress and shared checkpoint remain false.

Static exact-byte audit matched the pinned EXE. An initial evidence-span start
was corrected to the instruction boundary before the successful final audit.
Source compilation only otherwise; no game, tests/fixtures, live memory, input
or network execution. Cyberpunk hold remains active. Next: concrete filesystem
behind 0x695b70 and close slot+0xb4, fallback operation and final listener result.
Keep raw assets local. Runtime/two-PC mission/failure/save acceptance is pending.

## 2026-11-21 — user-requested coop-anything review

Read upstream docs and selected ownership/flight C++ sources through the web.
See COOP_ANYTHING_REVIEW.md for source links and evidence limits. Useful leads:
host-owned story effects, actor authority epochs, cross-machine traces, UDP
impairment and later hot reload. No tools/dependencies installed or imported;
no upstream tests, MCP, game, live memory, input or sockets were run. The clone
was unavailable (approval timeout, sandbox DNS failure), so no upstream SHA is
pinned; obtain/review exact files before reuse.

Important new related lead: JustAddMP/MafiaDE and its shared core. It targets
Mafia: Definitive Edition x64, not our Mafia II Classic x86. Its authors report
limited two-copy/one-PC checks, not two-PC acceptance. Study story/failure/save
policies; don't copy bindings or assume the umbrella MIT license covers it.
For our first story slice keep NPC authority on the host; map slot 0 upstream
to our 1/2 IDs only through an explicit adapter if later adopting ownership.
Prioritize remote-player control and shared-world transitions, then mission
primitive capture/apply and correlated traces. Keep all testing deferred under
the user's continuing hold; none of this research establishes campaign co-op.

## 2026-11-21 — implemented shared-world network agreement (compile only)

Added production world_sync.c/.h and integrated OFFER/ACK/COMMIT into session,
wire and worker mailboxes. Protocol is now 2, module 0.0.4; old builds cannot
connect. STATE carries host world generation. Nonzero-generation states require
matching agreement and acknowledged sender epoch. Reliable controls validate
role/session/channel, old generations cannot restore authority, local-load races
ignore obsolete COMMIT, generation exhaustion closes the gate. World mailbox
revision preserves withdrawal across worker polls; current local activity and
local/remote/worker freshness are rechecked by CoopNetworkReadRemoteWorld.

IMPORTANT: CoopNetworkReadRemote remains telemetry-only. Future actor consumers
must use CoopNetworkReadRemoteWorld. No verified Mafia loaded-world producer
exists yet and no production code publishes a nonzero world key. Do not hash
missionNumber/missionPart or save request IDs to bypass this missing producer.
The gate therefore remains closed in the actual observer. No avatar movement,
AI suppression, guest loading, story effects or host-save completion is claimed.

Full pinned x86 warnings-as-errors build completed in build/world-agreement.
Authored/compiled pure agreement, wire, snapshot and two-process synthetic world
mismatch/recovery fixtures; NONE executed. Build script compiles only. No game,
fixture/suite, live memory, network session or input ran; user hold continues.
Next: verified game-world identity/load producer, guest transition and actual
Human movement/stop/AI control behind this gate. Read WORLD_AGREEMENT.md and
HUMAN_CONTROL.md before integrating; two-PC mission/failure/save remains pending.

## 2026-11-21 — remote actor controller, engine backend still pending

Added remote_actor.c/.h, compiled into the observer without attaching it to game
callbacks. It consumes only fresh agreed-world snapshots, owns one backend
handle, retires it on session/connection/world/epoch/player change or stale data,
stops before removal, begins removal once and waits for registry-confirmed
absence before replacement. Bounded XY velocity requests, arrival hysteresis,
far/vertical stop and creation retry interval are implemented. There is NO
native backend, movement call, AI suppression or running network avatar yet.

The backend contract requires pinned image/game thread/registry guards and
proven AI exclusion; handle/reference ownership remains in that backend.
Read HUMAN_CONTROL.md's new section before wiring it. No direct queue/free-list
writes, raw command delete, unsafe CreateSimpleCommand call or AI payload reuse.
Static analysis additionally traced descriptor initialization 0xef5ad0 and
MoveDir start/tick field transfers. Extended exact-byte audit matched pinned EXE.

Pinned full x86 warnings-as-errors build completed in build/remote-actor.
Authored and compiled callback-only lifecycle/motion fixture; NOT executed.
No game/test/suite/live memory/input/network session ran. User test hold remains.
Next: native backend's safe allocation/reference handling, AI ownership and
confirmed stop/removal, plus verified loaded-world producer. Two-PC story
mission/failure/host-save acceptance remains outstanding.

## 2026-11-21 — native MoveDir owner candidate (not attached/executed)

Added move_command.c/.h with pinned x86 allocator/ref/init/enqueue/input/cancel/
release bindings. Guard is required for every action; EDIT needs verified world
and exclusive AI movement ownership, cleanup/release must remain possible after
network invalidation. No observer callback calls this module. It is not yet the
remote actor backend and no network avatar moves in the actual observer.

Candidate creation checks free capacity/empty movement queue/no retained ref,
null-checks allocation separately, owns one ref and passes an extra consumed
argument ref to AddCommand. It preserves queue/free-list ownership, high control
bits and caller-owned refs on INCOMPLETE. Cancel is a request, not proof of stop;
release drops only the caller ref. Never use cancellation as the actor backend's
confirmed stop without retained/physics/AI checks. Zero initialization of
reserved storage and duplicate pre-enqueue initialization need live acceptance.

Full pinned warnings-as-errors build passed in build/native-move. Extended
read-only EXE audit matched allocator dispatch/ref constructor/clear signatures.
Generated PE inspection confirmed ECX=Human and two stack words for AddCommand;
no native/game/test/fixture/live memory/input/network execution. User hold stays.
Next: validate allocation/ref/AI/stop behavior when tests resume, provide the
actual Human backend and verified loaded-world producer. Full story/two-PC
Radmin mission/failure/host-save acceptance remains outstanding.

## 2026-11-21 — AI controller lifetime traced; no disable API accepted

Pinned full-SHA static audit now checks Human+0xa8 population by base constructor
0x998cb0 through controller ctor 0x995f20, vtable 0x1967228, Human owner+8,
preparation 0x99f0a0, embedded+0x44 update 0x999b80 and unconditional virtual
teardown dereference at 0x9858b7. Earlier zero initialization is temporary:
do not null/replace/free +0xa8 to exclude AI. Vtable+0x14 -> 0x99d860 invokes
several subsystems; it is not an accepted pause/disable function.

See HUMAN_CONTROL.md. Next trace: registration/update eligibility and reversible
ownership while preserving physics/dispatch/teardown. MoveDir EDIT and agreed
world gates remain closed; no native backend is attached. Read-only byte audit
succeeded; no game/tests/fixtures/live memory/input/network ran. User hold still
applies. No two-PC mission/failure/save acceptance or AI suppression claimed.

## 2026-11-21 — MoveDir owner recursive-entry protection

Native candidate now holds a per-owner busy barrier across guards and native
calls for Create/Input/Cancel/Release. Same-owner recursive calls return REFUSED
without callbacks/state mutation; creation's pre-ref allocator interval and
release's post-transfer destructor interval cannot start another operation.
All normal returns unlock. Use on the verified game thread only; do not treat
this as thread safety or engine exception recovery, or reset a live owner.

Added production-linked tests/test_move_reentry.c with refusing recursive guards;
compiled in full pinned warnings-as-errors build/move-reentry, NOT executed.
No game/tests/live memory/input/network ran; user hold remains. AI message names
were found statically but do not yet prove a reversible AI ownership API.
Next: controller eligibility/ownership and confirmed stop, actual Human backend
and verified loaded-world producer. Neither runtime avatar nor two-PC story/
failure/restart/host-save acceptance is established.

## 2026-11-21 — AI lifecycle pairing and observation module

New human_ai.c/.h reader checks pinned Human/controller type, full GUID, both
Human back-pointers, bounds and repeated relevant fields. It does not retain
refs, mutate AI or establish ownership. Only -TestHuman logs it after Setup,
after Activate and before release. No new event has in-game acceptance yet.
agent presence and raw +0x170 mode flags are NOT AI suppression/stop evidence.

Extended full-SHA audit matched activation -> controller slot+0x10 (0x9960b0)
and deactivation -> slot+0x14 (0x99d860); whole-Human deactivation also disables
physics. SDK ScriptSetControlStyleStr resolves to stub 0x91aac0 for Human.
Neither is an AI-only ownership policy. Read HUMAN_CONTROL.md/HUMAN_TEST.md.
Full pinned warnings-as-errors compilation passed in build/human-ai; byte audit
passed, no game/tests/fixtures/live memory/input/network/native calls executed.
Hold continues. Next: actual per-actor AI ownership mechanism preserving physics
and lifecycle, then native movement/confirmed stop and loaded-world producer.
No two-PC mission/failure/restart/save or full campaign acceptance is claimed.

## 2026-11-21 — new scripted movement route, not enabled

Read SCRIPT_MOVE.md. Mafia2-Online's inspected network path creates Human and
moves via ScrMoveV; Player creation/keyboard filtering is separate. Pinned EXE
0x9910b0 constructs a proper AI command and submits its two-word handle to
NewCommand, returning an owned intrusive Sync ref. This offers a route through
the AI scheduler rather than direct MoveDir competing underneath it.

Added script_move.c/.h guarded/reentry-protected Start and caller-ref Release,
compiled into observer with NO callbacks/callers. HANDLE_CREATED proves only
Sync header, not accepted command/motion. INCOMPLETE can preserve a ref; never
reset it. Release drops our ref only, NOT cancellation or physical stop. Guard
must verify image/base/thread, and START needs current mod-owned registry Human
plus fresh agreed world. This does not open direct MoveDir AI-ownership gates.

Full pinned warnings-as-errors build/script-move and exact-byte audit passed;
emitted PE checked for thiscall/five arguments/ret-20 cleanup. No game/tests/
fixtures/native calls/live memory/input/network ran; user hold remains. Next:
trace native script-command cancellation/supersession and verify stop/ref balance
when tests resume, then choose/wire actual backend and loaded-world producer.
No two-PC mission/failure/restart/save or full campaign acceptance is claimed.

## 2026-11-21 — scripted command state reader; cancellation still pending

Added script_move_state.c/.h pure reader and CoopScriptMoveObserve. It follows
our owned Sync through typed userdata to AI move command, checking pointer/
class/count/link/state and repeated snapshots. Refusal clears output. Observe
needs pinned image/thread/owned Sync, not a live Human/world. No caller attached.
Raw state/Sync flags/link absence are NOT success, cancellation or stop proof.

Important new pinned fact: this AI command's separate refcounter is one FOUR-
byte allocation (0x9570a0), not the two-counter handle family. Sync flags are
byte+8; adjacent uninitialized bytes must not be interpreted as flags. Native
state setter 0xf5d210 notifies Sync then releases/clears command+4 Sync wrapper.
0xf95100 removes matching tasks/queued commands, but its concrete owning object
and physical cleanup remain unverified. Do not guess this or use raw +0x0d
writes; that byte is serialized. Next: concrete removal owner/caller/task cleanup.

Full pinned warnings-as-errors build/script-state and exact-byte audit passed.
Synthetic state-reader fixture compiled, NOT executed; no game/native calls/
tests/live memory/input/network ran. Hold remains. Actual backend, verified
loaded-world producer and two-PC story/failure/restart/save remain outstanding.

## 2026-11-21 — guarded native end request, no stop acceptance

Added CoopScriptMoveRequestEnd: END guard authorizes pinned image/thread/owned
Sync-command cleanup even after world withdrawal. Exact native signature and
typed ownership chain are checked. Only linked raw-state-0 commands request
state 3 via 0xf5d290; 1..3 are preserved as TERMINAL_OBSERVED. Owned Sync stays
retained. Busy covers guards/native callbacks. No callback calls this module.

END_REQUESTED/TERMINAL_OBSERVED do NOT mean stopped or removed. Native state-0
branch writes 3 without Sync Done/ref cleanup; engine processing/physical stop
must be verified separately. Never map either result directly to backend stop.
Alternate target-removal 0xf95100 remains unbound: its inspected slot+0x4c is a
secondary interface (dtor this adjustment -0x40), not Human/controller/brain.

See SCRIPT_MOVE.md. Full pinned warnings-as-errors build/script-end, full-SHA
audit and emitted-PE thiscall/argument review passed. No game/tests/fixtures/
native calls/live memory/input/network executed; user hold stays. Next: concrete
movement task cleanup/physical-stop condition and end/ref acceptance after hold,
then actual remote backend plus verified loaded-world producer. Two-PC mission/
failure/restart/host-save and full campaign acceptance remain outstanding.

## 2026-11-23 — current runtime acceptance and next work

User testing hold is lifted by the current request and explicit second-display authorization. The pinned full build, six Python scripts, eight C fixtures, nine loader/profile checks and complete network suite have run. A fixture-summary failure after expected peer disconnect was fixed without changing production networking: keep observed receive evidence independently of the currently zeroed mailbox. The stopped-output-zero check remains mandatory.

The current registry-guarded -TestHuman build ran in a normal-menu loaded, visually confirmed story scene on DISPLAY2. Creation identity and one Release/registry-absence cycle passed; AI snapshots were valid at all three phases and the command queue/retained reference reader produced valid observations. This does not authorize AI exclusion, native movement/end calls or remote motion. Screenshots did not independently identify the generated Human in the crowd. The original profile stayed unchanged and native child exit was 0. One epoch is not complete reload/lifecycle acceptance.

Keep movement and world gates closed. Next verify repeated actor lifetime/mission transitions and the scripted-command cancellation/physical-stop/reference contract, then a concrete backend and loaded-world producer. Normal Continue produced script progress observations, not a unique checkpoint identity. The first two 23 November passive windows were near-stationary; a subsequent bounded six-second peer run overlapped native Up input and received 104 active samples, ranges 1.33240 / 4.84363 / 0.00959. This proves local-player telemetry transfer, not a remote avatar. A second isolated game process repeated the guarded Human lifecycle with confirmed registry removal, normal exit and zero original-profile changes; same-process reload remains untested. Current AGENTS/HUMAN_TEST/SCRIPT_MOVE status now explicitly supersedes the historical hold. Full two-PC story/failure/save acceptance is still missing. Approximate full-campaign readiness: 10% (subjective).
