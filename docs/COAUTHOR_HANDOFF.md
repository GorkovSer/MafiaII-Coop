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

User explicitly authorized a separate native scan-code helper for arrows and Enter in our diagnostic Mafia II. coop_input validates full EXE SHA256, explicit PID, process creation time, live process and foreground owner. It holds one key for 100 ms and always releases it. SendInput is system input: foreground checking is not atomic with focus changes; stop and reobserve after any focus/error. No parallel senders, arbitrary text, hooks or OS changes.

Actual game verification: The Story opened; Down selected Load Chapter, Up returned to Continue Story; Enter on Continue Story loaded the isolated save to a visible snowy street with Vito. Internal Continue was not enabled. The observer produced active player samples. A passive ENet loopback receiver received 190 active samples in 10 seconds, all axis ranges zero. Autonomous movement, a remote avatar, two game clients and Radmin gameplay remain unverified. The game remains open; native exit and final original-profile comparison are pending.

Usage: coop_input inspect PID, then coop_input key PID CREATION up|down|left|right|enter. Use only our launcher's PID and inspect's creation value. Pinned clang warnings-as-errors build passed; wrong creation, invalid key and zero PID refused input.

User requested faster entry. Next runs should omit the completed diagnostic Down/Up detour and rebuilds when code is unchanged. Use lifecycle logs to wait for loading, screenshots at Continue selection and ready scene, and never retry Enter when its outcome is unknown. A fresh end-to-end timing measurement remains pending.