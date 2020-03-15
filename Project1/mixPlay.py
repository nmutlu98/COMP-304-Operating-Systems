import os 
import requests
import sys


url = "https://github.com/"

print("Please enter a username")
if sys.argv:
	url += sys.argv[0]
	content = requests.get(url)
	if content:
		print("User exists")
	else:
		print("User does not exist")
else:
	print("Please enter a username")
