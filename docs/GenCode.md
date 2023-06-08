# Generated Code Manual

## Before Reading

Before starting your reading, please allow me introduce the aim of each generated languages. And, a small, arrogant and realistic warning for C++ code user.

Compiler supports 4 languages now, Python, C\#, C++ and Proto. Python code is generated for quick test, small demo and etc. Python code is not ready for production environment.   
Oppositely, C\# and C++ code is more suit for your productions. C\# code can be used in some game engine easily, such as Unity. C++ code can be applied to game server and etc.  
Proto code generation is a migration way. As I said in readme, if you want to migrate to more stable binary protocol, use this output can let you feel more fluent about migration.

For C++ programmer, please pay attention to this warning. Because C++ do not have a confirmed code style, so we use "C with classes" style in this project to make sure the max compatibility of generated code. If you feel uncomfortable and want to say, why not use C11, C17 or C20 syntax? I will say, "fuck off and shut up". I will not upgrade any C++ syntax because C++ is a shitty non-standard language. You can open fork freely with proper license if you think I am an idiot.

## Data Type Conversion

Following table introduce the data type conversion used by Bp Compiler.

### Basic Type

|Bp		|Python	|C\#	|C++			|Proto	|
|:---	|:---	|:---	|:---			|:---	|
|float	|float	|float	|float			|float	|
|double	|float	|double	|double			|double	|
|int8	|int	|sbyte	|int8_t			|int32	|
|int16	|int	|short	|int16_t		|int32	|
|int32	|int	|int	|int32_t		|int32	|
|int64	|int	|long	|int64_t		|int64	|
|uint8	|int	|byte	|uint8_t		|uint32	|
|uint16	|int	|ushort	|uint16_t		|uint32	|
|uint32	|int	|uint	|uint32_t		|uint32	|
|uint64	|int	|ulong	|uint64_t		|uint64	|
|string	|str	|string	|std::string	|string	|

## Container

|Bp		|Python		|C\#									|C++				|Proto		|
|:---	|:---		|:---									|:---				|:---		|
|tuple	|list[T]	|T[]									|T[]				|repeated	|
|list	|list[T]	|System.Collections.Generic.List\<T\>	|std::vector\<T\>	|repeated	|

Tips:

* Python do not have compulsory type system, so I use Python type hints system instead.
* Python use `list[T]` for the static and dynamic array of Bp file just due to `tuple[T]` do not support assigning value one by one.
* String type need special treatment in almost languages although it is grouped as basic type. For convenience, we name the collection of all basic types without string as a new name, primitive type.
* At the opposite, we call string type and struct type as non-primitive type.
* Enum also can be seen as primitive type because its inheriting type is primitive type.

## Performance Optimization

Considering the environment where the Bp file are used, we need do some essential performance optimization in generated code, especially serialization and deserialization functions. This section will introduce the optimization strategy of each supported generated code, used by Bp compiler.

In general, natural layout and primitive type, with and without array, can get better performance. Non-primitive type will drag the speed of reading and writing. However, in some cases, string is necessary so you don't worry about this too much.

Commonly, the optimization can be divided into 2 parts, natural message and narrow message. Because they have different layout and can not be treated together.  

Proto generation is not be considered in there because Proto generation is just a transplant process. It doesn't involve any optimizations.

### Natural Message

Natural is designed for high performance instinctively. It exactly reflect the data in memory view at all small endian x86 system.  
In python, we iterate each member recursively to construct a correct pack format string and a sequence of each members splitted by comma. Then we deliver them to `struct` module and use some tricks, such as unpack assignment and etc to achieve out target.  
However, the Marshal class provided by C\# is too weak and unsafe features still is too safe(C\# do not allow declare struct array in struct, even if the size of struct is confirmed. It is conflicted with Bp design). So all natural messages will fall back into narrow messages to process.  
In C++, we declare a struct with explicit layout. Then directly convert data.

### Narrow Message

Narrow Message use following steps to accelerate RW speed. Some language may ignore some acceleration step and fallback to normal processing. For example, Only Python will do the first acceleration.

0. Put as much as possible single primitive members together and process them just like natural message done.
0. For the tuple or list of primitive type, read them together and convert them in a single conversion.
0. Process single or array non-primitive type normally.

## Name Conflict Principle

The templates of generated code may occupy some names to define something. Obviously the generated code can not work if you name your msg or class member to these names. There is the principle instructing how we occupy these names.

All objects used internally will add underline (`_`) prefix to prevent any possible name conflict. For example, _ss, the name of intermediate stream variable. The reason why we add underline prefix is that the name started with underline is not allowed in BP file.  
All objects exported for user should keep its original name. For example, BpMessage and BpStruct, the prototype of msg and struct.

## Python

### Read Message

```python
import YourBpModule

ss = io.BytesIO()
ss.write(blabla)
ss.seek(io.SEEK_SET, 0)

your_data = YourBpModule.UniformDeserialize(ss)
if your_data is None:
    raise Exception("Invalid OpCode.")

ss.seek(io.SEEK_SET, 0)
ss.truncate(0)
```

