#!/usr/bin/env python3
import glob
import sys
import os
import shutil

""" 
This script add labels to language files. Usage:

 python ./add_translations_entry.py LABEL "STRING VALUE"
 
It will iterate all language files and will add new entry with given label, unless label 
is already present in file. When run it copies all files before changes to ./src/lang/bckps

EXAMPLE:
 python add_translations_entry.py INTL_NEW_LABEL "Some string for output"

"""

if (len(sys.argv) < 2):
    print("Not enough args (2) LABEL \"FIRST VALUE\"")
    exit(0)
label = sys.argv[1]
value = sys.argv[2]

print("adding\nconst char {0}[] PROGMEM = \"{1}\"; to:".format(label,value))
if not os.path.exists("./src/lang/bckps"):
        os.mkdir("./src/lang/bckps")
for filepath in glob.iglob(r'./src/lang/intl_*.h'):
    print(filepath)
    shutil.copy(filepath,"./src/lang/bckps/")
    with open(filepath, 'rb+') as lang:
        if label in lang.read():
            print("{} already in {}".format(label,filepath))
            continue
        lang.seek(-1, os.SEEK_END)
        last = lang.read(1)
        if "\n" != last:
            lang.write("\n")
        lang.write("const char {0}[] PROGMEM = \"{1}\";\n".format(label, value))
        lang.close


