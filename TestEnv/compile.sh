echo "compiling basic.vobj"
../build/VCPP/VCPP -S ./case6/algorithm.vcpp -S ./case6/array.vcpp -S ./case6/basic.vcpp -S ./case6/console.vcpp -S ./case6/math.vcpp -S ./case6/formatstr.vcpp -t lib -o ./basic.vobj
echo "compiling main.vobj"
../build/VCPP/VCPP -S ./case6/main.vcpp -S ./case6/test2.vcpp -R basic.vobj -t exec -o main.vobj