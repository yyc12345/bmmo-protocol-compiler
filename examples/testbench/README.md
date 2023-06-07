# Testbench Readme

This testbench is served for whether testing generated code can work.  
Before using this testbench, you should executing `./bmmo_protocol_compiler -i examples/codegen.bp -p CodeGenTest.py -c CodeGenTest.cs -d CodeGenTest.hpp -s CodeGenTest.cpp -t CodeGenTest.pb` first to get all necessary files.

Next step, put `testbench.py` with `CodeGenTest.py`. Then executing `python3 testbench.py` (I develop it under Python 3.9 and I use a frequently changed module `typing`. Please choose a proper python version.).  
Script will create a folder called `TestbenchData`. And test Python code first.

Then, the script will write some binary file in `TestbenchData/py`. Try reading them in other languages and order other languages output their binary file in the corresponding folder in `TestbenchData`.  
Then, executing `testbench.py` again to check the correction of binary files output by other languages.
