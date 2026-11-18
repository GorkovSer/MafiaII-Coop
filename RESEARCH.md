# Журнал исследования

## 2026-11-17: восстановление состояния

До текущего этапа были проверены публичные страницы GitHub, README и страницы релизов. Локального проекта, истории Git или сборки не было. Это не были игровые испытания.

| Основа | Назначение и версия | Сборка/зависимости | Проверка и ограничения |
|---|---|---|---|
| https://github.com/mafia2online/m2o-reborn | Исходники мультиплеера Steam Mafia II Classic, Apache-2.0 | CMake, VS2017, librg/ENet, m2sdk, Steam API; CEF опционален | Клон получен, commit `4af7edfec2e450112943f69926f4a394d5868ea6`. GitHub Releases пуст. Код запускает freeride; не готовая основа кампании |
| https://github.com/mafia2online/Mafia2-Online | Старый RC4 на основе M2MP | По README VS2015, клиент/сервер/launcher; лицензии ещё требуют локальной проверки | Архивирован; актуальный код и зависимости проверяются. Не считать проверенным сюжетным кооперативом |
| https://github.com/Verraton/BRIX-v0.9 | Скриптовый режим для отдельного сервера Mafia 2 Online | Squirrel .nut, meta.xml, нужен сервер M2O; по GitHub MIT | Не самостоятельный сетевой клиент. Наличие режима не подтверждает поддержку сюжетных глав |

## Подтверждено по исходникам m2o-reborn

Все пути ниже относятся к зафиксированному upstream commit, локальный клон хранится только в игнорируемом `work/upstream/`.

- `projects/client/mod.cpp:215`: `StartGame(TYPE_NEW_GAME, 0, 1, "freeride", false)` вызывается при инициализации модуля.
- `projects/client/mod.cpp`, `mod_respawn`: отключаются ambient/generators, изменяются здоровье, позиция и загрузочные наборы.
- `projects/client/win32.cpp:205`: переопределяется `freeride.sds`.
- `projects/launcher/launcher.cpp`: используется Steam API; требования README ограничивают запуск Steam-версией.
- `vendor/m2sdk/include/classes/CGame.hpp`, `CCore.hpp`: игровые singleton-адреса зафиксированы численно. Для установленного GOG EXE они ещё не валидированы.

Вывод: кандидат для изучения SDK/репликации, но перенос всего клиента подменит сюжет freeride. Не вызывать SDK до проверки версии и адресов. Предварительное направление — отдельная загрузка, сохраняющая одиночную кампанию, с переиспользованием только проверенных компонентов. Окончательный выбор после аудита трёх исходников и GOG EXE.

## Современные проекты, найденные ранее

- https://mafia2online.com/ — авторы заявляют V2 public beta и серверный free roam. Публичный код V2 по ссылке `https://github.com/mafia2online/Mod` недоступен (404 при проверке). Не заменять его старым m2o-reborn молча.
- https://mafiam.com/ — авторы заявляют Mafia II Definitive Edition и свободный мир без сюжета; текущая установленная игра Classic.
- https://github.com/JustAddMP/MafiaDE — кооператив для ремейка **первой** Mafia, не Mafia II.

## Установленная игра

Пользователь подтвердил каталог установки. Найдены GOG metadata gameId `1816059595`, buildId `51080548070633011`, исполняемый файл `pc/mafia2.exe`, FileVersion `1. 0. 0. 1`, размер 24338944 байт, SHA-256 `16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f`.

PE32 x86 (`Machine=014C`), image base `00400000`, entry RVA `00A75B0D`, CRC32 `87669A6D`. Скрипт read-only анализа: `scripts/audit_game.py`. Эти metadata не доказывают совместимость с SDK.

## Дополнительные проверки исходников и EXE

