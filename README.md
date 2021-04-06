# duplist
duplist is a command line tool that recursively searches a directory for duplicate files.

# two versions
duplist was originally written in python, but I ported it to C. I left the python version in this repository a couple reasons. First, because it is more terse and easy to read, so it can be used as a reference, and second, because it will work on more platforms while the C version only supports GNU/Linux currently. I consider the C version to be the "official" version. **The rest of this readme assumes you are using the C version**. The python version is mostly the same, you can just read the source code to see how it works.

# installation
Clone the repository and run `sudo make install`.

# usage
Simply pass duplist the directory you want to search example: `duplist ~/Pictures`. By default duplist will list the duplicates from largest to smallest. You can optionally pass `--reverse` or `r` (both do the same thing) to reverse the order. duplist prints the list to stdout so you can stream the output to a file like this `duplist ~/Pictures > ~/Pictures/duplicates.txt`.

# how does it work?
duplist makes a list of every file contained in a directory (and its subdirectories) then orders the list based on size. It then groups together files of the same size and checks the first kibibyte of both files to be reasonably sure they are the same file. duplist cannot delete files itself, it merely lists files which it thinks are duplicates. **It is not gaurenteed that the files listed as duplicates are exactly the same, just very likely. I do not recommend automatically deleting files which are listed as duplicates. I am not responsible for data loss.**
