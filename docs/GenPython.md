# Python Generation

## Code Layout

### Basic Layout

```py
# snippets/header.py粘贴在这里

# OpCode生成在此
class _OpCode(enum.IntEnum):
    msg_a: int = 0
    msg_b: int = 1
    # 更多OpCode将在这里继续写入

# enum，struct，msg定义在这里

# 通用反序列化函数生成在此
def _UniformDeserialize(_ss: io.BytesIO) -> _BpMessage:
	_opcode: int = _PeekOpCode(_ss)  
	if _opcode == _OpCode.msg_a:
		_data = msg_a()
		_data.Deserialize(_ss)
		return _data
	elif _opcode == _OpCode.msg_b:
		_data = msg_b()
		_data.Deserialize(_ss)
		return _data
	# 更多msg将以elif继续进行分析
	return None

```

`snippets/header.py`定义了一些常用的序列化反序列化格式，这些格式反复被使用，因此将其独立出来定义可以减少不必要的性能开销。  
定义了Peek OpCode的读取代码。  
还定义了`_BpMessage`和`_BpStruct`作为基类。其中有`str`类型的序列化和反序列化方法。

### Alias Layout

Python不支持alias，所有alias将在编译时转换为其对应的数据类型。

### Enum Layout

```py
class enum_a(enum.IntEnum):
	Entry: int = 0
    # 有更多项则继续写
```

所有enum均继承`enum.IntEnum`。  
每一项均为`int`类型，且其数值在编译时确定并以等号指定。

### Struct Layout

```py
class struct_a(_BpStruct):
	_struct_packer: tuple[struct.Struct] = (struct.Struct('<B3xI'), struct.Struct('<I'), )
	def __init__(self):
		# Py / Ctor写在此处。无内容可够造则写pass
	def Deserialize(self, _ss: io.BytesIO):
		# Py / Deserialize写在此处。无内容可反序列化则跳过1个字节。
	def Serialize(self, _ss: io.BytesIO):
		# Py / Serialize写在此处。无内容可序列化则写入1个空字节。
```

所有struct均继承`_BpStruct`。  
每个struct拥有3个类内函数，构造函数，序列化函数，反序列化函数。  
`_struct_packer`与Python优化相关，参见相应章节。其项可以按需无限添加。

### Msg Layout

```py
class msg_a(_BpMessage):
	_struct_packer: tuple[struct.Struct] = (struct.Struct('<B3xI'), struct.Struct('<I'), )
	def __init__(self):
		# Py / Ctor写在此处。无内容可够造则写pass
	def GetIsReliable(self) -> bool:
		return True # 根据类定义改变
	def GetOpCode(self) -> int:
		return _OpCode.msg_a    # 根据类名生成到对应的_OpCode
	def Deserialize(self, _ss: io.BytesIO):
		self._ReadOpCode(_ss)
		# Py / Deserialize写在此处。无内容可反序列化则跳过1个字节。
	def Serialize(self, _ss: io.BytesIO):
		self._WriteOpCode(_ss)
		# Py / Serialize写在此处。无内容可序列化则写入1个空字节。
```

所有msg均继承`_BpMessage`。  
每个struct拥有5个类内函数，构造函数，序列化函数，反序列化函数，是否是可靠消息函数，获取对应OpCode函数。

## Python Optimization

### 合并连续的Primitive成员

为了提升Python序列化和反序列化性能，编译器允许将**多个连续的Primitive类型**合并在一起进行操作。  
例如对于`int x, y, z;`的声明。编译器将生成如下序列化与反序列化语句：

```py
# 序列化
_ss.write(struct.pack("<III", self.x, self.y, self.z))
# 反序列化
(self.x, self.y, self.z, ) = struct.unpack("<III", _ss.read(DSIZE))
```

其中`III`是针对这三个合并了的Primitive类型成员的序列化格式，DSIZE是这个合并后的数据大小。这些参数需要根据合并的Primitive类型，**以及是否有Align Padding**进行改变。这些参数在编译期间计算完成。

### Struct预编译

为了避免Python反复分析序列化格式文本，创建一个名为_struct_packer的类静态成员。其是一个tuple，tuple中每一项为预编译的struct.Struct。  
“被合并的连续Primitive成员”，和“Primitive静态数组”将会使用此特性。他们会将他们需要使用的序列化格式预先编译为struct.Struct并存储在_struct_packer中。在运行时直接引用，而非边解析格式文本边序列化。  
需要注意的是，“合并的连续Primitive成员”，和“Primitive静态数组”虽然均使用此特性，但“Primitive静态数组”和“连续的Primitive成员”绝对不可以合并在一起！它们绝不可能共享同一个struct.Struct！  
_struct_packer的定义位置如下图。其中NAME是当前的类名

```
# ...
class NAME(_BpMessage):
    _struct_packer: tuple[struct.Struct] = (struct.Struct('<III'), )
# ...
```

取用时，通过下标取用，例如对于上述的被合并的连续Primitive成员，其序列化和反序列化改写为如下代码。  
序列化与反序列化中的N则代表了取哪一个预编译struct.Struct。对于有多个绑定在一起的Primitive类型成员组，其序列化和反序列化所用的N不同。例如此序列化与反序列化中，N取0。

```py
# 序列化
_ss.write(NAME._struct_packer[N].pack(self.x, self.y, self.z))
# 反序列化
(self.x, self.y, self.z, ) = NAME._struct_packer[N].unpack(_ss.read(NAME._struct_packer[N].size))
```
