# Bp File Manual

## Layout

Bp file consists of 2 parts. Config head and protocol body. There is a example. Each parts will be introduced in following chapter.

```
bpc 1;
namespace my.protocol

alias uuid uint64
enum ball_type : uint32 {
    wood,
    stone,
    paper
}
nature struct pos {
    float x, y, z;
}
narrow unreliable ball_state {
    string player_name;
    uuid player_id;
    uint8[8] map_hash;
    pos ball_pos;
}
```

## Annotations

Bp file supports 2 styles of annotation, line annotation and block annotations. They are very similar than C language annotation. There is a example.

```
// I'm a line annotation.

/*
Hey.
I'm a block annotation.
*/
```
## Control Head

### Version

Example: `bpc 1;`

Specific bmmo_protocol_compiler version.   
Mismatched version will abort the process of parsing. For current compiler, version is 1.  
This syntax must be place at the head of bp file.

### Namespace

Example: `namespace aaa.bbb.blablabla`

Namespace is a common feature for the most of programming language. Bp file also support namespace feature to ensure the generated code will not be post in global scope and prevent any name pollution at the same time.  
Namespace is a essential syntax and should be written after Version syntax immediately.  
Namespace only works in generated C#, C++ and Proto code. It will not applied to Python code because Python do not have namespace feature. Python use its unique module feature which is related to generated code file name.

## Protocol Body

There are 4 types declaration can be written in Protocol Body section, Alias, Enum, Struct and Msg. There are no order of them. But you should promise that declare any tokens before referring them, just like C language.

### Basic Types

Before introducing the declaration syntax, let we know the basic types supported by bp file first.  
Bp file supports following basic types:

* float
* double
* int8
* int16
* int32
* int64
* uint8
* uint16
* uint32
* uint64
* string

### Naming Requirements

For each alias, enum, struct and msg declaration, they have a unique name. We call it **identifier**. Also, for every enum members' name, and every struct and msg's fields' name. We call it **entry**. This section will introduce the constraints of identifier and entry.

Like C language, we require:

* Identifier should starts with `a` to `z` and `A` to `Z`. `_` is **not** allowed in first character.
* The rest of identifier can be `0` to `9`, `a` to `z`, `A` to `Z` and `_`.

Also, the name shouldn't be the keywords used by bp file. In the same scope, the name shouldn't duplicated.

### Alias

```
//Syntax:
alias ALIAS_NAME BASIC_NAME;

//Examples:
alias my_type uint32
alias simple_string string
```

#### Declaration

You can use this syntax give an alias for any **basic types**.  
`ALIAS_NAME` is your preferred identifier following naming requirements, we do not emphasis it again.  
`BASIC_TYPE` is one of basix types string introduced previously. we do not emphasis it again in following content.

### Enum

```
//Syntax:
enum BASIC_TYPE ENUM_NAME {
    ENTRY_NAME1,
    ENTRY_NAME2 = 1
    ...
}

//Example:
enum uint8 weather {
    Sunday,
    Cloudy,
    Raining,
    Unknow = 255
}
```

#### Declaration

The declaration of enum in bp file is very similar than C++ enum syntax. `ENUM_NAME` is your preferred identifier. `BASIC_TYPE` indicates the underlying type of this enum and decides the size of this enum.  
Write your enum entries in bracket with comma to split them. You also can specific value for some enum entry just like C language. It will perform the same behavior with C language.

#### Basic Types Limit

Please note that `BASIC_TYPE` used in Enum only can be:

* int8
* int16
* int32
* int64
* uint8
* uint16
* uint32
* uint64

Following basic couldn't be used as enum type:

* float
* double
* string

#### Interaction with Alias

Alias syntax can not give any alias for enum type, even if enum is inherit from the basic types.

### Struct

```
//Syntax:
(narrow | nature) struct STRUCT_NAME {
    fields list...
}

//Field Syntax:
(BASIC_TYPE | IDENTIFIER)([] | [TUPLE_LEN) FIELD_NAME1(, FILED_NAME2, ...) (#ALIGN);

//Example:
nature struct vx_pos {
    float x,y,z;
}
nature struct vx_quat {
    float x,y,z,w;
}
struct ball_state {
    string player_name;
    uint32 type;
    vx_pos position;
    vx_quat rotation;
    uint8[8] map_hash;
}
narrow palyers {
    uint8 uid #3;
    ball_state[] states;
}
```

