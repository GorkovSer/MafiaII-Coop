# Adaptive interpolation — работа только с кодом

Основа: `4ee648a`, актуальный origin/development при создании ветки
`codex/adaptive-interpolation`. PR21 уже включён. Весь новый блок **не собран
и не тестирован**: пользователь запретил компилятор, fixtures, тесты и игру.
Разрешены чтение исходников/diff, редактирование, коммиты и Draft PR.

## Аудит исходной версии

- Новые sequence добавляются один раз. Повтор того же снимка меняет только
  текущий authority, не создаёт ещё одну точку истории. Изменённый duplicate
  и rollback отклоняются; watermark PR21 сохраняется отдельно от count.
- Кольцо — фактически массив из 8 точек со сдвигом. При заполнении безусловно
  удаляется первая точка. Если приходит плотная пачка разных timestamps, она
  может вытеснить нужную для задержанного представления нижнюю границу.
- Пакеты в одну миллисекунду заменяют последний anchor; нулевой интервал не
  интерполируется. Восстановить порядок времени движения внутри такого burst
  нельзя: sender timestamp отсутствует.
- Fixed 100 мс компенсирует часть jitter; если следующего снимка нет, достигается
  последний anchor и возникает удержание. Пауза честнее выдуманного движения.
  Нет оценки устойчивой неравномерности и политики увеличения/уменьшения буфера.
- Receive gap >250 мс и скачки >20 XY / >3 Z сбрасывают траекторию. Scope:
  session, connection generation, world generation, epoch, player и visual_only
  сравниваются до добавления. NULL/invalid/withdrawal/TTL отзывают представление.
- State, mailbox и world freshness независимо проверяет production
  CoopRemoteSnapshotAt. Timeline не должен обновлять эти времена.
- Отдельные position/heading используют общий target. Короткая дуга, точные
  концы, неизвестный угол и неизменный raw authority PR21 сохраняются.
- Текущий implicit memset reset не имеет локальной конфигурации/диагностики.
  Нужны явная reconfigure/reset семантика и ограниченная per-instance статистика.
- Сохранение timeline watermark при retire визуальной модели и подключение
  frame.heading в игровые потребители — работа основного разработчика,
  эти файлы/участки здесь не меняются.

## Архитектура решения

Fixed 100 мс остаётся default для zero-initialized timeline. Отдельный opt-in
adaptive config; ограниченные arrival-window и delay, hysteresis и голосование
по новым положительным интервалам. Presentation time не уменьшается при смене
delay; warm-up/underflow/overflow и gap/discontinuity явно диагностируются.
Никаких прогнозов, динамических аллокаций или сетевых/игровых вызовов.

Все части написаны в этой ветке: production API/алгоритм, собственные сценарии,
автономный runner и trace replay. Ни одна часть не запускалась.

## Production API — написано, запуск запрещён

Добавлены DefaultConfig, Configure, Reset, ReadDiagnostics. Zero-init остаётся
fixed 100 мс; adaptive только по явному Configure. Недопустимые параметры
отклоняются без изменения экземпляра. Идентичная настройка — no-op, изменение
очищает историю/оценку/presentation, сохраняя config и принятый watermark.
Reset сохраняет config/watermark и cumulative counters; новая область/withdrawal
сбрасывают scope-local данные. Память ограничена 8 poses + 16 intervals.

Adaptive candidate defaults: delay 100..200 мс, warm-up 8 положительных
интервалов. Окно 16 интервалов; wanted = mean + 2*MAD +20 мс, с ограничением
min/max. Увеличение по 10 мс после 3 подтверждений, снижение по 5 после 20,
hysteresis 10/20 мс. Рост также требует хотя бы 3 отклонений >20 мс в окне,
устойчиво медленного cadence (mean+20 >= delay) либо нового underrun между
принятыми интервалами. Один outlier не получает 16 голосов только из-за
присутствия в окне. Bursts не голосуют; gap >250 исключается из окна.
Presentation не отматывается при изменении delay, обычный catch-up ограничен
1.25x; hard age cap при наличии данных имеет приоритет, см. ниже.
При underflow — удержание anchor без экстраполяции. В adaptive overflow
сохраняет нижний anchor, удаляя одну внутреннюю точку, если она ещё в будущем;
погрешность такого downsampling подлежит оценке по трассам. Fixed FIFO сохранён.

