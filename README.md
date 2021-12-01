# freeLib
freeLib - каталогизатор для библиотек LibRusEc и Flibusta

Это форк общедоступного freeLib 5.0 , разработка которого прекращена. 

#### Сборка и установка из исходников в Ubuntu 21.10

Установить необходимые компоненты:
```
sudo apt update
sudo apt-get install cmake build-essential qtbase5-dev qtwebengine5-dev libqt5xmlpatterns5-dev libquazip5-dev
```
Скачать исходники программы:
```
git clone --recurse-submodules https://github.com/petrovvlad/freeLib.git
```
Собрать  и установить:
```
mkdir freeLib/build && cd freeLib/build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DQUAZIP_STATIC:BOOL=ON .. && cmake --build .
sudo make install
```
