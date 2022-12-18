
//#define BP_FREE_STDVEC(vec) for (auto it = (vec).begin(); it != (vec).end(); ++it) {\
//delete (*it);\
//}

#define SSTREAM_PRE_RD(ss) if (!(ss)->good()) \
return false;

#define SSTREAM_RD_STRUCT(ss, len, ptr) (ss)->read((char*)(ptr), (len)); \
if (!(ss)->good() || (ss)->gcount() != (len)) \
return false;

#define SSTREAM_RD_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_RD(ss) return (ss)->good();

#define SSTREAM_PRE_WR(ss) ;

#define SSTREAM_WR_STRUCT(ss, len, ptr) (ss)->write((char*)(ptr), (len));

#define SSTREAM_WR_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_WR(ss) return (ss)->good();

bool _Helper::PeekOpCode(std::stringstream* ss, _OpCode* code) {
    SSTREAM_RD_STRUCT(ss, sizeof(uint32_t), code);
    ss->seekg(-(int32_t)(sizeof(uint32_t)), std::ios_base::cur);
    return true;
}

bool _Helper::ReadOpCode(std::stringstream* ss, _OpCode* code) {
    SSTREAM_RD_STRUCT(ss, sizeof(uint32_t), code);
    return true;
}

bool _Helper::WriteOpCode(std::stringstream* ss, _OpCode code) {
    SSTREAM_WR_STRUCT(ss, sizeof(uint32_t), &code);
    return true;
}

bool _Helper::ReadString(std::stringstream* ss, std::string* strl) {
    uint32_t length = 0;
    SSTREAM_RD_STRUCT(ss, sizeof(uint32_t), &length);
    if (length > ss->str().length()) return false;

    strl->resize(length);
    SSTREAM_RD_STRUCT(ss, length, strl->data());

    return true;
}

bool _Helper::WriteString(std::stringstream* ss, std::string* strl) {
    uint32_t length = strl->size();
    SSTREAM_WR_STRUCT(ss, sizeof(uint32_t), &length);
    SSTREAM_WR_STRUCT(ss, length, strl->c_str());

    return true;
}

