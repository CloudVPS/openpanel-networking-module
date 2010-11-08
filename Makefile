# This file is part of OpenPanel - The Open Source Control Panel
# OpenPanel is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License as published by the Free 
# Software Foundation, using version 3 of the License.
#
# Please note that use of the OpenPanel trademark may be subject to additional 
# restrictions. For more information, please visit the Legal Information 
# section of the OpenPanel website on http://www.openpanel.com/

include makeinclude

OBJ	= main.o version.o

all: networkingmodule.exe module.xml down_network.png
	./addflavor.sh
	grace mkapp networkingmodule 

down_network.png: network.png
	convert -modulate 50,100,100 network.png down_network.png

version.cpp:
	grace mkversion version.cpp

module.xml: module.def
	mkmodulexml < module.def > module.xml

networkingmodule.exe: $(OBJ) module.xml
	$(LD) $(LDFLAGS) -o networkingmodule.exe $(OBJ) $(LIBS) \
	/usr/lib/opencore/libcoremodule.a

clean:
	rm -f *.o *.exe
	rm -rf networkingmodule.app
	rm -f networkingmodule

SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I../opencore/api/c++/include -c -g $<
