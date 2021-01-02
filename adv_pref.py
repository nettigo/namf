import argparse
import os.path

parser = argparse.ArgumentParser('Serach for default options')

parser.add_argument('filename', help='File with options')
parser.add_argument('-d', '--default', nargs='?')

args = parser.parse_args()

if os.path.isfile(args.filename):
    #print("Zawartość {}".format(args.filename))
    file = open(args.filename, 'r')
    print(file.read())
    file.close()
else:
    if args.default:
        print(args.default)
    else:
        print("")