- Mafia2-Online зафиксирован на `6e941527fa63195430d0149f8f680f3f5da3631f`. `Client/CCore.cpp` вызывает `COffsets::Initialise(0, ...)`. Таблица `COffsets.cpp` различает старые v1 и Steam адреса, не GOG-совместимость.
- `Shared/SharedUtility.cpp`, `IsEXESignatureAllowed`: только CRC32 `97FA8A06`, `189BB75A`, `2F388D11`, `AE196A1E`. Алгоритм `Shared/CRC.h` — обычный CRC32; установленный EXE `87669A6D` не входит в список.
- `Client/CPatches.cpp`, `Initialise`: отключает сохранения, CityShops и DLC loading. Переносить эти патчи в сюжетный проект нельзя.
- В корне RC4 отдельная LICENSE не найдена. Не считать доступность исходников разрешением на копирование всех компонентов.
- BRIX зафиксирован на `eea35b9f7e9bf342ce672c1d7ce7b9e5c5d95669`, локальный LICENSE подтверждает MIT.
- Дизассемблирование GOG EXE через llvm-objdump: пять байт в `0x4F2F05` соответствуют инструкциям, воспроизводимым `GameModuleInstall` в m2sdk (`mov eax,[edx+1Ch]; push 0Ah`). Это подтверждает конкретное совпадение участка, **не** совместимость всего SDK или безопасность всех патчей `Initialize`.
- `CTickedModuleManager.hpp` SDK содержит `AddAction`, события GAME_INIT/TICK/LOADING_STARTED/LOADING_FINISHED. Адреса и ABI AddAction для установленной игры ещё не подтверждены; вызовов не было.

## Текущее решение

Создан отдельный диагностический загрузчик и DLL без стороннего игрового кода. Они сохраняют обычную точку входа EXE и не запускают freeride. Собственная DLL действительно выполнила код внутри x86 игры. Внешняя проверка вне ограниченного окружения подтвердила живую игру, отвечающее окно `Mafia II` и правильный путь EXE. Первое заключение о закрытии процесса основывалось на неполной видимости процессов и не считается установленным сбоем игры. Следующий шаг: подтвердить меню, затем валидировать только минимальный игровой callback. Старые массовые SDK-патчи не переносить.

## Read-only наблюдение живого процесса

`scripts/inspect_runtime.py` проверяет SHA256 образа и читает только три candidate globals через ReadProcessMemory. Проба в диагностическом процессе: `reborn_game` и `reborn_ticked_manager` указывают на читаемые структуры, первые элементы предполагаемых vtable ведут в исполняемые секции проверенного EXE. У старого `rc4_game` такая проверка не проходит. Это дополнительный аргумент исследовать m2o-reborn SDK первым; ещё не доказательство типа объекта, всех offsets, ABI или корректности вызова AddAction. Память не изменялась, игровых вызовов не было, сырые адреса/байты и полный runtime report в Git не включены.

## 2026-11-18: подготовка минимального игрового callback

По GOG EXE: начало `0x11A0C10` соответствует кандидату AddAction, в дизассемблировании имеется `ret 0x1c` (семь stack arguments), `this` используется из ECX. Исходник `CTickedModuleManager.hpp` и `mod_install` задаёт те же семь аргументов и IDs событий. Конкретный стартовый участок `0x4F2F05` соответствует пяти байтам из SDK bridge; весь Initialize SDK не переносится.

Собственный `observer.c` реализует отдельную проверку SHA256, двух сигнатур и минимальный startup bridge. C callback имеет thiscall и выравнивает стек. Последующая реальная проба в GOG EXE подтвердила принятие всех 22 регистраций и вызовы событий меню, открытия/закрытия миссии, тиков/кадров и паузы. Регистрация и наблюдавшиеся callbacks шли на одном игровом потоке; выход завершился с кодом 0. Остальные методы SDK этим не проверены.

Ранняя загрузка сначала провалилась на test host из-за отсутствия готового PEB module list. Изменён способ определения отображённых DLL и инициализации loader до WinMain; реальный test host затем подтвердил `test_host_entry_seen=0` и завершился с кодом 0. Это тест загрузчика, не имитация сетевого клиента.

В импортах GOG EXE обнаружен SHGetFolderPathA; в проверенном common.dll такой импорт не найден. Реализован процессный redirect LocalAppData для тестовой копии профиля, без изменения системного пути и оригинального save directory. Проверка собственным host прошла: запрос LocalAppData вернул изолированный root, реальная запись попала в него, CSIDL_WINDOWS остался обычным. В последующей игровой пробе redirect сработал, в копии появились новые файлы, исходный профиль после завершения совпал с backup. Полный цикл checkpoint/save/reload ещё не проверен.

## Локальный сюжетный игрок: позиция без вызовов движка

