make:
	g++ -O3 -o run main.cpp parallel_pipe.cpp sequential_trad.cpp sequential_2.cpp -L/usr/local/lib -lpapi -pthread

clean:
	rm -f run
