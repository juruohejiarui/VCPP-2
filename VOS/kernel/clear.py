import sys
import os

suffixs = ['.s', '.o']
def scanDir(path : str) :
	files = os.listdir(path)
	flag = [False for _ in range(len(suffixs))]
	for file in files :
		subPath = os.path.join(path, file)
		if os.path.isdir(subPath) :
			scanDir(subPath)
		else :
			for (idx, suf) in enumerate(suffixs) :
				if flag[idx] : continue
				if file.endswith(suf) :
					flag[idx] = True

	cmd = "rm "
	for (idx, prefix) in enumerate(suffixs) :
		if not flag[idx] : continue
		cmd += os.path.join(path, "*" + prefix)
		cmd += " "
	if cmd != "rm ":
		os.system(cmd)

if __name__ == "__main__" :
	scanDir("./") 