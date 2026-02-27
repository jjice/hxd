.PHONY: all clean install

all:
	cmake -B build -S .
	cmake --build build

clean:
	rm -rf build

install:
	cmake --install build