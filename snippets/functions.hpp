
class _BpStruct {
public:
    _BpStruct() {};
    virtual ~_BpStruct() {};

    virtual bool Serialize(std::stringstream* data) = 0;
    virtual bool Deserialize(std::stringstream* data) = 0;
    
    bool _PeekOpCode(std::stringstream* ss, _OpCode* code);
    bool _ReadOpCode(std::stringstream* ss, _OpCode* code);
    bool _WriteOpCode(std::stringstream* ss, _OpCode code);

    bool _ReadString(std::stringstream* ss, std::string* strl);
    bool _WriteString(std::stringstream* ss, std::string* strl);
};

class _BpMessage : public _BpStruct {
public:
    _BpMessage() {};
    virtual ~_BpMessage() {};

    virtual _OpCode GetOpCode() = 0;
    virtual bool IsReliable() = 0;
};

class _Helper {
public:
    static _BpMessage* UniformDeserializer(std::stringstream* ss);
};
