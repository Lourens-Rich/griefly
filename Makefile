.PHONY: all
all: Exec/KVEngine

.PHONY: travis-get-deps
travis-get-deps:
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo add-apt-repository -y ppa:beineri/opt-qt541
	sudo add-apt-repository -y ppa:sonkun/sfml-stable
	sudo add-apt-repository -y ppa:ubuntu-sdk-team/ppa
	sudo add-apt-repository -y ppa:kalakris/cmake
	sudo apt-get update -qq
	sudo apt-get install --yes gcc-4.9 g++-4.9
	sudo apt-get install --yes libsdl-ttf2.0-dev libsdl-image1.2-dev libsdl-mixer1.2-dev \
		libsdl-net1.2-dev zlib1g-dev libsfml-dev libpng-dev
	sudo apt-get -qq install qt54tools qt54base qt54svg qt54webkit qt54creator
	sudo apt-get install cmake

.PHONY: clean
clean:
	rm -rf build
	rm -f Exec/KVEngine

Exec/KVEngine:
	mkdir -p build
	cd build && cmake ../ && make CXX=$(CXX) CC=$(CC)
	cp build/KVEngine Exec
