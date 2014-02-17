################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../TagArray.c \
../Text.c \
../line_reader.c \
../node.c \
../simple_state_machine.c \
../stack.c \
../synchronize.c \
../tag.c 

CU_SRCS += \
../simple_scan_parser.cu 

CU_DEPS += \
./simple_scan_parser.d 

OBJS += \
./TagArray.o \
./Text.o \
./line_reader.o \
./node.o \
./simple_scan_parser.o \
./simple_state_machine.o \
./stack.o \
./synchronize.o \
./tag.o 

C_DEPS += \
./TagArray.d \
./Text.d \
./line_reader.d \
./node.d \
./simple_state_machine.d \
./stack.d \
./synchronize.d \
./tag.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-5.5/bin/nvcc -Iphread -G -g -O0  -odir "" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-5.5/bin/nvcc -Iphread -G -g -O0 --compile  -x c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-5.5/bin/nvcc -Iphread -G -g -O0  -odir "" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-5.5/bin/nvcc --compile -G -Iphread -O0 -g  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


