import os
import sys


def get_all_files(directory):
    file_set = set()
    for root, dirs, files in os.walk(directory):
        for file in files:
            # Zapisujemy względne ścieżki plików
            relative_path = os.path.relpath(os.path.join(root, file), directory)
            file_set.add(relative_path)
    return file_set


def compare_directories(dir1, dir2):
    files_dir1 = get_all_files(dir1)
    files_dir2 = get_all_files(dir2)

    only_in_dir1 = files_dir1 - files_dir2
    only_in_dir2 = files_dir2 - files_dir1

    print("Files only in {}:".format(dir1))
    for file in only_in_dir1:
        print(file)

    print("\nFiles only in {}:".format(dir2))
    for file in only_in_dir2:
        print(file)


if len(sys.argv) < 3:
    print("Provide two language code as in docs folder.\n {} EN PL".format(sys.argv[0]))
    exit(0)
directory1 = 'docs/{}'.format(sys.argv[1])
directory2 = 'docs/{}'.format(sys.argv[2])

compare_directories(directory1, directory2)