0. Import the generated Python module first.
0. Create an `io.BytesIO` as an intermediate buffer. This buffer can be recycled as much as you want. So it also can be defined in global or class scope.
0. Write binary data into buffer from other sources, such as file or network stream.
0. Then we need reset cursor to the head of buffer for the convenience of following deserialization.
0. Call uniform deserialization function to try parsing data.
    * Return an instance of the class inheriting `BpMessage` when successfully parsing.
    * Return `None` when parser do not understand this binary sequence.
    * Throw exception when something went wrong when parsing message.
0. Clear buffer for following using. It is highly recommended even if you just want to use this buffer only once.

### Write Message

```python
import YourBpModule

ss = io.BytesIO()

your_data = YourBpModule.ExampleMessage()
your_data.essential = 114514
your_data.essential_list[0] = "test"

YourBpModule.UniformSerialize(your_data, ss)

your_reliable_setter(your_data.IsReliable())
your_opcode_setter(your_data.GetOpCode())
your_data_sender(ss.getvalue())

ss.seek(io.SEEK_SET, 0)
ss.truncate(0)
```

0. Import the generated Python module first.
0. Create a buffer like deserialization.
0. Create an instance of your data and fill all fields before doing serialization.
0. Call uniform serialization function.
    * Data will be written in passed `io.BytesIO` when everything is OK.
    * Raise exception when something went wrong. It usually caused by:
        - Forget to fill some fields.
        - Fill data with wrong type. (Python do not have compulsory type system so this error only can be found when doing parsing.)
	* Uniform serialization function will write OpCode automatically. If you don't need it, call `your_data.Serialize(ss)` directly.
0. Send gotten binary sequence via stream or anything you like.
0. Clear buffer like deserialization.

## C\#

### Read Message

```csharp
MemoryStream ms = new MemoryStream();
ms.Write(blabla);
ms.Seek(0, SeekOrigin.Begin);

_BpMessage your_data = YourNameSpace._Helper.UniformDeserialize(new BinaryReader(ms, Encoding.Default, true));
if (your_data is null)
    throw new Exception("Invalid OpCode.");

ms.SetLength(0);

```

0. Create an instance of `System.IO.MemoryStream` as an intermediate buffer. This buffer can be recycled as much as you want. 
    * You also can define it in class scope. This is the common situation.
    * If you are using it in a small scope, you also can use `using` keyword to ensure it can be released safely.
0. Write binary data into buffer from other sources, such as file or network stream.
0. Then we need reset cursor to the head of buffer for the convenience of following deserialization.
0. Call uniform deserialization function to try parsing data.
    * Function will do different reactions when facing different data.
        - Return an instance of the class inheriting `_BpMessage` when successfully parsing.
        - Return `null` when parser do not understand this binary sequence.
    * This function will **not** throw any exception.
    * We need construct a `System.IO.BinaryReader` in there as its parameter.
        - Set `leaveOpen = true` to make sure that `MemoryStream` will not be closed automatically.
        - If you want to use this `BinaryReader`, you also can declare it in any scope as you want.
        - The `encoding` parameter is not important because we do not use its string RW functions.
0. Clear buffer for following using. It is highly recommended even if you just want to use this buffer only once.

### Write Message

```csharp
MemoryStream ms = new MemoryStream();

var your_data = new ExampleMessage();
your_data.essential = 114514;
your_data.essential_list[0] = "test";

your_data.Serialize(new BinaryWriter(ms, Encoding.Default, true));
your_sender(ms.ToArray());
ms.SetLength(0);

```
0. Create a buffer like deserialization.
0. Create an instance of your data and fill all fields before doing serialization.
0. Call object's serialization function.
    * Function will do different reactions when facing different data.
        - Return `true` to indicate everything is okay and data will be written in `MemoryStream`.
        - Return `false` when something went wrong. It usually caused by forgetting to fill some fields.
        - This function will **not** throw any exception.
    * Parameter need an instance of `System.IO.BinaryWriter` like deserialization.
0. Send gotten binary sequence via stream or anything you like.
0. Clear buffer like deserialization.

## C++

### Code Styles

* Bp use `NULL` as the symbol of null pointer, not `nullptr`. If you want to check null pointer for some Bp managed variables (however you might not need to check in almost situation.), please compare its pointer with `NULL`, not `nullptr`.
* Bp use naked pointer everywhere. Please ensure your delivered pointer is naked pointer.

### Generated Class

Like other languages, Bp will create individual class definition for each message declaration. However, for some convenience reason, we will create a variable called `_InnerData` as a class instance member.  
This variable is defined as a struct and this structure contains all members defined for this message.  
So in general, `_InnerData` is just a container and class only take responsibility to serialization and memory management.  
Here is a example.

