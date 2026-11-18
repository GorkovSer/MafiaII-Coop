# Статический путь восстановления SAVE

Это карта исследования pinned GOG EXE, а не подтверждённый сетевой world ID.
Полный SHA256 и точные инструкции проверяет `scripts/audit_story_identity.py`.
Не вызывать перечисленные функции из DLL до проверки их полных контрактов.

## Объект и методы

Глобальный указатель 0x1ac0c48 получает объект из конструктора 0x11cafd0.
Путь выделения/присваивания виден в 0x11cb730..0x11cb74a; второй путь использует
тот же конструктор в 0x11cccd4. Конструктор ставит vtable 0x190c70c.
Её slot +0x0c ведёт в 0x11c55f0, slot +0x10 — в **0x11c59a0**.
Ранее исследованные операции вызывают оба slot; нельзя переносить семантику
результата одного метода на другой.

В 0x11c59a0 читается заголовок через 0x11bfd70; ветка 0x11c5a2f..0x11c5a41
требует version 24 или 25 и совпадение ID заголовка с аргументом. Затем идут
другие проверки, создание контекста чтения и обход секций. Само совпадение
version/ID не является завершением этого пути или готовностью миссии.

## Слои

0x11ca2e0 создаёт 11 объектов слоёв с индексами 0..10. Их строковые имена
возвращает 0x11bc8f0. В конструкторе слоя 0x11ca100 имя преобразуется в hash
циклом multiply-then-XOR с basis 0x811c9dc5 и multiplier 0x1000193.
Индексы обозначают стадии обработки, а не сюжетные главы.

| ID | Имя в EXE |
| --- | --- |
| 0 | LAYER_PREINIT |
| 1 | LAYER_INIT |
| 2 | LAYER_CLEANUP |
| 3 | LAYER_DATA_DEPENDENT |
| 4 | LAYER_PREREQUISITIES |
| 5 | LAYER_DATA |
| 6 | LAYER_DEPENDENCIES |
| 7 | LAYER_DONE |
| 8 | LAYER_POSTLOAD |
| 9 | LAYER_LOAD_FAILED |
| 10 | LAYER_CHECK_SAVE_ENABLED |
| 11 | LAYER_LAST — маркер конца, не один из 11 создаваемых слоёв |

Read-метод ищет слой по hash секции через manager+8 и 0x11c3bb0, получает
объект из tree node+0x10 и его индекс из layer+0x1c. Индексы >=9 обходятся
иначе, для индекса 8 есть отдельная ветка. Затем 0x11c5bc4 вызывает
**0x11c4600**, передавая слой как this и контекст/аргументы чтения.
Это не доказательство выполнения всех слоёв: поток и ошибки определяют путь.

В 0x11c4600 ненулевой поток читается до hash маркера `LAST` (0x18f3ee8), lookup
0x11c3b10 выбирает зарегистрированный объект из дерева layer+0x10, затем
0x11c468b..0x11c4698 вызывает его **vtable+4** с контекстом на стеке.
Нулевой поток идёт по отдельному обходу вектора layer+4..+8 и тоже вызывает
vtable+4. Ни одну из этих функций новый код мода сейчас не вызывает.

## Следующая работа

Один обработчик Application уже прослежен ниже. Для остальных нужно найти
регистрацию обработчиков сюжетных данных в этих слоях, подтвердить
их vtable+4 и восстановленные поля mission/checkpoint, а затем срок жизни
полей и сигнал завершения всего перехода. Само имя POSTLOAD, вход в callback
или его AL не гарантирует валидность мира. Обработчик может завершиться ошибкой,
а соседние слои ещё не выполниться. Не добавлять сетевую авторизацию по имени
слоя, локальному индексу или ID сейва из запроса.

Динамические проверки отложены по просьбе пользователя. Карта получена только
чтением EXE; полная совокупность callbacks и сюжетные поля ещё не проверены
в игре, что отражено в `save_layer_callbacks_verified=false` и
`world_identity_verified=false` статического отчёта.

## Application: missionNumber и missionPart

Конструктор 0x4e2390 устанавливает в subobject +0x94 vtable 0x18f4f54.
Регистрация 0x473810 передаёт этот subobject в 0x11ca3a0 для слоёв
10/0/1/8/4/9. Read slot +4 ведёт в thunk 0x507f60, который берёт Application
из 0x1ab2838 и переходит в **0x507b30**. Это конкретный найденный read callback,
а не предположение по расположению соседних функций.

