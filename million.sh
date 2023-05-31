make
sudo rmmod fibdrv
sudo insmod fibdrv.ko
# sudo ./client 


# Compile
gcc million.c -o million

# Analyze
# perf stat -e cache-misses,cache-references,instructions,cycles ./million
sudo perf record -g --call-graph dwarf ./million
# sudo perf report --stdio -g graph,0.5,caller
perf report -f

# sudo ./thread > fib_without_mutex.txt
# gnuplot fib_plot.gp
