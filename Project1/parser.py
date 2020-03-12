import os
from bs4 import BeautifulSoup 
import requests
import sys
url = "https://www.bbc.com/news/world"
content = requests.get(url)
soup = BeautifulSoup(content.text, 'html.parser')

searched_part = soup.find_all( "a",{'class' : "gs-c-promo-heading gs-o-faux-block-link__overlay-link gel-pica-bold nw-o-link-split__anchor"})

for i in searched_part:
	print(i.get_text().encode("utf-8"))
