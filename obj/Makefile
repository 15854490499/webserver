DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2 

endif

$(BIN_DIR)/$(BIN) : $(OBJ)
		$(CXX) $^ $(CXXFLAGS) -o $@ -lpthread -lmysqlclient
