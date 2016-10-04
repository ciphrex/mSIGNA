ifdef DEBUG
    C_FLAGS += -g -fPIC
else
    C_FLAGS += -O3 -fPIC
endif

