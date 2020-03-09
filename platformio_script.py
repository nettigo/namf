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
    # f = open('version.txt', 'r')
    # val = (f.read()).split(' ', 2)
    # print(val)

    firmware_path = str(target[0].path)
    firmware_name = basename(firmware_path)

    if not os.path.exists("./builds"):
        os.mkdir("./builds")

    configName = env.Dump('PIOENV').replace("'", "")
    sectionName = 'env:' + configName
    lang = config.get(sectionName, "lang").lower()
    dest = 'builds/latest_{0}.bin'.format(lang)
    print("Uploading {0} to {1}".format(firmware_name, dest))
    shutil.copy(target[0].path, dest)

    firmware_path = str(source[0].path)
    firmware_name = basename(firmware_path)
    dest = 'builds/latest_{0}.elf'.format(lang)
    print("Uploading {0} to {1}".format(firmware_name, dest))
    shutil.copy(source[0].path, dest)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)
