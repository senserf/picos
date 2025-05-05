Execute make_index in this directory to create the index. Then make sure that
the directory path is available in TCLLIBPATH when tclsh is run, e.g.,

export TCLLIBPATH=/home/nripg/SOFTWARE/PICOS/PICOS/Scripts/Packages/

In Tcl, lappend the dir to auto_path, e.g.,

lappend auto_path "/home/nripg/SOFTWARE/PICOS/PICOS/Scripts/Packages/"


