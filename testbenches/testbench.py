import CodeGenTest as BP
import typing, inspect, timeit, os, io
import argparse

SS: io.BytesIO = io.BytesIO()

DATA_FOLDER_NAME: str = 'TestbenchData'

THIS_LANG: str = 'py'
ALL_LANGS: tuple[str] = (
    THIS_LANG, 'cs', 'cpp'
)

DEFAULT_INT: int = 114
DEFAULT_FLOAT: float = 114.514
DEFAULT_STR: str = "shit"
DEFAULT_LIST_LEN: int = 5

BENCHMARK_TIMES: int = 100

def GetStructMsgsName() -> tuple[list[str]]:
    msgs: list[str] = []
    structs: list[str] = []

    for name, obj in inspect.getmembers(BP):
        if inspect.isclass(obj):
            # get class inherit list and check it
            mro = inspect.getmro(obj)
            if BP.BpMessage in mro and name != 'BpMessage':
                msgs.append(name)
            elif BP.BpStruct in mro and name != 'BpMessage' and name != 'BpStruct':
                structs.append(name)

    return (msgs, structs, )

def GetVariablesType(name_list: list[str]) -> dict[str, dict]:
    result: dict[str, dict] = {}
    for name in name_list:
        intermediate: dict = {}
        for vname, vtype in typing.get_type_hints(getattr(BP, name)).items():
            if not vname.startswith('_'):
                intermediate[vname] = vtype

        result[name] = intermediate

    return result

def AssignVariablesValue(typedict: dict[str, dict], clsname: str, instance):
    for vname, vtype in typedict[clsname].items():
        if typing.get_origin(vtype) == list:
            # list assigner
            # try getting its length
            listlen: int = len(getattr(instance, vname))
            if listlen == 0:
                # dynamic list. give a number
                listlen = DEFAULT_LIST_LEN
            else:
                # static list. keep its original length
                pass

            # assign value
            itemtype = typing.get_args(vtype)[0]
            if itemtype == int:
                setattr(instance, vname, [DEFAULT_INT] * listlen)
            elif itemtype == float:
                setattr(instance, vname, [DEFAULT_FLOAT] * listlen)
            elif itemtype == str:
                setattr(instance, vname, [DEFAULT_STR] * listlen)
            else:
                ls = list(itemtype() for _ in range(listlen))
                for item in ls:
                    AssignVariablesValue(typedict, itemtype.__name__, item)
                setattr(instance, vname, ls)
        else:
            # non-list assigner
            if vtype == int:
                setattr(instance, vname, DEFAULT_INT)
            elif vtype == float:
                setattr(instance, vname, DEFAULT_FLOAT)
            elif vtype == str:
                setattr(instance, vname, DEFAULT_STR)
            else:
                item = vtype()
                AssignVariablesValue(typedict, vtype.__name__, item)
                setattr(instance, vname, item)

def CompareInstance(typedict: dict[str, dict], clsname: str, inst1, inst2) -> bool:
    for vname, vtype in typedict[clsname].items():
        instattr1 = getattr(inst1, vname, None)
        instattr2 = getattr(inst2, vname, None)

        # check no attr
        if instattr1 is None or instattr2 is None:
            return False
        # check type
        if not (type(instattr1) == type(instattr2)):
            return False
        
        if typing.get_origin(vtype) == list:
            # list checker
            # get item type and check len
            itemtype = typing.get_args(vtype)[0]
            if len(instattr1) != len(instattr2):
                return False
            
            # check item one by one
            if itemtype == int or itemtype == float or itemtype == str:
                for i1, i2 in list(zip(instattr1, instattr2)):
                    if i1 != i2:
                        return False
            else:
                for i1, i2 in list(zip(instattr1, instattr2)):
                    if not CompareInstance(typedict, itemtype.__name__, i1, i2):
                        return False
        else:
            # non-list checker
            if vtype == int or vtype == float or vtype == str:
                if instattr1 != instattr2:
                    return False
            else:
                CompareInstance(typedict, vtype.__name__, instattr1, instattr2)

    return True
                
def WriteToFile(instance: BP.BpMessage, filename: str):
    global SS
    instance.Serialize(SS)
    with open(filename, 'wb') as f:
        f.write(SS.getvalue())
    SS.seek(io.SEEK_SET, 0)
    SS.truncate(0)

def ReadFromFile(instance: BP.BpMessage, filename: str) -> BP.BpMessage:
    global SS
    with open(filename, 'rb') as f:
        SS.write(f.read())
    SS.seek(io.SEEK_SET, 0)
    instance.Deserialize(SS)
    SS.seek(io.SEEK_SET, 0)
    SS.truncate(0)

    return instance

def BenchmarkTestSerializeEntry(instance: BP.BpMessage):
    global SS
    BP.UniformSerialize(instance, SS)
    SS.seek(io.SEEK_SET, 0)
def BenchmarkTestDeserializeEntry():
    global SS
    _ = BP.UniformDeserialize(SS)
    SS.seek(io.SEEK_SET, 0)
def BenchmarkTest(vtypes: dict[str, dict], msgs: list[str]):
    global SS

    for name in msgs:
        # create a example instance
        instance = getattr(BP, name)()
        AssignVariablesValue(vtypes, name, instance)

        # write to file
        print(f'Benchmarking {name}...')
        env: dict = {
            'BenchmarkTestSerializeEntry': BenchmarkTestSerializeEntry,
            'BenchmarkTestDeserializeEntry': BenchmarkTestDeserializeEntry,
            'instance': instance
        }
        ser_sec = timeit.timeit(stmt='BenchmarkTestSerializeEntry(instance)', number=BENCHMARK_TIMES, globals=env) / BENCHMARK_TIMES
        deser_sec = timeit.timeit(stmt='BenchmarkTestDeserializeEntry()', number=BENCHMARK_TIMES, globals=env) / BENCHMARK_TIMES
        print(f'Serialize {ser_sec * 1e6:7f} usec. Deserialize {deser_sec * 1e6:7f} usec.')

        # mannually clear SS
        SS.seek(io.SEEK_SET, 0)
        SS.truncate(0)

