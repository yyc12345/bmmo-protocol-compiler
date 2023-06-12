using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using BP = TestNamespace.EmptyName1.EmptyName2;

namespace CSharpTestbench {
    class Program {

        private static readonly string DATA_FOLDER_NAME = "TestbenchData";
        private static readonly string THIS_LANG = "cs";

        private static readonly int DEFAULT_INT = 114;
        private static readonly float DEFAULT_FLOAT = 114.514f;
        private static readonly string DEFAULT_STR = "shit";
        private static readonly int DEFAULT_LIST_LEN = 5;

        static void GetStructMsgsName(ref List<Type> structs, ref List<Type> msgs) {
            var nsname = typeof(BP.OpCode).Namespace;
            var bpmessage_t = typeof(BP.BpMessage);
            var bpstruct_t = typeof(BP.BpStruct);
            var classes = from ty in Assembly.GetExecutingAssembly().GetTypes()
                          where ty.IsClass && ty.Namespace == nsname && ty.IsSubclassOf(bpstruct_t) && ty.Name != bpstruct_t.Name && ty.Name != bpmessage_t.Name
                          select ty;

            msgs.AddRange(from ty in classes
                          where ty.IsSubclassOf(bpmessage_t)
                          select ty);
            structs.AddRange(from ty in classes
                             where (!ty.IsSubclassOf(bpmessage_t))
                             select ty);
        }

        private class VariablesEntry {
            public string mName = "";
            public Type mType = null;
            public FieldInfo mVisitor = null;
        }
        static Dictionary<string, List<VariablesEntry>> GetVariablesType(List<Type> allty) {
            var result = new Dictionary<string, List<VariablesEntry>>();

            foreach (var ty in allty) {
                var intermediary = new List<VariablesEntry>();
                var members = from member in ty.GetFields(BindingFlags.Public | BindingFlags.Instance)
                              select member;

                foreach(var member in members) {
                    intermediary.Add(new VariablesEntry() {
                        mName = member.Name,
                        mType = member.FieldType,
                        mVisitor = member
                    });
                }

                result.Add(ty.Name, intermediary);
            }

            return result;
        }

        static object GenerateDefaultValue(Dictionary<string, List<VariablesEntry>> typedict, Type t) {
            // process enum first. reveal its unerlying type first
            Type ot = t;
            if (ot.IsEnum) {
                t = t.GetEnumUnderlyingType();
            }

            object instance;
            if (t == typeof(byte)) instance = (byte)DEFAULT_INT;
            else if (t == typeof(sbyte)) instance = (sbyte)DEFAULT_INT;
            else if (t == typeof(ushort)) instance = (ushort)DEFAULT_INT;
            else if (t == typeof(short)) instance = (short)DEFAULT_INT;
            else if (t == typeof(uint)) instance = (uint)DEFAULT_INT;
            else if (t == typeof(int)) instance = DEFAULT_INT;
            else if (t == typeof(ulong)) instance = (ulong)DEFAULT_INT;
            else if (t == typeof(long)) instance = (long)DEFAULT_INT;
            else if (t == typeof(float)) instance = DEFAULT_FLOAT;
            else if (t == typeof(double)) instance = (double)DEFAULT_FLOAT;
            else if (t == typeof(string)) instance = DEFAULT_STR;
            else {
                // this is a complex structure
                instance = Activator.CreateInstance(t);
                AssignVariablesValue(typedict, instance);
                return instance;
            }

            // process enum finally
            if (ot.IsEnum) {
                return Enum.ToObject(ot, instance);
            } else {
                return instance;
            }
        }
        static void AssignVariablesValue(Dictionary<string, List<VariablesEntry>> typedict, object instance) {
            foreach (var member in typedict[instance.GetType().Name]) {
                if (member.mType.IsGenericType) {
                    // dynamic array
                    var item_t = member.mType.GetGenericArguments()[0];

                    member.mType.GetMethod("Clear").Invoke(member.mVisitor.GetValue(instance), null);
                    for(int i = 0; i < DEFAULT_LIST_LEN; ++i) {
                        member.mType.GetMethod("Add").Invoke(member.mVisitor.GetValue(instance), new[] { GenerateDefaultValue(typedict, item_t) });
                    }
                } else if (member.mType.IsArray) {
                    // static array
                    var item_t = member.mType.GetElementType();
                    var absarray = (member.mVisitor.GetValue(instance) as Array);

                    for (int i = 0; i < absarray.Length; ++i) {
                        absarray.SetValue(GenerateDefaultValue(typedict, item_t), i);
                    }
                } else {
                    // single
                    member.mVisitor.SetValue(instance, GenerateDefaultValue(typedict, member.mType));
                }

            }
        }

        static void BenchmarkTest(Dictionary<string, List<VariablesEntry>> typedict, List<Type> msgs) {
            MemoryStream ms = new MemoryStream();

            foreach (var msg in msgs) {
                // create example first
                var instance = (BP.BpMessage)Activator.CreateInstance(msg);
                AssignVariablesValue(typedict, instance);
                Console.WriteLine($"Benchmarking {msg.Name}...");

                // write and rewind
                BP.BPHelper.UniformSerialize(instance, new BinaryWriter(ms));
                ms.Seek(0, SeekOrigin.Begin);

                // read and clear
                BP.BPHelper.UniformDeserialize(new BinaryReader(ms));
                ms.SetLength(0);
            }
        }

        static void Main(string[] args) {
            // get necessary variables
            List<Type> structs = new List<Type>(),
                msgs = new List<Type>();
            GetStructMsgsName(ref structs, ref msgs);

            List<Type> allty = new List<Type>(structs);
            allty.AddRange(msgs);
            var typedict = GetVariablesType(allty);

            Console.WriteLine("===== Serialize & Deserialize Benchmark =====");
            BenchmarkTest(typedict, msgs);
        }
    }
}
