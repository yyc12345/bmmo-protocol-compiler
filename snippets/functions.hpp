
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
