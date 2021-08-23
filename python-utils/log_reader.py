import argparse  
from pathlib import Path
           

parser = argparse.ArgumentParser(description='command line options')
parser.add_argument('--lvl', action="store", dest="lvl", default='info', help="scaning log level")    
parser.add_argument('--dir', action="store", dest="dir", default=f'{str(Path.home())}/Library/Application Support/vcmi', help="logs directory")    
parser.add_argument('--file', action="store", dest="file", default='server_log.txt', help="logs filename")
parser.add_argument('--text', action="store", dest="text", default=None, help="text for search")
parser.add_argument('--filter', action="store", dest="filter", default=None, help="log filter")
inputs = parser.parse_args()

print(inputs.lvl)
print(inputs.dir)
print(inputs.file)

with open(inputs.dir + '/' + inputs.file, 'r') as file:
  lines = file.readlines()
  for line in lines:
    if str(inputs.filter).upper() in line or str(inputs.text).lower() in line.lower():
      print(line)

