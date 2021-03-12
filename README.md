# simplefs
```
mkdir build
cd build
cmake ..
make
./simplefs
```

# Usage 

Disk is simulated in file "simplefs.data".

Supported operations:
- ls <directory>
- cd <directory>
- touch <filename>
- mkdir <directory_name>
- append <filename> "data" (quotes are necessery)
- cat <filename>
- rm <name> (works with files and empty dirs)
- exit (save superblock and exit)


If you don't exit properly then fs becomes corrupted and you should recreate it (delete simplefs.data).