Диагностика: политика delay, фактический возраст presentation, прогрев, holding,
заполнение, arrival-window, burst/gap/underflow и причины отказов/reset.
Отказы считаются по вызовам, reset — по переходам состояния; счётчики насыщаются.
Все коэффициенты кандидаты, не измеренные оптимумы. Выполнено только чтение
исходников и diff; никаких результатов выполнения нет.

Обоснование кандидатов: при 20 Гц 100 мс — два номинальных интервала, 200 мс —
четыре; 250 мс остаётся отдельной границей отказа от мостика через receive stall.
Окно 16 точек соответствует примерно 0.8 с стабильной доставки, warm-up 8 —
примерно 0.4 с. Mean+2*MAD даёт запас на наблюдаемую вариацию, +20 мс — локальный
запас, не оценка RTT. Три голоса защищают от мгновенного роста, двадцать голосов
и шаг 5 мс делают снижение значительно медленнее роста. Из-за variable cadence
это голоса по событиям, не точные wall-clock таймеры. Все числа ещё требуют
сравнения трасс и проверки в игре; statistical confidence не заявляется.

## Отдельное статическое исправление: receiver clock rollback

Чтением PR21 выявлен путь: приняты seq11/received1050 при now1200, затем
поступает ещё свежий seq10/received1010 при now1100. Старый код сначала делал
полный reset по now<last_now, стирал watermark и мог принять seq10 заново.
Новый код до валидации/добавления отказывает при clock rollback, сохраняет
watermark и прежнюю нижнюю границу now. Повторные вызовы отказывают, пока время
не догонит границу; после этого старый sequence всё равно отклоняется.
Сценарий `tests/test_interpolation_clock.c` написан для будущего воспроизведения.
Это вывод из чтения кода, **не результат выполнения**. Семантика clock rollback
стала fail-closed и для fixed; обычная монотонная доставка не меняется.

## API и пример для последующей интеграции владельцем потребителя

```c
CoopRemoteTimeline timeline = {0};  /* Без Configure: fixed 100 ms. */
CoopTimelineConfig config;
CoopRemoteTimelineDefaultConfig(&config);
config.mode = COOP_INTERPOLATION_ADAPTIVE;
/* Сохранить локальную настройку, проверить отказ, не молча clamp. */
if (!CoopRemoteTimelineConfigure(&timeline, &config)) {
    /* Сообщить вызывающему об ошибке; не включать adaptive. */
}
/* На каждом callback: получить НОВЫЙ результат разрешённого receiver. */
CoopRemoteFrame frame;
if (CoopRemoteTimelineAt(&timeline, current_or_null, now_ms, &frame)) {
    /* position + heading/heading_valid относятся к frame.presentation_ms.
       Guard полномочий и расстояния остаётся на frame.authority/raw snapshot. */
}
CoopTimelineDiagnostics view;
CoopRemoteTimelineReadDiagnostics(&timeline, &view); /* Не меняет алгоритм. */
```

Это пример, не внесённая интеграция: visual_test.c/remote_actor.c не тронуты.
Не игнорировать false при Configure или At. Zero-init — обязательное исходное
состояние; внутренние поля нельзя изменять напрямую. Snapshot/output alias
`current == &frame.authority` по-прежнему поддерживается. Diagnostic output должен
быть отдельным объектом, не перекрывать память timeline. Thread-safety/locking
не добавлялись: один владелец потока, как раньше.

| Поле config | Default | Допустимо |
| --- | --- | --- |
| mode | FIXED | FIXED или ADAPTIVE |
| fixed_delay_ms | 100 | 0..200 |
| min_delay_ms | 100 | 50..max_delay_ms |
| max_delay_ms | 200 | min_delay_ms..200 |
| warmup_intervals | 8 | 4..16 |

Проверяются все поля даже у fixed. Некорректный config/NULL возвращает false
без изменений state/счётчиков. Идентичный valid config — no-op. Другой config
очищает poses, окно статистики, warm-up, votes, presentation и scope-local
counters, сохраняя newest/watermark и lifetime counters. Следующий валидный
current снова даёт anchor, без смешивания старого буфера. Reset делает такой
же reset с сохранением настройки; полный новый zero-init очищает всё. Нельзя
использовать reset для легализации старого пакета. Принятый ранее неизменный
duplicate может стать anchor после reset, но не новым статистическим событием.

