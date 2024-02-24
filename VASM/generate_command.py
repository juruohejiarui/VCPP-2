
outfile = open("command.list", 'w')
toutfile = open("tcommand.list", 'w')
outfile_str = open("command_str.list", 'w')
toutfile_str = open("tcommand_str.list", 'w')

infile = open("command_list.in", 'r')


tcmd_dic = {}
tcmd_ls = []
cnt = 0

is_str = 0
def generate(line : str, cmd : str, pos : int) :
    global cnt
    if pos >= len(line):
        outfile_str.write('%-19s, ' % ('"' + cmd + '"'))
        if cmd == 'new' or cmd == 'and' or cmd == 'or' or cmd == 'xor' or cmd == 'not': 
            outfile.write('%-19s, ' % ('_' + cmd))
        else:    
            outfile.write('%-19s, ' % (cmd))
        tcmd = cmd.split('_')[-1]
        if tcmd not in tcmd_dic:
            tcmd_dic[tcmd] = 1
            tcmd_ls.append(tcmd)
        cnt += 1
        if cnt % 10 == 0:
            outfile.write('\n')
            outfile_str.write('\n')
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
    
    infile = open("command_list.in", 'r')
    lines = infile.read().splitlines()
    for line in lines:
        lst_cnt = cnt
        generate(line, "", 0)
        outfile.write('\n')
        outfile_str.write('\n')
        cnt = 0
    
    cnt = 0
    for tcmd in tcmd_ls:
        cnt += 1
        if tcmd == 'new' or tcmd == 'and' or tcmd == 'or' or tcmd == 'xor' or tcmd == 'not':
            toutfile.write('%-10s, ' % ('_' + tcmd))
        else:
            toutfile.write('%-10s, ' % (tcmd))
        toutfile_str.write('%-10s, ' % ('"' + tcmd + '"'))
        if cnt % 10 == 0:
            toutfile.write('\n')
            toutfile_str.write('\n')
