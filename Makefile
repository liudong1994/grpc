CC=gcc
CXX=g++
INC_PATH= .

O_FLAG = -O0
CFLAGS    += ${O_FLAG} -Wno-deprecated -Wall -std=c++11
LDFLAGS   =  -L/usr/lib -L/usr/local/lib -lgrpc++_reflection -lgrpc++ -lprotobuf -lgrpc -lz -lcares 
LDFLAGS   += -laddress_sorting -lre2 -labsl_hash -labsl_city -labsl_wyhash -labsl_raw_hash_set -labsl_hashtablez_sampler -labsl_exponential_biased -labsl_statusor -labsl_bad_variant_access 
LDFLAGS   += -lgpr -lupb -ldl -lrt -lm -labsl_status -labsl_cord -labsl_str_format_internal -labsl_synchronization -labsl_stacktrace -labsl_symbolize -labsl_debugging_internal -labsl_demangle_internal
LDFLAGS   += -labsl_graphcycles_internal -labsl_malloc_internal -labsl_time -labsl_strings -labsl_throw_delegate -labsl_strings_internal -labsl_base -labsl_spinlock_wait -labsl_int128 -labsl_civil_time 
LDFLAGS   += -labsl_time_zone -labsl_bad_optional_access -labsl_raw_logging_internal -labsl_log_severity -lssl -lcrypto -lpthread
CFLAGS += -I$(INC_PATH) $(INCLUDE) -g

# 输出文件名
TARGET= ./bin/Test
OUTPUT_PATH = ./obj


#设置VPATH 包含源码的子目录列表
#添加源文件
SUBINC = tfserving .

#添加头文件
SUBDIR = tfserving .

#设置VPATH
INCLUDE = $(foreach n, $(SUBINC), -I$(INC_PATH)/$(n)) 
SPACE =  
VPATH = $(subst $(SPACE),, $(strip $(foreach n,$(SUBDIR), $(INC_PATH)/$(n)))) $(OUTPUT_PATH)

C_SOURCES = $(notdir $(foreach n, $(SUBDIR), $(wildcard $(INC_PATH)/$(n)/*.c)))
CPP_SOURCES = $(notdir $(foreach n, $(SUBDIR), $(wildcard $(INC_PATH)/$(n)/*.cpp)))
CC_SOURCES = $(notdir $(foreach n, $(SUBDIR), $(wildcard $(INC_PATH)/$(n)/*.cc)))

C_OBJECTS = $(patsubst  %.c,  $(OUTPUT_PATH)/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst  %.cpp,  $(OUTPUT_PATH)/%.o, $(CPP_SOURCES))
CC_OBJECTS = $(patsubst  %.cc,  $(OUTPUT_PATH)/%.o, $(CC_SOURCES))

CXX_SOURCES = $(CPP_SOURCES) $(C_SOURCES) $(CC_SOURCES)
CXX_OBJECTS = $(CC_OBJECTS) $(CPP_OBJECTS) $(C_OBJECTS)


$(TARGET):$(CXX_OBJECTS)
	$(CXX) -o $@ $(foreach n, $(CXX_OBJECTS), $(n)) $(foreach n, $(OBJS), $(n))  $(LDFLAGS) 
	#******************************************************************************#
	#                               Build successful !                             #
	#******************************************************************************#
	
$(OUTPUT_PATH)/%.o:%.cc
	$(CXX) $< -c $(CFLAGS) -o $@

$(OUTPUT_PATH)/%.o:%.cpp
	$(CXX) $< -c $(CFLAGS) -o $@
	
$(OUTPUT_PATH)/%.o:%.c
	$(CC) $< -c $(CFLAGS) -o $@

mkdir:
	mkdir -p $(dir $(TARGET))
	mkdir -p $(OUTPUT_PATH)
	
rmdir:
	rm -rf $(dir $(TARGET))
	rm -rf $(OUTPUT_PATH)

clean:
	rm -f $(OUTPUT_PATH)/*
	rm -rf $(TARGET)

