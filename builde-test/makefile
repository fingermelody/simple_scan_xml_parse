objects := TagArray.o Text.o line_reader.o node.o simple_scan_parser.o simple_state_machine.o stack.o synchronize.o tag.o \
MPQ_hash.o \
tools/hash_map.o tools/linked_list.o tools/memory.o

SUBDIRS := ../ ../tools

Simple_scan_XML_Parse: $(objects) 
	nvcc --cudart static -link -o  "Simple_scan_XML_Parse" $(objects) -lpthread -lmpi 

%.o: ../%.c
	@echo 'Building file: $<'
	nvcc -G -g -O0 -I/usr/include/mpi -odir "" -M -o "$(@:%.o=%.d)" "$<" 
	nvcc -G -g -O0 -I/usr/include/mpi --compile -odir ""  -x c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

tools/%.o: ../tools/%.c
	@echo 'Building file: $<'
	nvcc -G -g -O0 -I/usr/include/mpi -odir "" -M -o "$(@:%.o=%.d)" "$<" 
	nvcc -G -g -O0 -I/usr/include/mpi --compile -odir ""  -x c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cu
	@echo 'Building file: $<'
	nvcc -G -g -O0 -I/usr/include/mpi -odir "" -M -o "$(@:%.o=%.d)" "$<" 
	nvcc -G -g -O0 -I/usr/include/mpi --compile -odir ""  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



.PHONY: clean
clean:
	-rm Simple_scan_XML_Parse *.o *.d

