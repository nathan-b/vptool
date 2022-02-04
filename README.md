# vptool
A tool for parsing and manipulating VP (Volition Package) files.

# Usage
```
Usage: vptool <operation> <vp_file> [options]
  Valid operations: t / dump-index             Print the index of the package file
                    d / dump-file  <-f filename>  Dump the contents of a single file in the package
                    f / extract-file <-f filename> <-o output-file>  Extract the contents of a single file to disk
                    x / extract-all  [-o output-path]  Extract the entire package to the output path (or current directory)
                    r / replace-file <-f filename> <-i input-file>  Replace the contents of a single file
                    p / build-package <-i input-path>  Build a new vp file with the contents of input-path
```

# Operations

# dump-index
```
./vptool dump-index mypackage.vp
```
Prints the index listing to the console. 

The index is the internal directory structure of the VP file. It will tell you what filenames and directories are contained in the package.

# dump-file
```
./vptool dump-file mypackage.vp -f InternalFile.fs2
```
Reads a file from the package and writes it to the console.

You do not need to provide the full internal path to the file, just the file's name. 

# extract-file
```
./vptool extract-file -f InternalFile.fs2 -o /tmp/outfile.fs2
```
Reads a file from the package and writes it to the given output filename.

As with the above, you do not need to provide the full internal path to the file, just the file's name. This is equivalent to:
```
./vptool dump-file -f InternalFile.fs2 > /tmp/outfile.fs2
```

# extract-all
```
./vptool extract-all mypackage.vp -o /tmp
```
Extracts every file and directory in the package, replicating the internal directory structure.

For example, let's take a simple, small package:
```
./vptool dump-index test.vp
data/
   maps/
      test_map.dds
      test_data.pcx
   missions/
      test_mission.fs2
```
Running `extract-all` on this package would create a top-level `data` directory which contains two subdirectories, `maps` and `missions`. These subdirectories would contain the files listed above.

The `-o` parameter is optional. If it is not specified, the package is extracted into the current directory. Given that VP files generally contain a single, top-level `data` directory, this would put the `data` directory in the current directory.

# replace-file
```
./vptool replace-file mypackage.vp -f ToBeReplaced.ext -i MyNewFile.ext
```
Finds a file in the package and replaces it with the provided input file.

A common task when working with VP files is to update the contents of a single file in the package, sometimes multiple times in a row as you tweak the file. This operation is designed to facilitate that workflow.

Example:
```
$ ./vptool extract-file mypackage.vp -f MyCoolMission.fs2 -o ~/src/missions/MyCoolMission.fs2
...edit the mission...
$ ./vptool replace-file mypackage.vp -f MyCoolMission.fs2 -i ~/src/missions/MyCoolMission.fs2
...test the mission...
...edit the mission...
$ ./vptool replace-file mypackage.vp -f MyCoolMission.fs2 -i ~/src/missions/MyCoolMission.fs2
...repeat...
```

# build-package
```
./vptool build-package my_new_package.vp -i ~/path/to/package/dir
```
Builds a new VP file from the given directory.

Traditionally, VP files start with a top-level `data` directory. The `build-package` operation honors this by looking for a directory named `data` in the given directory. If one is present, it chooses that as the top-level directory. Otherwise, the provided directory itself is the top-level directory. This heuristic is designed so that `vptool` will generally do the right thing without you having to think about it (you can either point _to_ the data directory, or point to the directory _containing_ the data directory, and it will do the right thing in both cases), but if you really want to create a VP file which doesn't conform to the standard format, you can do that too. No judgement.