Путь INIT этого callback читает из потока значения и в 0x507c8a/0x507ca2
передаёт их setter 0x5981d0 для имён **missionNumber** и **missionPart** через
global 0x1abfe24. Отдельный writer 0x473910 читает оба имени через 0x598040.
POSTLOAD-ветка 0x507d5a проверяет свой контекст и позднее посылает event 33;
это ещё не проверка всех сюжетных callbacks и не общий сетевой transition ID.

В 0x598040 тип записи должен быть 2; значение читается как float.
Lookup 0x58b4b0 идёт с конца массива, stride=12, name-offset в record+0,
float в +4, type uint16 в +8 относительно manager+4. Count находится в
manager+0x4b4, pool имён начинается в +0x4b8. Добавление 0x597ff0 ограничивает
число записей сотней. Setter 0x5981d0 создаёт имя при отсутствии и ставит type=2.

`scripts/script_progress_audit.py`, вызываемый из внешнего inspect_runtime,
читает только этот bounded layout. Он повторно сверяет pointer/count/records
и прочитанные имена; это обнаруживает видимые изменения, но не даёт атомарность
или защиту от ABA. Диагностические ограничения: name-offset вместе с нужным
именем должен помещаться в первые 4096 байт pool, значения — finite целые
0..65535. Это консервативные ограничения reader, не доказанные диапазоны
сюжетных глав. Новейший дубликат имеет приоритет, как в native lookup; неверный
тип не разрешает откат к старой записи. Missing/invalid не превращаются в ноль.

Значения остаются в частном отчёте. Консоль показывает только status и false
для identity/checkpoint mapping. Синтетические случаи подготовлены, не выполнены
из-за hold; reader не проверялся на живой игре. Переменные могут быть stale в
меню/при загрузке, а missionPart может не различать каждый checkpoint. До
сопоставления загрузки, движения сюжета, провала/повтора, новой главы и меню
не использовать эту пару для разрешения применения сетевого состояния.

## Нативное наблюдение в DLL (пока только сборка)

`src/game/script_progress.c` реализует то же ограниченное чтение без вызовов
движка. Наблюдатель берёт global slot 0x1abfe24 только после своего полного
EXE SHA256 gate и на игровом callback thread. Читается максимум 100 записей и
200 ограниченных фрагментов имён с повторным сравнением. Отдельный callback
чтения использует существующую проверку доступности памяти. Никаких ссылок
движка reader не захватывает; сравнение не атомарно и не исключает ABA/stale.
При любой ошибке выходная структура очищается; значения доступны только со
статусом `COOP_PROGRESS_OBSERVED`.

Телеметрия пишет частные строки `script_progress`: время, исходный event ID,
локальную epoch, initialized/loaded/paused, статус и, только если прочитаны оба
поля, mission_number/mission_part. Точки наблюдения — события 10/11/12/13/14,
19/20/21/27/32/33/39 и максимум два раза в секунду на tick/paused/no-game tick.
Наблюдение в меню и на границах намеренно допускает stale script values: оно
нужно для проверки их срока жизни. Локальная epoch — метка сопоставления журнала,
а не общий world ID. Каждая строка содержит world_verified=0/checkpoint_verified=0.

| status | Значение |
| --- | --- |
| 0 | unreadable |
| 1 | missing |
| 2 | bad layout/bounds |
| 3 | wrong type/nonfinite/nonintegral/out-of-budget value |
| 4 | changed during reread |
| 5 | pair observed; current world/checkpoint не подтверждены |

Кэш для сети не добавлен, wire/ENet и публикация позиции не менялись. Наблюдатель
не разрешает применение удалённого состояния по этой паре. Отдельный C fixture
использует только собственные байты; в telemetry fixture script slot равен нулю,
чтобы не читать игровые адреса. Полная сборка завершилась, fixtures только
скомпилированы. После снятия hold сначала выполнить отложенные проверки reader,
затем проверить журнал на обычном Continue и переходах сюжета.

## Native Lua SaveGame binding (2026-11-19)

Pinned EXE only; addresses require the full hash recorded above. The function
registration pair at 0x1aaa368 maps string SaveGame (0x196e1f0) to 0xad6620.
This bridge parses Lua argument index 2 through numeric-to-integer helper
0x440430, obtains interface 0x1cc395c via 0xb2d230, then calls interface slots
+0x34 and +0x38. Constructor 0xb66690 installs table 0x1974668 at main+4;
these slots resolve to 0xb6e130 and 0xb4efc0, respectively.

