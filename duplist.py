#!/usr/bin/python3

import os, sys

def get_files(folder):
    file_list = []
    for (root, folder_names, filenames) in os.walk(folder):
        for filename in filenames:
            full_path = os.path.join(root, filename)
            file_list.append((os.path.getsize(full_path), os.path.relpath(full_path, folder)))
    return file_list

def get_duplicates(folder):
    file_list = get_files(folder)

    file_list.sort()

    duplicates = []
    for i in range(len(file_list)-1):
        if file_list[i][0] == file_list[i+1][0]:
            duplicates.append(file_list[i])
            duplicates.append(file_list[i+1])
            i += 1

    return duplicates

def data_size_fmt(size):
    suffix_list = ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB']
    for suffix in suffix_list:
        if size < 1024:
            return "%.2f %s" % (size, suffix)
        else:
            size /= 1024.0
    return "BIG"
        
if __name__ == "__main__":
    if len(sys.argv) > 1 and os.path.isdir(sys.argv[1]):
        last_item_size = 0
        duplicates = get_duplicates(sys.argv[1])
        if len(duplicates) == 0:
            print("no duplicates found")
        else:
            print("potential duplicates found:")
            for item in duplicates:
                if item[0] != last_item_size: print()
                print("(%s) %s" % (data_size_fmt(item[0]), item[1]))
                last_item_size = item[0]
    else:
        print("error: no directory provided")

