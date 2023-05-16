make
sudo rmmod fibdrv
sudo insmod fibdrv.ko
# sudo ./client 

gcc thread.c -pthread -o thread
# sudo ./thread > fib_mutex.txt
sudo ./thread > fib_without_mutex.txt
# gnuplot fib_plot.gp
