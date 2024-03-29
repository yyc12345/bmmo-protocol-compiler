# Testbench Readme

This testbench is served for checking whether the generated code can work.  
Before using this testbench, you should execute `./bmmo_protocol_compiler -i examples/codegen.bp -p CodeGenTest.py -c CodeGenTest.cs -d CodeGenTest.hpp -s CodeGenTest.cpp -b CodeGenTest.fbs` first to get all necessary files.

Next step, put `testbench.py` with `CodeGenTest.py`. Then executing `python3 testbench.py` (I develop it under Python 3.9 and I use a frequently changed module `typing`. Please choose a proper python version.).  
At first running, script will create a folder called `TestbenchData`. And benchmark the performance of generated ython code.

At the same time, this script also will write some binary files in `TestbenchData/py`. You need to try reading these binary files in other languages and order other languages output their binary files in the corresponding folder in `TestbenchData`.  
Then, execute `python3 testbench.py -b` again to check the correction of binary files output by other languages.

Technical infos:

* `g_ForceBigEndian` in testbench code mean that forcely treat data as big endian. Because the principal said that the data always should be little endian. It **doesn't** mean that forcely specifying the endian of OS.
* C\# need 2 different tests.
  - .Net Framework 4.6 Test
  - .Net Core 2.1 Test
* Flatbuffers is not included in test.