EXPECTED_STR_DLEN = len(DEFAULT_STR.encode('utf-8', errors='ignore')) + 4
EXPECTED_SIZE: dict[str, int] = {
    'codegen_test_align': (1 + 3) + 4 + (1 * 3 + 1) + (4 * 3) + (1 + 3) + 4 + (1 * 3 + 1) + (4 * 3),
    'codegen_test_manual_align': 1 + 4 + (1 + 3) + 4 + (EXPECTED_STR_DLEN + 3) + EXPECTED_STR_DLEN,
    'codegen_test_natural': 4 + (4 * 3) + (4 * 5) + (1 + 3) + (1 * 3 + 1) + (1 * 5 + 3),
    'codegen_test_natural_alternative': 4 + (4 * 3) + 4 + (4 * 3) + 4 + (4 * 3),
    'codegen_test_narrow': 4 + (4 * 3) + (4 * DEFAULT_LIST_LEN + 4) + 
        EXPECTED_STR_DLEN + (EXPECTED_STR_DLEN * 3) + (EXPECTED_STR_DLEN * DEFAULT_LIST_LEN + 4) + 
        1 + (1 * 3) + (1 * DEFAULT_LIST_LEN + 4) +
        1 + (1 * 3) + (1 * DEFAULT_LIST_LEN + 4),
    'codegen_test_narrow_alternative': 4 + (4 * 3) + (4 * DEFAULT_LIST_LEN + 4) + 
        4 + (4 * 3) + (4 * DEFAULT_LIST_LEN + 4) + 
        EXPECTED_STR_DLEN + (EXPECTED_STR_DLEN * 3) + (EXPECTED_STR_DLEN * DEFAULT_LIST_LEN + 4) + 
        EXPECTED_STR_DLEN + (EXPECTED_STR_DLEN * 3) + (EXPECTED_STR_DLEN * DEFAULT_LIST_LEN + 4) + 
        4 + (4 * 3) + (4 * DEFAULT_LIST_LEN + 4),
}
def SizeTest(vtypes: dict[str, dict], msgs: list[str]):
    global SS

    for name in msgs:
        # check whether need size test
        if not (name in EXPECTED_SIZE):
            print(f'Skip {name} check.')
            continue

        # create a example instance
        instance = getattr(BP, name)()
        AssignVariablesValue(vtypes, name, instance)

        # check size
        print(f'Checking {name} size...')
        instance.Serialize(SS)
        v_got = len(SS.getvalue())
        v_expect = EXPECTED_SIZE[name]
        if v_expect != v_got:
            print(f'Failed! Expect {v_expect} got {v_got}.')

        # clear stream
        SS.seek(io.SEEK_SET, 0)
        SS.truncate(0)

def LangInteractionTest(vtypes: dict[str, dict], msgs: list[str]):
    standard: dict[str, BP.BpMessage] = {}
    for name in msgs:
        # create and insert
        instance = getattr(BP, name)()
        AssignVariablesValue(vtypes, name, instance)
        standard[name] = instance

        # write to file
        filename = os.path.join(DATA_FOLDER_NAME, THIS_LANG, name + '.bin')
        WriteToFile(instance, filename)

    # re-read and check them again
    for lang in ALL_LANGS:
        if not os.path.isdir(os.path.join(DATA_FOLDER_NAME, lang)):
            print(f'Skip {lang} language test!')
            continue

        print(f'Start testing {lang} language.')
        for name in msgs:
            # read from file
            filename = os.path.join(DATA_FOLDER_NAME, lang, name + '.bin')
            if not os.path.isfile(filename):
                print(f'Skip msg {name} test!')
                continue
            
            newinstance = getattr(BP, name)()
            ReadFromFile(newinstance, filename)

            # compare
            print(f'Checking {name}...')
            if not CompareInstance(vtypes, name, standard[name], newinstance):
                print('Failed!')
                print(vars(standard[name]))
                print(vars(newinstance))

def Testbench(skip_benchmark: bool, skip_size: bool, skip_langinter: bool):
    # make sure dir
    for lang in ALL_LANGS:
        os.makedirs(os.path.join(DATA_FOLDER_NAME, lang), exist_ok=True)

    (msgs, structs, ) = GetStructMsgsName()
    vtypes: dict[str, dict] = GetVariablesType(msgs + structs)

    # benchmark speed
    if not skip_benchmark:
        print('===== Serialize & Deserialize Benchmark =====')
        BenchmarkTest(vtypes, msgs)

    # check size
    if not skip_size:
        print('===== Data Size Check =====')
        SizeTest(vtypes, msgs)

    # check lang compatibility
    if not skip_langinter:
        print('===== Language Interaction =====')
        LangInteractionTest(vtypes, msgs)
    
    print('')
    print('ALL DONE!')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog = 'Python Code Testbench',
        description = 'This script will benchmark generated code and test language interactions.'
    )
    parser.add_argument('-b', '--skip-benchmark', help = 'Skip Python benchmark.', action='store_true', dest='skip_benchmark')
    parser.add_argument('-s', '--skip-size', help = 'Skip data size test.', action='store_true', dest='skip_size')
    parser.add_argument('-l', '--skip-langs', help = 'Skip language interaction test.', action='store_true', dest='skip_langs')
    args = parser.parse_args()

    Testbench(args.skip_benchmark, args.skip_size, args.skip_langs)
