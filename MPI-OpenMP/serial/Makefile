
main: main.c lib/libbmp/libbmp.c lib/bmp.c
	gcc -o $@ $^ -lm

run: main
	./main

clean:
	@rm main sobel.bmp boxblur.bmp

.PHONY: clean