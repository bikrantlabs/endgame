ifeq ($(OS),Windows_NT)
    EXE = build\endgame.exe
else
    EXE = build/endgame
endif

install:
	cmake --preset default

run:
	cmake --build --preset default
	$(EXE)

clean:
	cmake -E remove_directory build