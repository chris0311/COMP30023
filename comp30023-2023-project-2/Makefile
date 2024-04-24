CC=cc
RPC_SYSTEM=rpc.o

.PHONY: format all

all: $(RPC_SYSTEM)

$(RPC_SYSTEM): rpc.c rpc.h rpc_handler_list.o
	$(CC) -c -o $@ rpc.c rpc_handler_list.o

# RPC_SYSTEM_A=rpc.a
# $(RPC_SYSTEM_A): rpc.o
# 	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

%.o: %.c %.h
	$(CC) -c -o $@ $<

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -rf *.o *.a