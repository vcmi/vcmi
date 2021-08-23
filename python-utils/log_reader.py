import argparse  
from pathlib import Path
           

parser = argparse.ArgumentParser(description='command line options')
parser.add_argument('--lvl', action="store", dest="lvl", default='info', help="scaning log level")    
parser.add_argument('--dir', action="store", dest="dir", default=f'{str(Path.home())}/Library/Application Support/vcmi', help="logs directory")    
parser.add_argument('--file', action="store", dest="file", default='server_log.txt', help="logs filename")
parser.add_argument('--text', action="store", dest="text", default=None, help="text for search")
parser.add_argument('--filter', action="store", dest="filter", default=None, help="log filter")
inputs = parser.parse_args()

<<<<<<< HEAD
print('Settings:')
print(f'LVL: {inputs.lvl}')
print(f'DIR: {inputs.dir}')
print(f'FILE: {inputs.file}')
print(f'TEXT: {inputs.text}')
print(f'FILTER: {inputs.filter}')
print('\r\n')
=======
print(inputs.lvl)
print(inputs.dir)
print(inputs.file)
>>>>>>> 4ce19d7a55895e9b3b45c0ed3840ec205509c65d

with open(inputs.dir + '/' + inputs.file, 'r') as file:
  lines = file.readlines()
  for line in lines:
    if str(inputs.filter).upper() in line or str(inputs.text).lower() in line.lower():
      print(line)

