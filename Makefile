include makeinclude

OBJ	= main.o version.o

all: networkingmodule.exe module.xml down_network.png
	./addflavor.sh
	mkapp networkingmodule 

down_network.png: network.png
	convert -modulate 50,100,100 network.png down_network.png

version.cpp:
	mkversion version.cpp

module.xml: module.def
	mkmodulexml < module.def > module.xml

networkingmodule.exe: $(OBJ) module.xml
	$(LD) $(LDFLAGS) -o networkingmodule.exe $(OBJ) $(LIBS) \
	../opencore/api/c++/lib/libcoremodule.a

clean:
	rm -f *.o *.exe
	rm -rf networkingmodule.app
	rm -f networkingmodule

SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I../opencore/api/c++/include -c -g $<
