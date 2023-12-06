
outfile = open("command_list.out", 'w')
infile = open("command_list.in", 'r')

tcmd_dic = {}
tcmd_ls = []
cnt = 0

is_str = 0
def generate(line : str, cmd : str, pos : int) :
    global cnt
    if pos >= len(line):
        if is_str == 1:
            outfile.write('%-19s, ' % ('"' + cmd + '"'))
        else:
            outfile.write('%-19s, ' % (cmd))
        tcmd = cmd.split('_')[-1]
        if tcmd not in tcmd_dic:
            tcmd_dic[tcmd] = 1
            tcmd_ls.append(tcmd)
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
    is_str = int(input())
    for line in lines:
        lst_cnt = cnt
        generate(line, "", 0)
        outfile.write('\n')
        cnt = 0

    for tcmd in tcmd_ls:
        if is_str == 1:
            outfile.write(f'"{tcmd}", ')
        else:
            outfile.write(f'{tcmd}, ')
