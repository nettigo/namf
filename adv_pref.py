import argparse
import os.path

parser = argparse.ArgumentParser(description="Check if filename exists and output it's content, if not - output -d value. ")

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