echo "0" > position.txt
rm -f poem.txt
touch poem.txt
mpirun -n $1 ./trabalho
