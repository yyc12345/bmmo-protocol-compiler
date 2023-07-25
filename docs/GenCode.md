# Generated Code Manual

## Before Reading

Before starting your reading, please allow me introduce the aim of each generated languages.

Compiler supports 3 languages officially now, Python, C\#, C++ and Flatbuffers. Python code is generated for quick test, small demo and etc. Python code is not ready for production environment.   
Oppositely, C\# and C++ code is more suit for your productions. C\# code can be used in some game engine easily, such as Unity. C++ code can be applied to game server and etc.  
Flatbuffers code generation is a migration way. As I said in readme, if you want to migrate to more stable binary protocol, use this output can let you feel more fluent about migration.

You may notice the requirement of some laguages are extremely high. For example, we order C++17 for C++ and .Net Core 2.1 / .Net Framework 4.6 for C\#. That's because the features we needed only provided in the version higher that our requested version. If you think these version are too high, you can create a fork and change the code on your requirement freely.

## Data Type Conversion

Following table introduce the data type conversion used by Bp Compiler.

### Basic Type

|Bp		|Python	|C\#	|C++			|Flatbuffers|
|:---	|:---	|:---	|:---			|:---		|
|float	|float	|float	|float			|float		|
|double	|float	|double	|double			|double		|
|int8	|int	|sbyte	|int8_t			|byte		|
|int16	|int	|short	|int16_t		|short		|
|int32	|int	|int	|int32_t		|int		|
|int64	|int	|long	|int64_t		|long		|
|uint8	|int	|byte	|uint8_t		|ubyte		|
|uint16	|int	|ushort	|uint16_t		|ushort		|
|uint32	|int	|uint	|uint32_t		|uint		|
|uint64	|int	|ulong	|uint64_t		|ulong		|
|string	|str	|string	|std::string	|string		|

### Container

|Bp		|Python		|C\#									|C++				|Flatbuffers	|
|:---	|:---		|:---									|:---				|:---			|
|tuple	|list[T]	|T[]									|CStyleArray\<T, N\>|[T:N]			|
|list	|list[T]	|System.Collections.Generic.List\<T\>	|std::vector\<T\>	|[T]			|

Tips:

* Python do not have compulsory type system, so I use Python type hints system instead.
* Python use `list[T]` for the static and dynamic array of Bp file just due to `tuple[T]` do not support assigning value one by one.
* In Flatbuffers, `[T:N]` is called Array, and `[T]` is called Vector.
* String type need special treatment in almost languages although it is grouped as basic type. For convenience, we name the collection of all basic types without string as a new name, primitive type.
* At the opposite, we call string type and struct type as non-primitive type.
* Enum also can be seen as primitive type because its inheriting type is primitive type.

## Serialized Binary Data

This section will briefly introduce some features of Serialized Binary Data.

* Serialized Binary Data always is written in Little Endian, no matter the OS endian it is.
* Align and padding is like C compiler does.
* String is stored with following steps.
  1. Encode this string into UTF8 encoding.
  1. Get the length of this UTF8 string and store it as an `uint32`.
  1. Store the whole UTF8 string without null terminator.
* When storing tuple, no space between each members in tuple.
* List is stored with following steps.  
  1. Get the length of this list and store it as an `uint32`.
  1. Store the members of list like tuple.

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
0. Send gotten binary sequence via stream or anything you like.
0. Clear buffer like deserialization.

## C\#

### Read Message

```csharp
MemoryStream ms = new MemoryStream();
ms.Write(blabla);
ms.Seek(0, SeekOrigin.Begin);

BpMessage your_data = YourNameSpace.BPHelper.UniformDeserialize(new BinaryReader(ms, Encoding.Default, true));
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
        - Return an instance of the class inheriting `BpMessage` when successfully parsing.
        - Return `null` when parser do not understand this binary sequence.
    * This function will **not** throw any exception.
    * We need construct a `System.IO.BinaryReader` in there as its parameter.
        - Set `leaveOpen = true` to make sure that `MemoryStream` will not be closed automatically.
        - If you want to re-use this `BinaryReader`, you also can declare it in any scope as you want.
        - The `encoding` parameter is not important because we do not use its string RW functions.
0. Clear buffer for following using. It is highly recommended even if you just want to use this buffer only once.

### Write Message

```csharp
MemoryStream ms = new MemoryStream();

var your_data = new ExampleMessage();
your_data.essential = 114514;
your_data.essential_list[0] = "test";

YourNameSpace.BPHelper.UniformSerialize(your_data, new BinaryWriter(ms, Encoding.Default, true));

your_reliable_setter(your_data.IsReliable());
your_opcode_setter(your_data.GetOpCode());
your_data_sender(ms.ToArray());

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

* Bp use `nullptr` as the symbol of null pointer, not `NULL`. If you want to check null pointer for some Bp managed variables (however you might not need to check in almost situation.), please compare its pointer with `nullptr`, not `NULL`.
* Bp use naked pointer if it needed. But for the most cases, pointer is not used.

### Encoding

The principle order Bp string must be UTF8 encoding. However, our generated C++ code do not ensure this. You should create your encoding convertion code to make sure that all you stored in std::string is writtin in UTF8 encoding.

### Ownerships

All generated Bp struct and msg follows RAII. And each of them have implemented copy and move semantics.

All variables within Bp class definition are managed by Bp. You do **not** allowed to help Bp free them. All data will be free properly once you free class instance.  
So, as the result, Bp class take the ownership of all its members.

### Message Read

```cpp
std::stringstream buffer;
buffer.write(blabla);

YourNameSpace::BpMessage* your_data = YourNameSpace::BPHelper::UniformDeserialize(buffer);
if (your_data == nullptr)
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
    * Return an instance **pointer** of the class inheriting `BpMessage` when successfully parsing.
    * Return `nullptr` when parser do not understand this binary sequence.
    * This function will **not** throw any exception.
0. Clear buffer for following using. It is highly recommended even if you just want to use this buffer only once.
0. Free your gotten message in anytime you want.

### Message Write

```cpp
std::stringstream buffer;

YourNameSpace::ExampleMessage* your_data = new YourNameSpace::ExampleMessage();
your_data->essential = 114514;
your_data->essential_list[0] = "test";

if (!YourNameSpace::BPHelper::UniformSerialize(buffer, your_data))
    throw std::exception("Invalid Message.");

your_reliable_setter(your_data->IsReliable())
your_opcode_setter(your_data->GetOpCode())
your_data_sender(buffer.str(), buffer.str().size());

buffer.str("");
buffer.clear();

delete your_data;

```

0. Create a buffer like deserialization.
    - Do not forget to free it.
0. Create an instance of your data and fill all fields before doing serialization.
0. Call object's serialization function.
    * Return `true` to indicate everything is okay and data will be written in `std::stringstream`.
    * Return `false` when something went wrong. It usually caused by forgetting to fill some fields.
    * This function will **not** throw any exception.
0. Send gotten binary sequence via stream or anything you like.
0. Clear buffer like deserialization.
0. Free your initialized message in anytime you want.

## Flatbuffers

The generation of Flatbuffers just interpret the BP file's entries one by one, because Flatbuffers is served for migration.  
Some essential properies and features may be ignored during generation.  
There is a list containing what you should notice during conversion.

* All align number and padding will be wiped out.
* Natural and narrow keywords are ignored.
* Reliability is ignored.
* `struct` will be translate to `struct` and `msg` will be translated to `table`.
* Compiler do not check field type legality during conversion. For example, Flatbuffers `table` do not accept static array, but we still write it if you specify a static array in BP file's `msg`.
* No mechanism to ensure the geenerated fbs can pass compiling without any modifications.
