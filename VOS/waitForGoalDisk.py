import sys
import os
goalDisk = sys.argv[1]
print(f"Waiting for {goalDisk} to be plugged in...")
while True :
	list = os.listdir("/dev/")
	for ele in list :
		pth = os.path.join("/dev/", ele)
		if pth == goalDisk :
			sys.exit(0)