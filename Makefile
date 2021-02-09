CPP_SHARED := -DUSE_LIBUV -std=c++11 -O3 -I uWebSockets/src -shared -fPIC uWebSockets/src/Extensions.cpp uWebSockets/src/Group.cpp uWebSockets/src/Networking.cpp uWebSockets/src/Hub.cpp uWebSockets/src/Node.cpp uWebSockets/src/WebSocket.cpp uWebSockets/src/HTTPSocket.cpp uWebSockets/src/Socket.cpp uWebSockets/src/Epoll.cpp src/addon.cpp
CPP_OSX := -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup

default:
	make targets
	NODE=targets/node-v14.0.0 ABI=83 make `(uname -s)`
	cp src/uws.js dist/uws.js
	for f in dist/*.node; do chmod +x $$f; done
targets:
	mkdir targets
	curl https://nodejs.org/dist/v14.0.0/node-v14.0.0-headers.tar.gz | tar xz -C targets
Linux:
	g++ $(CPP_SHARED) -Wno-unused-result -Wno-deprecated-declarations -static-libstdc++ -static-libgcc -I $$NODE/include/node -s -o dist/uws_linux_$$ABI.node
Darwin:
	g++ $(CPP_SHARED) $(CPP_OSX) -I $$NODE/include/node -o dist/uws_darwin_$$ABI.node
.PHONY: clean
clean:
	rm -f dist/LICENSE
	rm -f dist/uws_*.node
	rm -f dist/uws.js
	rm -rf targets
