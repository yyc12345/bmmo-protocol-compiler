
class _BpcMessage {
public:
    _BpcMessage() {};
    virtual ~_BpcMessage() {};

    virtual _OpCode GetOpCode() = 0;
    virtual bool IsReliable() = 0;
    virtual bool Serialize(std::stringstream* data) = 0;
    virtual bool Deserialize(std::stringstream* data) = 0;
protected:
    _OpCode _mInternalType;
    bool inOpCode(std::stringstream* data);
    bool outOpCode(std::stringstream* data);

    bool inStdstring(std::stringstream* data, std::string* strl);
    bool outStdstring(std::stringstream* data, std::string* strl);

    _OpCode peekInternalType(std::stringstream* data);
};
