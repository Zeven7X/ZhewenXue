TITLE_COLOR = \033[33m
NO_COLOR = \033[0m

# when executing make, compile all exe's
all: sensor_gateway sensor_node file_creator

# When trying to compile one of the executables, first look for its .c files
# Then check if the libraries are in the lib folder
sensor_gateway : main.c connmgr.c datamgr.c sensor_db.c sbuffer.c lib/libdplist.so lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** CPPCHECK *****$(NO_COLOR)"
	cppcheck -f --enable=all --suppress=missingIncludeSystem main.c connmgr.c datamgr.c sensor_db.c sbuffer.c
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_gateway *****$(NO_COLOR)"
	gcc -c main.c      -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o main.o      -fdiagnostics-color=auto
	gcc -c connmgr.c   -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o connmgr.o   -fdiagnostics-color=auto
	gcc -c datamgr.c   -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o datamgr.o   -fdiagnostics-color=auto
	gcc -c sensor_db.c -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o sensor_db.o -fdiagnostics-color=auto
	gcc -c sbuffer.c   -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o sbuffer.o   -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_gateway *****$(NO_COLOR)"
	gcc main.o connmgr.o datamgr.o sensor_db.o sbuffer.o -ldplist -ltcpsock -lpthread -o sensor_gateway -Wall -L./lib -Wl,-rpath=./lib -lsqlite3 -fdiagnostics-color=auto

file_creator : file_creator.c
	@echo "$(TITLE_COLOR)\n***** COMPILE & LINKING file_creator *****$(NO_COLOR)"
	gcc file_creator.c -o file_creator -Wall -fdiagnostics-color=auto

sensor_node : sensor_node.c lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_node *****$(NO_COLOR)"
	gcc -c sensor_node.c -Wall -std=c11 -Werror -o sensor_node.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_node *****$(NO_COLOR)"
	gcc sensor_node.o -ltcpsock -o sensor_node -Wall -L./lib -Wl,-rpath=./lib -fdiagnostics-color=auto -DLOOPS=5

# If you only want to compile one of the libs, this target will match (e.g. make liblist)

allDynamicLibs: lib/libdplist.so lib/libtcpsock.so

lib/libdplist.so : lib/dplist.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB dplist *****$(NO_COLOR)"
	gcc -c lib/dplist.c -Wall -std=c11 -Werror -fPIC -o lib/dplist.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING LIB dplist< *****$(NO_COLOR)"
	gcc lib/dplist.o -o lib/libdplist.so -Wall -shared -lm -fdiagnostics-color=auto

lib/libtcpsock.so : lib/tcpsock.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB tcpsock *****$(NO_COLOR)"
	gcc -c lib/tcpsock.c -Wall -std=c11 -Werror -fPIC -o lib/tcpsock.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING LIB tcpsock *****$(NO_COLOR)"
	gcc lib/tcpsock.o -o lib/libtcpsock.so -Wall -shared -lm -fdiagnostics-color=auto

#try:main.c connmgr.c datamgr.c sensor_db.c sbuffer.c lib/libdplist.so lib/libtcpsock.so
#	cppcheck --enable=all --suppress=missingIncludeSystem main.c connmgr.c datamgr.c sensor_db.c sbuffer.c
#	gcc sensor_node.c -o sensor_node -Wall -std=c11 -Werror -DLOOPS=10 -lm -L./lib -Wl,-rpath=./lib -ltcpsock
#	gcc -g main.c connmgr.c datamgr.c sbuffer.c sensor_db.c -o sensor_gateways -Wall -std=c11 -Werror -lm -L./lib -Wl,-rpath=./lib -ltcpsock -ldplist -lpthread -lsqlite3 -DTIMEOUT=5 -DSET_MAX_TEMP=20 -DSET_MIN_TEMP=20
#	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-origins=yes --track-fds=yes ./sensor_gateways 1234
#
#try1:datamain.c datamgr.c sbuffer.c lib/libdplist.so lib/libtcpsock.so
#	gcc sensor_node.c -o sensor_node -Wall -std=c11 -Werror -DLOOPS=5 -lm -L./lib -Wl,-rpath=./lib -ltcpsock
#	gcc -g main.c connmgr.c datamgr.c sbuffer.c sensor_db.c -o sensor_gateway -Wall -std=c11 -Werror -lm -L./lib -Wl,-rpath=./lib -ltcpsock -ldplist -lpthread -lsqlite3 -DTIMEOUT=20 -DSET_MAX_TEMP=20 -DSET_MIN_TEMP=10
#	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-origins=yes --track-fds=yes ./sensor_gateways 1234

run:
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-origins=yes --track-fds=yes  ./sensor_gateway 1234

runAllSensors:
	./sensor_node 129 5 127.0.0.1 1234 & ./sensor_node 112 4 127.0.0.1 1234

sensor1:
	./sensor_node 129 5 127.0.0.1 1234
sensor2:
	./sensor_node 112 4 127.0.0.1 1234
sensor3:
	./sensor_node 1 4 127.0.0.1 1234
sensor4:
	./sensor_node 21 4 127.0.0.1 1234
sensor5:
	./sensor_node 129 4 127.0.0.1 1234
sensor6:
	./sensor_node 18 10 127.0.0.1 1234
sensor7:
	./sensor_node 19 6 127.0.0.1 1234

# do not look for files called clean, clean-all or this will be always a target
.PHONY : clean clean-all run zip

clean:
	rm -rf *.o sensor_gateway sensor_node file_creator *~

clean-all: clean
	rm -rf lib/*.so




