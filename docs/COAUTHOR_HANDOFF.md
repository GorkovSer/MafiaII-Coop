# Указания соавторам и текущее состояние

Проверено: 18 ноября 2026 года. Последняя проверенная кодовая основа:
`4b17c554cdfcffa141d5f1d6beb662f0ac22e1ce` в `development`.
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