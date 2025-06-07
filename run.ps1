clear
cd C:\code_in_laptop\chatter2\client\
Remove-Item -Path C:\code_in_laptop\chatter2\client\build -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "build"
cd build

cmake .. -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1120_64/bin/gcc.exe `
    -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1120_64/bin/g++.exe `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/mingw_64 `
    -DCMAKE_BUILD_TYPE=Debug

mingw32-make

.\chatter_client.exe