make
sudo rmmod fibdrv
sudo insmod fibdrv.ko
sudo ./client 

# gcc test.c -o test
# sudo ./test > fib_sequence.txt 
# sudo ./test > fib_recursive.txt 
# sudo ./test > fib_interative.txt 
# sudo ./test > fib_interative_clz.txt 
gnuplot fib_plot.gp