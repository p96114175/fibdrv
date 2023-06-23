# make clean
make
sudo rmmod fibdrv
sudo insmod fibdrv.ko
# sudo ./client 

# gcc fibdrv_list_head.c -o a.out
# Compile

gcc -g million.c -o million
# sudo ./million

# Analyze performance
perf stat -e cache-misses,cache-references,instructions,cycles ./million
# sudo ./million
# Analyze memory 
# valgrind --tool=memcheck ./million

# Analyze memory leak
# valgrind -q --leak-check=full ./million

# sudo perf record -g --call-graph dwarf ./million
# sudo perf report --stdio -g graph,0.5,caller
# perf report -f

# sudo ./thread > fib_without_mutex.txt
# gnuplot fib_plot.gp
