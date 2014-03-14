################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tools/hash_map.c \
../tools/linked_list.c \
../tools/memory.c 

OBJS += \
./tools/hash_map.o \
./tools/linked_list.o \
./tools/memory.o 

C_DEPS += \
./tools/hash_map.d \
./tools/linked_list.d \
./tools/memory.d 


# Each subdirectory must supply rules for building sources it contributes
tools/%.o: ../tools/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-5.5/bin/nvcc -I/usr/include/mpi -I/usr/local/include -G -g -O0  -odir "tools" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-5.5/bin/nvcc -I/usr/include/mpi -I/usr/local/include -G -g -O0 --compile  -x c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