При смене scope/withdrawal/invalid/TTL сбрасывается и потоковая identity, как в
PR21; это не новая гарантия сохранности watermark вне timeline. Последующий
current обязан прийти от актуального receiver. Retire модели и сохранность
watermark в visual_test.c остаются отдельной задачей владельца. Его memset
timeline также уничтожает config; владельцу нужно явно согласовать жизненный
цикл конфигурации. Самостоятельных правок этих участков здесь нет.

Диагностика — snapshot, без логов/файлов/сокетов/globals/game calls. `delay_ms` —
bounded policy target; `effective_delay_ms` — now минус реально показанное время.
При провале доставки возраст последней доступной позы может быть больше 200 мс:
это недостаток данных, а не разрешение продлить TTL. Когда точки для ограничения
возраста уже есть, playhead не ниже min(now-max_delay, newest); ограничение
не позволяет накопить бесконечный playback debt. Такой forward clamp может
быть заметен, считается в delivery.latency_clamps и имеет приоритет над 1.25x.
Он не отматывает время и не создаёт будущую позу. Последние reason — исторические,
не текущая авторизация. Массивы
причин индексируются именованными enum из remote_timeline.h; счётчики cumulative
на экземпляр, delivery counters — с последнего stream/config/explicit reset.
Rejected packet очищает playback/window, сохраняя watermark и delivery totals;
gap/relocation очищают окно и playback, сохраняя totals и выбранный delay.

## Время представления, прогрев и провалы

До достаточного числа точек используется доступный anchor; warming не является
permission gate. Обновление политики начинается после 8 положительных интервалов
по default. Используются только локальные receive timestamps, не sender time.
При увеличении delay playhead может удерживаться, но не убывает. Снижение delay
догоняется со скоростью не более elapsed + floor(elapsed/4); при одинаковом now
временного бюджета нет. При последнем шаге к min после стабилизации допускается
снижение и в deadband, иначе политика могла бы навсегда застрять на min+5/min+10.

Краткий underflow удерживает последний доступный pose. Позиция и угол берутся
с одного момента; неизвестный угол интервала не восстанавливается из будущего.
Длительный gap >250 мс начинает новый segment с нового anchor, без мостика через
провал. Bursts в одну миллисекунду coalesce; raw authority всегда последний,
но выдуманного времени между такими пакетами нет. Остановка не экстраполируется;
при новом движении используются только реальные endpoints. XY/Z relocation
сбрасывает и позиционную, и угловую историю. NULL/withdrawal/TTL немедленно дают
false/нулевой frame; fallback не удерживает активным отозванного игрока. Отдельный
clock-rollback отказ сохраняет clock floor/watermark до восстановления времени.

Adaptive overflow сохраняет оба конца текущего playback interval, удаляя более
позднюю внутреннюю точку, если все следующие ещё в будущем. Иначе удаляется
старейшая точка, для которой есть более свежая нижняя граница. Это ограниченный downsampling, не гарантия
точности сложного движения. Между сохранёнными точками интервал может стать
длиннее 250 мс; это не receive stall: каждый реальный gap был отсечён до append.
Fixed по-прежнему вытесняет самый старый элемент FIFO. Ring capacity — 8,
interval window — 16, дополнительной production .c единицы нет, аллокаций нет.

## Написанные, НО НЕ ЗАПУЩЕННЫЕ сценарии

`tests/test_adaptive_interpolation.c`: 16 групп — config bounds/reconfigure/reset,
стабильные 20 Гц, repeated mailbox/read-only diagnostics, timestamp bursts,
длительный jitter и 520 stable intervals восстановления, независимые экземпляры,
изолированный outlier после warm-up, монотонный playhead при росте delay,
underflow/gap/return burst, stop/start, обе дуги ±π/unknown/exact endpoint/XY-Z
jump, bound возраста доступных данных после underflow, все 6 scope полей/withdrawal, три freshness clocks и sequence wrap /
изменённые/старые duplicates, adaptive overflow и сохранность уже отображаемого
нелинейного интервала при переполнении.

`tests/test_interpolation_clock.c`: отдельный сценарий clock rollback.
Runner также включает прежние 17 групп `test_remote_timeline.c` без изменения
их исходника. Прежние результаты PR21 не являются результатом нового кода.
Все новые проверки проверяют наблюдаемый frame/diagnostics/return; внутренние
формулы адаптации не копируются в тесты.

## Replay и метрики — только подготовлено

