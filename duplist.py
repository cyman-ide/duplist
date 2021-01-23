#!/usr/bin/python3

import os, sys

def get_files(folder):
    file_list = []
    for (root, folder_names, filenames) in os.walk(folder):
        for filename in filenames:
            full_path = os.path.join(root, filename)
            file_list.append((os.path.getsize(full_path), full_path))
    return file_list

def get_duplicates(folder):
    file_list = get_files(folder)

    file_list.sort()

    duplicates = []
    last_item = file_list[0]
    dup_count = 0
    for item in file_list[1:]:
        if item[0] == last_item[0]:
            if dup_count == 0:
                duplicates.append(last_item)
            duplicates.append(item)
            dup_count += 1
        else:
            dup_count = 0
        last_item = item

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
        folder = sys.argv[1]
        last_item_size = 0
        duplicates = get_duplicates(folder)
        if len(duplicates) == 0:
            print("no duplicates found")
        else:
            print("potential duplicates found:")
            for item in duplicates:
                if item[0] != last_item_size: print()
                print( "(%s) %s" % (data_size_fmt(item[0]), os.path.relpath(item[1], folder)) )
                last_item_size = item[0]
    else:
        print("error: no directory provided")

