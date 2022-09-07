
/*
If the include syntax show error and notice you that it couldn't find correct header file, 
please add your generaed header into the system include path of your project.
*/

/*
READ THIS FIRST!
READ THIS FIRST!
READ THIS FIRST!
C++ Code Style Warning:


BPC use NULL as the symbol of null pointer, not nullptr. 
If you want to check null pointer for BPC variables(however you might not need to check in almost situation.), 
please compare its pointer with NULL, not nullptr.
BPC use naked pointer as the internal storage of std::vector and std::array, please ensure your delivered pointer is naked pointer.


BPC will convert some list structs into following C++ types:
* tuple -> std::vector
* array -> std::array


The fields of each generated classes are not pointer, however, the item of std::vector and std::array is naked pointer and use NULL to indicate null pointer.
If you push some data into the std::vector and std::array, such as number, or std::string. It mean that you have take your data's ownership to BPC. BPC will manage your data automatically, including allocation and free. 
For example, if you have some volatile std::string data, you need freeze it via creating a new std::string before inserting it into BPC managed std::vector and std::array.
Or, if you want to use some std:string located in BPC managed std::vector or std::array, you need to create a new std::string and make a copy of it, otherwise, this string pointer will become invalid after this data has been free.
There are some example about correct operations, take these messages for example:
```
struct struct_test {
    float x, y, z;
}
msg msg_test : reliable {
    int uuid;
    struct_test closed_pos;
    string username;
    
    int random tuple;
    string nickname tuple;
    struct_test possible_pos array 6;
}
```
msg_test* data = new msg_test();
// number can be specific normally
int num = 114514;
data->uuid = num;
data->random.push_back(num);

// non-list string has been initialized, specific data directly
std::string others;
others = "test";
// simply copy
data->username = others.c_str();
// seriously copy
data->username.resize(others.size());
memcpy(data->username.data(), others.c_str(), others.size());
// do not copy string like this, this is dangerous copy for volatile std::string
// data->username = others;

// non-list struct has been initialized, specific data directly
data->closed_pos.x = 0.0f;

// string or struct list insert, we need made a copy first
// then insert it and move ownership to data self.
std::string* str_data = new std::string();
(*str_data) = others.c_str();
data->nickname.push_back(str_data);

// string or struct array assign
// in initial, each item is NULL
data->possible_pos[0] = new struct_test();
data->possible_pos->x = 0.0f;
if (data->possible_pos[0] == NULL)
    throw std::excpetion("Impossible.");

// delete your data when you do not need it
delete data;

*/

/*
Message Read Example:
// Create buffer, this buffer can be recyclable use. 
// So you can define it in some global scopes, such as class member.
std::stringstream buffer;
// Use your method to fill buffer with some binary data delivered by your stream, such as file or network stream
buffer.write(blabla);
// Call uniformed deserializer to generate data struct
// If this function return NULL, it mean that parser can not understand this binary data
// This method also can throw exceptions or cause C++ errors, usually caused by broken binary data.
// You do not need to set stringstream any more, deserializer function will treate it properly.
_BpcMessage* your_data = YourNameSpace._UniformDeserialize(&buffer);
if (your_data == NULL)
	throw std::exception("Invalid OpCode.");
// Clear buffer for future using. This operation is essential, 
// especially you are recyclable using this buffer.
buffer.str("");
buffer.clear();
// Free your message in proper time
delete your_data;

*/

/*
Message Write Example:
// Create buffer, this buffer can be recyclable use. 
// So you can define it in some global scopes, such as class member.
std::stringstream buffer;
// Prepare your data struct, fill each fields, make sure all of theme are not None.
// The array and list located in your struct has been initialized. You do not need to init them again. Just fill the data.
ExampleMessage* your_data = new ExampleMessage();
your_data->essential = 114514;
your_data->essential_list[0] = "test";
// Call your data's serialize() function to get binary data.
// This method also can throw some exceptions, usually caused by that you have not fill all essential fileds.
your_data->Serialize(&buffer);
// Get bianry data from buffer and send it via some stream which you want.
// Or, you also can use MemoryStream directly to send data, to prevent unnecessary memory use caused by ToArray().
your_sender(buffer.str(), buffer.str().size());
// Clear buffer for future using. This operation is essential, 
// especially you are recyclable using this buffer.
buffer.str("");
buffer.clear();
// Free your message in proper time
delete your_data;

*/

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

uint32_t _BpcMessage::peekInternalType(std::stringstream* data) {
    uint32_t code = 0;
    data->read((char*)&code, sizeof(uint32_t));
    data->seekg(-(int32_t)(sizeof(uint32_t)), std::ios_base::cur);
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