```cpp
class message_a : public _BpMessage {
public:
    struct _InnerDataDef {
        uint32_t data1;
    };
    _InnerDataDef _InnerData;
    
    // blabla
};

class message_b : public _BpMessage {
public:
    struct _InnerDataDef {
        uint32_t data1;
        message_a::_InnerDataDef data2;
    };
    _InnerDataDef _InnerData;
    
    // blabla
};
```

### Data Structures

We have talked data type conversion and container conversion in each languages at the head of this document. And, we have known all members are placed in `_InnerData`. So, it is time to discuss how data are presented in C++ because C++ need your self to manage memory. So we need to write some extra chapters to talk about this.

In general, all primitive types. with or without array, will presented normally. But, for non-primitive types, the presentation depends on its array modifier. For single and static array, non-primitive type use its normal formation but in dynamic array, non-primitive need to use its pointer formation. There is a form describe these rules.

// todo

Also, there is a example for your reference.

```cpp
class message_a : public _BpMessage {
public:
    struct _InnerDataDef {
        uint32_t data1;
    };
    _InnerDataDef _InnerData;
    
    // blabla
};

class message_b : public _BpMessage {
public:
    struct _InnerDataDef {
        uint32_t data1;
        std::string data2;
        message_a::_InnerDataDef data3;
        uint32_t[4] data4;
        std::string[4] data5;
        message_a::_InnerDataDef[4] data6;
        std::vector<uint32_t> data7;
        std::vector<std::string*> data8;
        std::vector<message_a::_InnerDataDef*> data9;
    };
    _InnerDataDef _InnerData;
    
    // blabla
};
```

### Ownerships

All variables within Bp class definition are managed by Bp. You do **not** allowed to help Bp free them. All data will be free properly once you free class instance.  
So, as the result, Bp class take the ownership of all its members.

Ownership is served for memory management. For more clear memory management, I will tell you how to get or set data within Bp.  
For primitive types, everything is simple, just assign or add them directly.  
For non-primitive types, take `std::string` for example, when you filling data from another `std::string`, you should use immediate copy (`bp = yours.c_str()`), rather than reference (`bp = yours`). Also, when you analyze some data from parsed message, please make a copy ahead of time if you need do some complex operations.  
In conclusion, I suggest you do a deep copy for all non-primitive type data which will be gotten or set in Bp class.

There is a example. The corresponding class definition has been written in previous chapter.

```cpp
message_b* bp = new message_b();
b->_InnerData.data1 = 114514u;
b->_InnerData.data2 = "static string";
b->_InnerData.data3.data1 = 0u;
b->_InnerData.data4[0] = 0u;
b->_InnerData.data5[0] = b->_InnerData.data2.c_str();
b->_InnerData.data6[0].data1 = 0u;
b->_InnerData.data7.push_back(0u);

b->_InnerData.data8.push_back(new std::string());
b->_InnerData.data8[0]->resize(b->_InnerData.data2.size());
memcpy(b->_InnerData.data8[0]->data(), b->_InnerData.data2.c_str(), b->_InnerData.data2.size());

b->_InnerData.data9.push_back(new message_a::_InnerDataDef());
b->_InnerData.data9[0]->data1 = 0u;

```

### Message Read

```cpp
std::stringstream buffer;
buffer.write(blabla);

_BpcMessage* your_data = YourNameSpace._UniformDeserialize(&buffer);
if (your_data == NULL)
    throw std::exception("Invalid OpCode.");

buffer.str("");
buffer.clear();

delete your_data;
```

0. Create an instance of `std::stringstream` as an intermediate buffer. This buffer can be recycled as much as you want. So it also can be defined in global or class scope.
    - Do not forget to free it.
0. Write binary data into buffer from other sources, such as file or network stream.
0. You do **not** need reset cursor. Because according to the implement of standard library, `std::stringstream` has separated cursors for reading and writing.
0. Call uniform deserialization function to try parsing data.
    * Return an instance **pointer** of the class inheriting `_BpMessage` when successfully parsing.
    * Return `NULL` when parser do not understand this binary sequence.
    * This function will **not** throw any exception.
0. Clear buffer for following using. It is highly recommended even if you just want to use this buffer only once.
0. Free your gotten message in anytime you want.

### Message Write

```cpp
std::stringstream buffer;

ExampleMessage* your_data = new ExampleMessage();
your_data->essential = 114514;
your_data->essential_list[0] = "test";

if (!your_data->Serialize(&buffer))
    throw std::exception("Invalid Message.");

your_sender(buffer.str(), buffer.str().size());

buffer.str("");
buffer.clear();

delete your_data;

```

0. Create a buffer like deserialization.
    - Do not forget to free it.
0. Create an instance of your data and fill all fields before doing serialization.
0. Call object's serialization function.
    * Return `true` to indicate everything is okay and data will be written in `stringstream`.
    * Return `false` when something went wrong. It usually caused by forgetting to fill some fields.
    * This function will **not** throw any exception.
0. Send gotten binary sequence via stream or anything you like.
0. Clear buffer like deserialization.
0. Free your initialized message in anytime you want.

## Proto



