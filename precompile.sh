cd src
flex -B -s compiler.l
echo "Lexer Done"
bison -d compiler.y -b y
echo "Parser Done"

cd ..
snippets_h=src/snippets.h
snippets_c=src/snippets.c

# $1 is name code
# $2 is prepared file
do_bin2c(){
	echo "extern const BPCSNP_EMBEDDED_FILE bpcsnp_${1};" >> $snippets_h
	
	echo "static const char[] _bpcsnp_file_${1} = \"" >> $snippets_c
	bin2c < ${2} >> $snippets_c
	echo "\";" >> $snippets_c
	echo "static const size_t _bpcsnp_len_${1} = sizeof(_bpcsnp_file_${1})/sizeof(char);" >> $snippets_c
	echo "const BPCSNP_EMBEDDED_FILE bpcsnp_${1} = {_bpcsnp_file_${1}, _bpcsnp_len_${1}};" >> $snippets_c
}

# set header and reset source and header
echo "#include<stdlib.h>" > $snippets_h
echo "typedef struct _BPCSNP_EMBEDDED_FILE{char* file; size_t len;｝BPCSNP_EMBEDDED_FILE;" >> $snippets_h
echo "#include \"snippets.h\"" > $snippets_c

# create snippets
do_bin2c "py_header" snippets/header.py
do_bin2c "cs_header" snippets/header.cs
do_bin2c "cs_functions" snippets/functions.cs
do_bin2c "hpp_header" snippets/header.hpp
do_bin2c "hpp_functions" snippets/functions.hpp
do_bin2c "cpp_functions" snippets/functions.cpp
do_bin2c "cpp_tail" snippets/tail.cpp

echo "Snippets Done"
