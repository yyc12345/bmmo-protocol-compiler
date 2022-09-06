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

class _BpcMessage {
public:
    _BpcMessage() {};
    virtual ~_BpcMessage() {};

    virtual bool Serialize(std::stringstream* data) = 0;
    virtual bool Deserialize(std::stringstream* data) = 0;
protected:
    uint32_t mInternalType;
    bool inOpCode(std::stringstream* data);
    bool outOpCode(std::stringstream* data);

    bool inStdstring(std::stringstream* data, std::string* strl);
    bool outStdstring(std::stringstream* data, std::string* strl);

    uint32_t peekInternalType(std::stringstream* data);
};
