ifeq ($(OS),Windows_NT)
    EXE = build\endgame.exe
else
    EXE = build/endgame
endif

build-windows:
	cmake -B build-win -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
	cmake --build build-win

install:
	cmake --preset default -DCMAKE_BUILD_TYPE=Debug

run:
	cmake --build --preset default
	$(EXE) --uci

run-terminal:
	cmake --build --preset default
	$(EXE) --terminal

clean:
	cmake -E remove_directory build