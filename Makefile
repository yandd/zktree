TARGET=zktree

all: $(TARGET)
	-echo "done!"

$(TARGET): zktree.c
	gcc -O2 zktree.c -lzookeeper_mt -lpthread -lm -o zktree

clean:
	rm -rf zktree
