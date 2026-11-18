# Указания соавторам и текущее состояние

Проверено: 18 ноября 2026 года. Последняя проверенная кодовая основа:
`aeaa613fbcaf309465fbfc01a0ab5f801f44074b` в `development`.
Этот хеш обозначает проверенную основу, а не будущий HEAD документа.

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
| Идентификатор сюжетного мира | Сопоставить новое save/load candidate-поле с загрузкой, checkpoint и сменой главы; см. STORY_IDENTITY.md | GameBase +0x0c — буфер запроса; четыре строки CGame — корни ресурсов. Ни одно пока не является world ID |

Перед одновременным изменением одних файлов указать здесь взятое направление,
ветку и фактический статус. Текущий основной участок — игровая проверка и ввод;
соавтор может независимо продолжать исследование жизненного цикла удалённого
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
