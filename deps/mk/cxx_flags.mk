INITIAL_CXX_FLAGS += -Wall

ifndef GLOBALVISIBILITY
    INITIAL_CXX_FLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
endif

ifdef DEBUG
    INITIAL_CXX_FLAGS += -g
else
    INITIAL_CXX_FLAGS += -O3
endif

CXX_FLAGS := $(INITIAL_CXX_FLAGS) $(CXX_FLAGS)

