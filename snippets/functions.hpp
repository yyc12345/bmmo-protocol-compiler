
class _BpStruct {
public:
    _BpStruct() {};
    virtual ~_BpStruct() {};

    virtual bool Serialize(std::stringstream* data) = 0;
    virtual bool Deserialize(std::stringstream* data) = 0;
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
        
    static bool PeekOpCode(std::stringstream* ss, _OpCode* code);
    static bool ReadOpCode(std::stringstream* ss, _OpCode* code);
    static bool WriteOpCode(std::stringstream* ss, _OpCode code);

    static bool ReadString(std::stringstream* ss, std::string* strl);
    static bool WriteString(std::stringstream* ss, std::string* strl);
};
