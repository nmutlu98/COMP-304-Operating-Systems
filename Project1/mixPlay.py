import os
import random
import sys
music_list = os.popen("ls | grep .*.mp3").readlines()
number_of_songs = int(sys.argv[1])
for i in range(0,number_of_songs):
	song_number =  random.randint(0,len(music_list)-1)
	os.system("play "+os.getcwd()+"/"+music_list[song_number])