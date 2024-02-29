# freeLib
freeLib - каталогизатор для библиотек LibRusEc и Flibusta

Это форк общедоступного freeLib 5.0 , разработка которого прекращена. 
![screenshot](./freeLib/doc/screenshot.png#gh-light-mode-only)
![screenshot](./freeLib/doc/screenshot-dark.png#gh-dark-mode-only)
* Создание собственных библиотек на основе файлов FB2(.ZIP), EPUB, FBD.
* Конвертация в форматы AZW3 (KF8), MOBI, MOBI7 (KF7), EPUB.
* Работа с несколькими библиотеками.
* Импорт библиотек из inpx-файлов.
* Поиск и фильтрация книг.
* Серверы OPDS и HTTP.
* Сохранение книг в выбранную папку.
* Различные настройки экспорта для нескольких устройств.
* Отправка выбранных файлов книг на email (Send to Kindle).
* Установка тегов для книги, автора, серии и фильтрация по тегам.
* Настройка форматирования книг (шрифты, буквица, заголовки, переносы, сноски)
* Чтение книг с помощью внешних приложений. Можно назначить отдельную программу для каждого формата.

#### Сборка и установка из исходников в Ubuntu

Установить необходимые компоненты:
```
sudo apt update && sudo apt-get install git cmake build-essential qtbase5-dev libqt5sql5-sqlite libquazip5-dev
```
или в Ubuntu ≥ 22.04 можно установить Qt6:
```
sudo apt update && sudo apt-get install git cmake build-essential qt6-base-dev libqt6core5compat6-dev libqt6svg6 zlib1g-dev
```
Скачать исходники программы:
```
git clone --recurse-submodules https://github.com/petrovvlad/freeLib.git
```
Собрать  и установить:
```
mkdir freeLib/build && cd freeLib/build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .. && cmake --build . -j2
sudo make install
```

Для конвертации книг в EPUB, AZW3, MOBI необходимо установить kindlegen.

#### Обсуждение
канал Matrix:  [#freeLib:matrix.p-vlad.ru](https://matrix.to/#/#freeLib:matrix.p-vlad.ru)
