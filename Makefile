ifeq ($(OS),Windows_NT)
    EXE = build\endgame.exe
else
    EXE = build/endgame
endif

build-windows:
	cmake -B build-release -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
	cmake --build build-release

build-linux:
	cmake --build --preset default


run:
	cmake --preset default
	cmake --build --preset default
	$(EXE) --uci

run-terminal:
	cmake --build --preset default
	$(EXE) --terminal

clean:
	cmake -E remove_directory build
