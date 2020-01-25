Import("env")
from os.path import basename

import os
import shutil

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

config = configparser.ConfigParser()
config.read("platformio.ini")

flags = " ".join(env['LINKFLAGS'])
flags = flags.replace("-u _printf_float", "")
flags = flags.replace("-u _scanf_float", "")
newflags = flags.split()
env.Replace(
    LINKFLAGS=newflags
)


def after_build(source, target, env):
    f = open('version.txt', 'r')
    val = (f.read()).split(' ', 2)
    print(val)

    firmware_path = str(target[0].path)
    firmware_name = basename(firmware_path)

    if not os.path.exists("./builds"):
        os.mkdir("./builds")

    configName = env.Dump('PIOENV').replace("'", "")
    sectionName = 'env:' + configName
    lang = config.get(sectionName, "lang").upper()

    dest = '/home/viciu/Documents/dev/fw.air.nettigo.pl/data/{0}.dir/{1}_{2}.bin'.format(val[0], val[1],lang)
    print("Uploading {0} to {1}".format(firmware_name, dest))
    shutil.copy(target[0].path, dest)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)