На реальном объекте из `CGame + 0x118` прочитан virtual GetPosition `0x938250` (slot `0x8C`). Код установленного EXE сначала проверяет физическое представление `player+0x190`, вызывает его slot `0x54` и копирует три float; при отсутствии физики читает модель `+0x60` и координаты `+0x44/+0x54/+0x64`. Реальный physics getter — `0x18AE690`: только `lea eax,[ecx+0x1D8]; ret`. Поэтому новый адаптер читает физическую позицию напрямую, без произвольного вызова vtable. При других targets снимок отклоняется.

Внешнее наблюдение движения дало 75 снимков за 15 секунд, один локальный GUID и изменяющиеся координаты. Реальный первый внутрипроцессный тест обнаружил, что между game_init и load_finished уже существуют объекты с временной нулевой позицией. Готовность по одному game_init недостаточна: добавлено ожидание обоих событий и блокировка на load_started/закрытии. См. docs/PLAYER_STATE.md; это ещё не сетевой поток и не создание второго персонажа.

## 2026-11-18 — entity factory audit on the pinned GOG build

The upstream m2sdk CEntityFactory and wrappers/entity.hpp at commit `4af7edfec2e450112943f69926f4a394d5868ea6` identify Human as type 0x0e and Player as 0x10. Local disassembly of the pinned EXE at 0x1167a60 confirms a cdecl dispatch: global head 0x1abe8cc, next pointer at +0, type at +4, factory callback at +0x10. A matching type tail-jumps to the callback; an absent type returns null. This does not establish safe setup or destruction.

Added a bounded read-only registry walker to inspect_runtime.py. It stops on unreadable records, cycles, a 256-record limit or duplicate candidate types. Addresses and callback pointers stay in the private report; the console summary reports only counts and whether candidate targets belong to executable image sections. No constructors, SDK initialization or memory writes occur.

A real read-only inspection during the first chapter found 48 records, reached the end without a cycle and found both candidate types. Human callback 0x999d60 allocates 0x220 bytes through 0x401830 and tail-jumps to 0x999050 on success. Player callback 0x430970 allocates 0x3d0 bytes and tail-jumps to 0x42f9a0. These are pinned-image analysis findings, not a successful second-character spawn. Before calling either path, validate model loading/ownership, setup, placement, removal and mission unload behavior. Prefer an ordinary Human for the remote avatar until effects of a second Player on story ownership are understood.

## 2026-11-18 — removal and mission-name constraints

The old m2sdk entity wrapper calls Deactivate, Release, an explicit base destructor and (for Human) Free. That sequence must not be copied as an ownership contract. On the pinned GOG image, Release at 0x119ddd0 has both a manager/deferred branch and a branch reaching the object's first virtual slot with stack argument 1 (0x119de53..0x119de5b). This is consistent with a deleting destructor; further destructor/Free calls could reuse an already released object. No removal calls were made. Inspect the concrete Human deleting destructor and manager queue lifecycle before implementing cleanup.

A second negative finding: StartGame's new-game path copies its mission-name argument to the GameBase global (0x1ab2838) object at +0x0c; the helper at 0x450cd0 clears this buffer and the category field at +0xc0. During the current first-chapter introduction, bounded external read-only inspection found that string empty and category 1. This is a request buffer, not an established current-chapter identifier; it must not become the network world's mission identity. No candidate mission APIs were invoked.

## 2026-11-18 — loaded map roots are not story identity

On the rehashed pinned GOG EXE, CGame vtable +0x14 is 0x5cd760. Its Open formats /games/%s.bin and stores the resident buffer at CGame+0x2c. Parser 0x5c9090 assigns four strings from GMP-v6 chunk 0xe001 to +0x0c/+0x10/+0x14/+0x18. A local read-only resource inspection confirms these are asset roots, not chapter names. The SDS wrapper/metadata was read through the existing zlib-licensed Gibbed-based local adapter; extracted game data and adapter changes remain ignored, not distributed.

The native save/load path supplies another candidate: 0xb2d230 returns interface address 0x1cc395c, and 0xb2c690 returns its +0x68 DWORD (direct interface call at 0xb4058f). The read-only inspector records it for later comparison with the save catalogue, without treating it as an accepted current checkpoint. Different this/subobject offsets must not be mixed. See docs/STORY_IDENTITY.md; no runtime mission-identity validation or game calls were performed in this stage.
