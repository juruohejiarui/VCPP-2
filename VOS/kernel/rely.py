import os

rely : dict[int, list] = {}
eleDict : dict[str, int] = {}
src : list[(str, str)] = []

def readDirectory(path : str) :
    global src, srcDict
    files = os.listdir(path)
    print(f"Reading {path}")
    for element in files :
        subPath = os.path.join(path, element)
        if os.path.isdir(subPath) :
            readDirectory(subPath)
        else :
            if element.endswith('.h') or element.endswith('.c') or element.endswith('.S') :
                src.append((subPath, path))
                eleDict[subPath] = len(src) - 1


def readFileRelies(eleId : int) :
    global rely, eleDict, src
    path = src[eleId][0]
    print(f"Reading {path}")
    lines = open(path, 'r', encoding = 'utf-8').readlines()
    rely[eleId] = []
    for line in lines :
        # skip the blank in the front of the line
        i = 0
        while i < len(line) and (line[i] == ' ' or line[i] == '\t') :
            i += 1
        # skip the comment
        if i + 1 < len(line) and line[i] == '/' and line[i + 1] == '/' :
            continue
        if line.startswith('#include \"') :
            i += len('#include \"')
            nxtQuote = line.find('\"', i)
            if nxtQuote != -1 :
                includePath = os.path.abspath(os.path.join(src[eleId][1], line[i : nxtQuote]))
                includePath = os.path.join("./", os.path.relpath(includePath, './'))       
                print(f"\t-> {includePath}", end = '\t')
                rely[eleId].append(eleDict[includePath])
    print()

visSet : set[int] = set()
outFile = open('.depend', 'w', encoding = 'utf-8')
def outputRely(path : str) :
    global rely, eleDict, visSet
    eleId = eleDict[path]
    if eleId in visSet : return 
    visSet.add(eleId)
    outFile.write(f"{path} ")
    for relyId in rely[eleId] :
        outputRely(src[relyId][0])

def main() :
    global rely, eleDict
    readDirectory('./')
    for i in range(len(src)) :
        readFileRelies(i)
    objFiles = []
    for (path, _) in src :
        if path.endswith('.c') or path.endswith('.S') :
            objFiles.append(path[0 : -2] + '.o')
            outFile.write(f"{path[0 : -2]}.o : ")
            outputRely(path)
            outFile.write('\n')
            if path.endswith('.c') :
                outFile.write('\t${CC} ${CFLAGS} -c ' + f"{path}\n")
            else :
                outFile.write('\t${CC} -E ' + f"{path} > " + f"{path[0 : -2]}.s\n")
                outFile.write('\t${ASM} {ASMFLAG} -o ' + f"{path[0 : -2]}.o {path[0 : -2]}.s\n")
    objList : str = ''
    for objFile in objFiles :
        objList += objFile + ' '
    outFile.write(f'ALLOBJS = {objList}\n')
    outFile.close()


if __name__ == '__main__' :
    main()