#### Declaration

`STRUCT_NAME` is your preferred identifier. `(narrow | nature)` order you choose a keyword from 2 field layouts which will be introduced in next chapter. And just like C language struct syntax, we need fill our fields declarations in bracket. However, the field declaration body can be empty.

For each field declaration, `FIELD_NAME` is your preferred entry and `1(, FILED_NAME2, ...)` mean that we can create more than 1 fields in one field declaration expression. And all of them will have the same data type. For example: `float x, y, z;`.  
Switch `(BASIC_TYPE | IDENTIFIER)` order you to make a choice between basic types and custom types. Basic types can be specified directly, however, for the custom types, just like I said previously, if you use a custom types, such as alias, enum and even the struct, you should declare it before using it.  
`([] | [TUPLE_LEN)` is an optional syntax which can make field become a array field. `(#ALIGN)` also is an optional syntax which will pad whitespace for current field. All of these will be introduced in following chapters.

#### Field Layout

If you have experience with C++ language, you may know about align feature. Compiler will pad some bytes after some fields to make the whole struct can be visited fluently.

Bp file support this feature, we provide 2 field layout. `nature` is nature align layout, if you choose this layout, bp compiler will do the same things as C compiler. `narrow` is a narrow layout. It should be called *no align* actually, because it will place field in order without any useless bytes.  

A struct or msg which can use `nature` should fulfill follow requirements:

* All fields are not `string`.
* All fields do not have list modifier.
* All structs referred by fields are `nature` struct.

If you do not specify layout, compiler will check struct and msg and choose a proper one.

If you have specify a struct or msg as nature layout, any align syntax in fields declarations will be omitted and throw a warning.

#### Array Field

Bp file support 2 types array, static array and dynamic array.  
Static array, we call it **tuple**, have a unchangeable length which has been determined in programming. You can use `[NUM]` to mark a field as a tuple. `NUM` is a non-zero positive number.  
Dynamic array, we call it **list**. Its length can be resized to fulfill items storage. You can use `[]` to mark a field as a list.

What you should know is that array modifier should be placed after data type identifier, not the name of entry. It is different with C language. It is more like C#.

#### Field Align

We have talked about field layout before. Field Align is another method to align your field. Align syntax allow you align fields on your hand, not compiler automatic determination.
You can add `#NUM` to the tail of any fields declarations to notice compiler add specific padding to your chosen fields. `NUM` is a non-zero positive number indicating the count of padding bytes.

If you use align syntax in a series of declaration, such as `float x, y, z #4`. It mean that every fields should be padding with 4 bytes blank.

### Msg

```
//Syntax:
(narrow | nature) (reliable | unreliable) msg MSG_NAME {
    fields list...
}

//Example:
narrow reliable msg player_kicked_msg {
    string kicked_player_name;
    string executor_name;
    string reason;
    uint8 crashed;
}
```

Msg is the final interface struct used in your final transmission protocol. Alias, Enum and Struct syntaxes are served for Msg syntax.  
Msg is very very similar than Struct syntax. `MSG_NAME` is your preferred identifier. We do not talk msg too much due to the similarity with struct. We only talk its differences in there.

#### Differences with Struct

* Because Msg is the key of protocol, Bp compiler will distribute an unique index for each Msg declarations from top to bottom. This index can be used in distinguishing serialized binary Msg.
* Msg syntax have an extra modifier called reliability which you can see it before keyword `msg`.

For how to get index and reliability of Msg, please view [Generated Code Manual](GenCode.md).

#### Reliability

For modern game protocol, some data is essential which synchronize the context of each clients. For example, player join and quit message. However, some data is not essential, for example, player movement.  
Also, some network libraries also provide reliability feature, such as [ValveSoftware/GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) and [networkprotocol/yojimbo](https://github.com/networkprotocol/yojimbo).  
So, bp file provide this feature to indicate this message's reliability.

## Examples

Now you have a basic understanding of bp file. However, you may still confused about how to write a perfect bp file. Don't worry about that. This project provide some good bp file examples in folder `examples`.

* `example.bp`: This file describe the whole format of BP file and it also can be accepted by compiler and output correct code file.
* `bmmo.bp`: This file is the description of BMMO protocol. You can gain some complex and productive techniques from this file.
* `errors.bp`: The error test for compilr, you should not read it if you are not the developer of this compiler. However you may find some common syntax error in this file and learn from it.
