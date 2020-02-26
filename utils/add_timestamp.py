# When there is file .add_build_time in main repo directory timestamp will be added to version in header (only in default nodemcuv2 target)

import os.path

path = os.path.join(os.curdir,".add_build_time")
if os.path.exists(path):
    print("-DBUILD_TIME=$UNIX_TIME")
