import sys
import os
import time
goalDisk = sys.argv[1]
print(f"Waiting for {goalDisk} to be plugged in...")
lst : int = 0
while True :
	time.sleep(0.5)
	list = os.listdir("/dev/")
	for ele in list :
		pth = os.path.join("/dev/", ele)
		if pth == goalDisk :
			sys.exit(0)
	lst += 1
	if lst >= 20 :
		text = input("Have been wait for 10 seconds, do you want to exit?\n[y(default)/n]")
		if text == "n" or text == "" : 
			lst = 0
			print("keep waiting.")
			continue
		elif text != 'y' :
			print("Aborted.")
			sys.exit(-1)
		else:
			print("Invalid input, exit")
			sys.exit(-1)