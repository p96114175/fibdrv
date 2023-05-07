set title "Fibonacci(n)"
set xlabel "n"
set ylabel "time(ns)"
set terminal png font " Times_New_Roman,6 "
set output "fibdrv.png"

plot \
"fib_sequence.txt" using 1:2 with linespoints linewidth 1 title "fib sequence", \
"fib_recursive.txt" using 1:2 with linespoints linewidth 1 title "fib sequence fast doubling recursive", \
"fib_interative.txt" using 1:2 with linespoints linewidth 1 title "fib sequence fast doubling iterative", \
"fib_interative_clz.txt" using 1:2 with linespoints linewidth 1 title "fib sequence fast doubling iterative clz", \