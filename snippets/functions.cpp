


#define SSTREAM_PRE_RD(ss) if (!(ss)->good()) \
return false;

#define SSTREAM_RD_STRUCT(ss, struct_type, variable) (ss)->read((char*)(&(variable)), sizeof(struct_type)); \
if (!(ss)->good() || (ss)->gcount() != sizeof(struct_type)) \
return false;

#define SSTREAM_RD_STRING(ss, strl) if (!inStdstring(ss, &(strl))) \
return false;

#define SSTREAM_RD_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_RD(ss) return (ss)->good();

#define SSTREAM_PRE_WR(ss) ;

#define SSTREAM_WR_STRUCT(ss, struct_type, variable) (ss)->write((char*)(&(variable)), sizeof(struct_type));

#define SSTREAM_WR_STRING(ss, strl) if (!outStdstring(ss, &(strl))) \
return false;

#define SSTREAM_WR_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_WR(ss) return (ss)->good();

_OpCode _BpcMessage::peekInternalType(std::stringstream* data) {
    _OpCode code = 0;
    data->read((char*)&code, sizeof(_OpCode));
    data->seekg(-(int32_t)(sizeof(_OpCode)), std::ios_base::cur);
    return code;
}

bool _BpcMessage::inOpCode(std::stringstream* data) {
    uint32_t c = 0;
    SSTREAM_RD_STRUCT(data, uint32_t, c);
    return c == mInternalType;
}

bool _BpcMessage::outOpCode(std::stringstream* data) {
    SSTREAM_WR_STRUCT(data, uint32_t, mInternalType);
    return true;
}

bool _BpcMessage::inStdstring(std::stringstream* data, std::string* strl) {
    uint32_t length = 0;
    SSTREAM_RD_STRUCT(data, uint32_t, length);
    if (length > data->str().length()) return false;

    strl->resize(length);
    data->read(strl->data(), length);
    if (!data->good() || data->gcount() != length) \
        return false;

    return true;
}

bool _BpcMessage::outStdstring(std::stringstream* data, std::string* strl) {
    uint32_t length = strl->size();
    SSTREAM_WR_STRUCT(data, uint32_t, length);
    data->write(strl->c_str(), length);

    return true;
}

