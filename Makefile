CXX 	= g++
ROOT 	:= $(shell pwd)
SUBDIR 	:= log \
		   http \
		   scheduler \
		   CGImysql \
		   timer \
		   fiber \
		   config \
		   main \
		   obj
OBJ_DIR	:= $(ROOT)/obj
BIN_DIR	:= $(ROOT)/bin
BIN		:= server
OBJ		:= main.o webserver.o config.o log.o http.o scheduler.o sql_connection_pool.o timer.o fiber.o AsyncLogging.o CountDownLatch.o FileUtil.o LogFile.o LogStream.o

export CXX OBJ ROOT OBJ_DIR BIN_DIR BIN

all: CHECKDIR $(SUBDIR) 
		
CHECKDIR:
		mkdir -p $(SUBDIR) $(BIN_DIR)
$(SUBDIR) : ECHO
		make -C $@
ECHO:
		@echo $(SUBDIR)
		@echo begin compile
clean:
		rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)
