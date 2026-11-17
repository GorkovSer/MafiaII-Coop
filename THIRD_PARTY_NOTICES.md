# Сторонние компоненты

На текущем этапе сторонний исходный код в проект не включён. Клоны кандидатов — только локальные материалы исследования в `work/upstream`, не содержимое распространяемого пакета.

- m2o-reborn, commit `4af7edfec2e450112943f69926f4a394d5868ea6`: корневой LICENSE — Apache License 2.0; отдельные vendor-компоненты требуют собственного аудита перед включением. Ссылка: https://github.com/mafia2online/m2o-reborn
- BRIX-v0.9: публичная страница сообщает MIT; точный commit и полный LICENSE проверяются до переиспользования. https://github.com/Verraton/BRIX-v0.9
- Mafia2-Online: условия использования и каждого bundled dependency ещё не подтверждены. https://github.com/mafia2online/Mafia2-Online

Сборка собственного диагностического кода использует LLVM-MinGW 20261104 (clang 23.1.5), закреплённый URL/SHA256 в `dependencies/toolchain.json`. Toolchain не включена в Git. Перед распространением статически связанных бинарников сохранить относящиеся к ним notices из toolchain LICENSE.TXT (LLVM/Clang, compiler-rt, mingw-w64). На текущем этапе бинарники локальные, Release не публиковался.

Не распространять Steam SDK, игровые DLL/EXE/SDS, ресурсы или чужие бинарники на основании одной лицензии корневого проекта. При включении разрешённого компонента сохранять его copyright, LICENSE/NOTICE и фиксировать commit.
