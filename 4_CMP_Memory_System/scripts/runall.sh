
######################################################################################
# This scripts runs all three traces
# You will need to uncomment the configurations that you want to run
# the results are stored in the ../results/ folder 
######################################################################################

########## ---------------  ABC ---------------- ################

echo "Running Part A"

../src/sim -mode 1 ../traces/bzip2.mtr.gz > ../results/A.bzip2.res
../src/sim -mode 1 ../traces/lbm.mtr.gz  > ../results/A.lbm.res 
../src/sim -mode 1 ../traces/libq.mtr.gz   > ../results/A.libq.res 

echo "Running Part B"

../src/sim -mode 2 -L2sizeKB 1024 ../traces/bzip2.mtr.gz  > ../results/B.S1MB.bzip2.res 
../src/sim -mode 2 -L2sizeKB 1024 ../traces/lbm.mtr.gz    > ../results/B.S1MB.lbm.res
../src/sim -mode 2 -L2sizeKB 1024 ../traces/libq.mtr.gz    > ../results/B.S1MB.libq.res 

echo "Running Part C"

../src/sim -mode 3 -L2sizeKB 1024 ../traces/bzip2.mtr.gz  > ../results/C.S1MB.OP.bzip2.res -dram_policy 0
../src/sim -mode 3 -L2sizeKB 1024 ../traces/lbm.mtr.gz    > ../results/C.S1MB.OP.lbm.res -dram_policy 0
../src/sim -mode 3 -L2sizeKB 1024 ../traces/libq.mtr.gz    > ../results/C.S1MB.OP.libq.res -dram_policy 0

../src/sim -mode 3 -L2sizeKB 1024 ../traces/bzip2.mtr.gz  > ../results/C.S1MB.CP.bzip2.res -dram_policy 1
../src/sim -mode 3 -L2sizeKB 1024 ../traces/lbm.mtr.gz    > ../results/C.S1MB.CP.lbm.res -dram_policy 1
../src/sim -mode 3 -L2sizeKB 1024 ../traces/libq.mtr.gz    > ../results/C.S1MB.CP.libq.res -dram_policy 1

########## ---------------  D ---------------- ################

echo "Running Part D"

../src/sim -mode 4 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz > ../results/D.mix1.res
../src/sim -mode 4 ../traces/bzip2.mtr.gz ../traces/lbm.mtr.gz  > ../results/D.mix2.res
../src/sim -mode 4 ../traces/lbm.mtr.gz ../traces/libq.mtr.gz   > ../results/D.mix3.res

########## ---------------  E (Same as D, except L2repl) -------------- ################

# echo "Running Part E"

# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 4 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz  > ../results/E.Q1.mix1.res
# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 8 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz  > ../results/E.Q2.mix1.res
# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 12 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz > ../results/E.Q3.mix1.res

# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 4 ../traces/bzip2.mtr.gz ../traces/lbm.mtr.gz  > ../results/E.Q1.mix2.res
# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 8 ../traces/bzip2.mtr.gz ../traces/lbm.mtr.gz  > ../results/E.Q2.mix2.res
# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 12 ../traces/bzip2.mtr.gz ../traces/lbm.mtr.gz > ../results/E.Q3.mix2.res

# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 4 ../traces/lbm.mtr.gz ../traces/libq.mtr.gz  > ../results/E.Q1.mix3.res
# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 8 ../traces/lbm.mtr.gz ../traces/libq.mtr.gz  > ../results/E.Q2.mix3.res
# ../src/sim -mode 4 -L2repl 2 -SWP_core0ways 12 ../traces/lbm.mtr.gz ../traces/libq.mtr.gz > ../results/E.Q3.mix3.res

########## ---------------  F ---------------- ################

# echo "Running Part F"

# ../src/sim -mode 4 -L2repl 3 ../traces/bzip2.mtr.gz ../traces/libq.mtr.gz  > ../results/F.mix1.res
# ../src/sim -mode 4 -L2repl 3 ../traces/bzip2.mtr.gz ../traces/lbm.mtr.gz  > ../results/F.mix2.res
# ../src/sim -mode 4 -L2repl 3 ../traces/lbm.mtr.gz ../traces/libq.mtr.gz  > ../results/F.mix3.res

########## ---------------  GenReport ---------------- ################

grep IPC ../results/*.res > report.txt
grep MISS_PERC ../results/*.res >> report.txt
grep DELAY_AVG ../results/*.res >> report.txt

echo "All Done. Check the .res file in ../results directory and report.txt in current directory";

