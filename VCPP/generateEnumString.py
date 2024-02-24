infile = open("enum.in", "r")
outfile = open("enum.out", "w")

def isLetter(ch : str) :
    return ord('a') <= ord(ch) and ord(ch) <= ord('z') or ord('A') <= ord(ch) and ord(ch) <= ord('Z') or ch == '_'
def isNumber(ch : str) :
    return ord('0') <= ord(ch) and ord(ch) <= ord('9')

if __name__ == "__main__":
    lines = infile.readlines()
    cnt = 0
    for line in lines:
        i, j = 0, 0
        if len(line) >= 2 and line[0 : 2] == '//':
            outfile.write(line)
            continue
        while i < len(line):
            if isLetter(line[i]):
                while j + 1 < len(line) and (isNumber(line[j + 1]) or isLetter(line[j + 1])):
                    j += 1
                outfile.write(f"\"{line[i : j + 1]}\"")
            else:
                outfile.write(line[i])
            j += 1
            i = j