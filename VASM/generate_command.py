
outfile = open("command_list.out", 'w')
infile = open("command_list.in", 'r')
cnt = 0
def generate(line : str, cmd : str, pos : int) :
    global cnt
    if pos >= len(line):
        outfile.write('%-20s, ' % ('"' + cmd + '"'))
        cnt += 1
        if cnt % 10 == 0:
            outfile.write('\n')
        return 
    if line[pos] == '[':
        rpos : int = line.find(']', pos)
        parts : list[str] = line[pos + 1 : rpos].split('/')
        # print(parts)
        for p in parts:
            generate(line, cmd + p, rpos + 1)
    else:
        generate(line, cmd + line[pos], pos + 1)

if __name__ == "__main__":
    lines = infile.read().splitlines()
    cnt_ls = []
    for line in lines:
        lst_cnt = cnt
        generate(line, "", 0)
        outfile.write('\n')
        cnt_ls.append(cnt - lst_cnt)
    outfile.write(f'\n {cnt_ls}')
