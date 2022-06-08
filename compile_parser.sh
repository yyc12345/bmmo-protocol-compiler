cd src
flex compiler.l
bison -d compiler.y -b y
echo Compile Done
