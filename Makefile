compile:
	g++ src/main.cpp src/RegExToDFAConverter.cpp -Iinclude -o main
run: compile
	./main
clean:
	if [ -f main ]; then \
		rm main; \
	fi;