The first method is a request path, not a durable-save result:

- 0xb6e136 checks interface+0x1c bit 1 and returns zero on that branch;
  interface+0x44 is another rejection check. These flags' complete semantics
  and runtime timing are not established.
- 0xb6e158 reads and increments global counter 0x1aabaa0. The previous value
  becomes a ticket, distinct from the incoming numeric argument.
- 0xb6e167 retains the caller's argument in the second word of a temporary
  three-word record. 0xb6e193 passes the record to vector insertion 0xb6cf40
  at interface+0x50. The insertion's copy at 0xb6cf84 confirms the layout:
  ticket, numeric argument, reference-managed object. No manual ownership or
  engine-call API is being proposed here.
- Nonzero interface state can take a zero-return branch after insertion;
  state 9 also checks the queue length. The idle path changes state to 9
  and calls main vtable+0x18. Its consumer/completion work is not audited yet.
- 0xb6e20d returns the generated ticket on the remaining path.

Slot +0x38 (0xb4efc0) searches 12-byte records in interface+0x54..+0x58 for
that ticket and a nonnull third word. A hit copies the managed reference;
otherwise it creates an object through 0x11e6bd0 / 0x11e2200 / 0x11e7680.
The Lua bridge then pushes its object result via 0x11818d0 and returns integer
1 as the Lua C function's result count. That final 1 is not save success.
The returned object's completion/failure protocol is still unverified.

Keep the Lua numeric save argument, local request ticket, story event ID,
script progress pair and durable host progress separate. Neither the ticket,
queue membership nor a returned object establishes a finished disk write or a
shared checkpoint. Counter initialization/wrap/lifetime are unaudited; do not
send it as a cross-client identity.

Exact-byte checks for the binding, installed slots, ticket generation, record
copy and lookup matched the pinned file in scripts/audit_story_identity.py.
No game API was called. Next static target: main vtable+0x18's queued-save
consumer, managed object's completion/failure transitions, and the actual
write/commit path. Live confirmation remains deferred by the user's test hold.

## Queue notification and next request (2026-11-19)

The installed main-object table 0x19746b8 maps slot +0x0c to update 0xb6a6d0
and slot +0x18 to 0xb2d070. The latter creates operation table 0x1973444 with
its supplied numeric argument at operation+8 and installs it via 0xb2c9c0.
This connects the previously inspected Lua request to that concrete operation.
All offsets in this section are relative to the main object, not main+4's
save interface.

Update first invokes operation+vtable+4 at 0xb6a7b7. If current main+0x10 still
equals pending main+0x14 afterward, it returns. Otherwise it releases/replaces
the operation and may run its replacement before reaching state dispatch.
Thus one callback return or operation disappearance is not final success.

At 0xb6a7e3, main+0x1c selects a 1..9 jump table; state 9 resolves to 0xb6a8bd.
That branch:

1. Loads the first queue record through main+0x58 and calls 0x11e7680 on its
   third word (record+8).
2. Calls a listener at main+0x0c, vtable+0x1c, passing main+0x18 as a byte.
3. Erases that record through 0xb5ede0 using the container at main+0x54.
4. If another record remains, takes its second word and calls main vtable+0x18
   to start the next operation. Otherwise it clears main+0x1c.

The object factory at 0x11e2200 initializes object+8 to 1 and installs table
0x190cc0c. Notification 0x11e7680 clears bit 0 at object+8 before traversing
its waiter structures. There is no explicit success/error argument at this
notification call. This is evidence of a notification mechanism, not enough
evidence to read that bit as durable host-save success. Full waiter resumption,
listener semantics and backend error propagation remain unaudited.

The save operation also has a UI/timing stage: 0xb40956 calls 0xb2c800 with
mode 2 and interval 1000000; 0xb2c860 compares a time source with stored timing
and may call UI routines. Its boolean return must not be interpreted as disk
success. The earlier SAVE operation then reaches backend global 0x1ac0c50,
vtable+0x10 at 0xb40932; that backend and its commit/error contract are the
next static target.

New exact-byte checks for both installed slots, operation creation, dispatch,
state-9 branch, notification, next-record forwarding and object flag mutation
matched the full pinned EXE. This work did not execute the game, test suites,
fixtures or live memory reads. Runtime and two-PC acceptance remain pending.
