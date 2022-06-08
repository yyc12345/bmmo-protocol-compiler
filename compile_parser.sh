cd src
flex -B -s compiler.l
bison -d compiler.y -b y
echo Compile Done