`tests/interpolation_trace.c` вызывает тот же production CoopRemoteTimelineAt.
Шесть заранее заданных трасс: stable, jitter, gap-burst, recovery, stop-turn,
relocation. Кадр 5 мс, источник 20 Гц. Jitter задаётся детерминированным шаблоном
задержек 20/20/150/20/20/40/120/20; сериализация очереди создаёт same-ms bursts.
Gap-burst пропускает заданный диапазон, затем меняет session/connection. Это
известные синтетические omissions, не вывод о потере пакетов из receive stats.
Recovery после 160 пакетов возвращает стабильность. Truth — аналитическая
позиция с остановкой/возобновлением и поворотом через ±π; relocation добавляет
30 единиц. Общая шкала времени source/receiver известна только генератору.

Для fixed/adaptive создаются четыре CSV: frames (playhead, delay/age, pose/error,
reasons/clamps), holds (интервалы и длительность), delays (policy и effective-age
histograms только доступных кадров) и summary (RMSE,
max position error, mean angular error, rewind и счётчики причин).
Отдельно учитывается holding во время доставки и после последней доставки:
общий хвост 2100 мс специально доводит до TTL, не сравнивайте его с реальным
jitter. Ошибка считается относительно truth в presentation time, а не now;
это не измерение RTT/сетевой latency и не физическая точность в Mafia.

Replay явно наблюдает каждое delivery event и затем render tick. Настоящий
mailbox может coalesce несколько пакетов между callback; replay такой потери
наблюдений не моделирует. Буфер не способен восстановить невидимые снимки.
Файл CSV/результаты заранее не сгенерированы; никакой второй реализации policy
в replay нет, truth-функции описывают только исходную траекторию.

| Трасса | Fixed holding/error/delay | Adaptive holding/error/delay | Статус |
| --- | --- | --- | --- |
| stable | — | — | не запускалась |
| jitter | — | — | не запускалась |
| gap-burst | — | — | не запускалась |
| recovery | — | — | не запускалась |
| stop-turn | — | — | не запускалась |
| relocation | — | — | не запускалась |

## Команды ТОЛЬКО после снятия запрета

Из корня ветки, закреплённый LLVM-MinGW 20261104 / clang 23.1.5:

```powershell
./tests/run-adaptive-interpolation.ps1 -Toolchain '<LLVM-MinGW>' -ReplayTraces
./tests/test-remote-timeline.ps1 -Toolchain '<LLVM-MinGW>'
./scripts/build-probe.ps1 -Toolchain '<LLVM-MinGW>' -OutputDirectory build/adaptive-full
```

Первый runner компилирует legacy/clock/adaptive tests в O0/O2, затем optional
replay, второй включает существующие actor/foundation regressions. Продукты
ignored build/adaptive-interpolation; при дефекте runner отказывает, а не
записывает успех. Для одной трассы после будущей сборки:

```powershell
./build/adaptive-interpolation/interpolation_trace.exe jitter build/adaptive-interpolation/manual-jitter
```

Общие build/package scripts не менялись. Для production дополнительных source
файлов нет: существующая сборка уже включает remote_timeline.c. Для новых tests
нужен явный вызов автономного runner. Структуры выросли: пересобрать всех C
потребителей; wire-протокол и версия модуля не менялись. Упаковка нового бинарника
и его соответствие исходникам — задача основного разработчика после проверок.

## Условия оставить adaptive выключенным

- Пока код не собран и не пройдены все сценарии/legacy regressions.
- При rewind presentation вне явно определённого reset, NaN, overshoot,
  смешивании scope, нарушении TTL или мутации authority.
- Если trace comparison не даёт приемлемого компромисса holding/error/delay,
  либо политика дрожит/не возвращается к min после стабилизации.
- Если adaptive downsampling и warm-up ухудшают видимость/повороты или вызывают
  неприемлемую добавочную задержку на двух реальных ПК.

До будущей игровой проверки неизвестны приемлемые пределы delay, пороги gap/
relocation и параметры MAD/votes, качество stop/start и частота callback.
Тесты игры только по отдельному разрешению, в окне, без звука только Mafia,
не закрывая уже запущенную игру пользователя. Не заявляется готовность сюжетного
кооператива, анимаций/боёв/транспорта. Ветка предназначена для Draft PR и review,
не для самостоятельного merge или изменения потребителей.

**Новый код не собирался и не тестировался.** Итоговая проверка в этой задаче —
чтение исходников и diff, проверка разрешённого состава файлов; таблицы намеренно
пустые. Общие COAUTHOR_HANDOFF.md/TESTING.md не изменялись по поручению пользователя.
