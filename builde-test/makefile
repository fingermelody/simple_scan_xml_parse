objects := TagArray.o Text.o line_reader.o node.o simple_scan_parser.o multi_read.o simple_parser.o reader.o stack.o synchronize.o tag.o task_queue.o \
MPQ_hash.o query.o array.o slaves.o master.o \
tools/hash_map.o tools/linked_list.o tools/memory.o

SRCPATH := ../Simple_scan_XML_Parse
SUBDIRS := ../ ../tools
MPIPATH := /usr/local/cluster/openmpi/openmpi-1.6.3

Simple_scan_XML_Parse: $(objects)
        nvcc --verbose --cudart static -link -G -g -o  "Simple_scan_XML_Parse" $(objects) -L$(MPIPATH)/lib -lpthread -lmpi 

%.o: $(SRCPATH)/%.c
        @echo 'Building file: $<'
        nvcc --verbose --compiler-bindir mpicc -G -g -O0 -odir "" -M -o "$(@:%.o=%.d)" "$<" 
        nvcc --verbose --compiler-bindir mpicc -G -g -O0 --compile -odir ""  -x c -o  "$@" "$<"
        @echo 'Finished building: $<'
        @echo ' '

tools/%.o: $(SRCPATH)/tools/%.c
  	@echo 'Building file: $<'
        nvcc --verbose -G -g -O0 --compiler-bindir mpicc -odir "" -M -o "$(@:%.o=%.d)" "$<" 
        nvcc --verbose -G -g -O0 --compiler-bindir mpicc --compile -odir ""  -x c -o  "$@" "$<"
        @echo 'Finished building: $<'
        @echo ' '

%.o: $(SRCPATH)/%.cu
        @echo 'Building file: $<'
        nvcc --verbose -G -g -O0 --compiler-bindir mpicc -odir "" -M -o "$(@:%.o=%.d)" "$<" 
        nvcc --verbose -G -g -O0 --compiler-bindir mpicc --compile -odir ""  -x cu -o  "$@" "$<"
        @echo 'Finished building: $<'

.PHONY: clean
clean:
        -rm Simple_scan_XML_Parse *.o *.d

