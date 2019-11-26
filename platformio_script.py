Import("env")
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
  if not os.path.exists("./builds"):
    os.mkdir("./builds")

  configName = env.Dump('PIOENV').replace("'", "")
  sectionName = 'env:' + configName
  lang = config.get(sectionName, "lang")
  target_name = lang
  shutil.copy(target[0].path, "./builds/latest_"+target_name.lower()+".bin")

env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)