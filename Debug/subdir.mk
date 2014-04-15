################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MPQ_hash.c \
../TagArray.c \
../Text.c \
../line_reader.c \
../master.c \
../node.c \
../query.c \
../simple_state_machine.c \
../slaves.c \
../stack.c \
../synchronize.c \
../tag.c \
../task_queue.c 

CU_SRCS += \
../simple_scan_parser.cu 

CU_DEPS += \
./simple_scan_parser.d 

OBJS += \
./MPQ_hash.o \
./TagArray.o \
./Text.o \
./line_reader.o \
./master.o \
./node.o \
./query.o \
./simple_scan_parser.o \
./simple_state_machine.o \
./slaves.o \
./stack.o \
./synchronize.o \
./tag.o \
./task_queue.o 

C_DEPS += \
./MPQ_hash.d \
./TagArray.d \
./Text.d \
./line_reader.d \
./master.d \
./node.d \
./query.d \
./simple_state_machine.d \
./slaves.d \
./stack.d \
./synchronize.d \
./tag.d \
./task_queue.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-5.5/bin/nvcc -I/usr/include/mpi -I/usr/local/include -G -g -O0  -odir "" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-5.5/bin/nvcc -I/usr/include/mpi -I/usr/local/include -G -g -O0 --compile  -x c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-5.5/bin/nvcc -I/usr/include/mpi -I/usr/local/include -G -g -O0  -odir "" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-5.5/bin/nvcc --compile -G -I/usr/include/mpi -I/usr/local/include -O0 -g  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


