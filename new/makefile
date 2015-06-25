
################################################################################
# include sources.mk
################################################################################

O_SRCS := 
CPP_SRCS := 
C_UPPER_SRCS := 
C_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
CXX_SRCS := 
C++_SRCS := 
CC_SRCS := 
C++_DEPS := 
OBJS := 
C_DEPS := 
CC_DEPS := 
CPP_DEPS := 
EXECUTABLES := 
CXX_DEPS := 
C_UPPER_DEPS := 

# Every subdirectory with source files must be described here
SUBDIRS := \
src \

################################################################################
# include objects.mk
################################################################################

USER_OBJS :=

LIBS := -lpthread

################################################################################
# include src/subdir.mk
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
./ClientManager.cpp \
./InterfacesManager.cpp \
./Proxy.cpp \
./RequestManager.cpp \
./StatManager.cpp 

OBJS += \
./build/ClientManager.o \
./build/InterfacesManager.o \
./build/Proxy.o \
./build/RequestManager.o \
./build/StatManager.o 

CPP_DEPS += \
./build/ClientManager.d \
./build/InterfacesManager.d \
./build/Proxy.d \
./build/RequestManager.d \
./build/StatManager.d 


# Each subdirectory must supply rules for building sources it contributes
./build/%.o: ./%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
# All Target
all: directories Proxy

directories:
	mkdir -p build

# Tool invocations
Proxy: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross G++ Linker'
	g++  -o "Proxy" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C++_DEPS)$(OBJS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) Proxy
	rm -rf build
	-@echo ' '
