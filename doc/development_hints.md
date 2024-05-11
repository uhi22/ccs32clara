

# Size comparisions depending on the used schema

Precondition: Use common exi document (in the makefile, set the compiler option -DUSE_SAME_DOC_FOR_ENCODER_AND_DECODER).

Limits: The STM32F103RE has 512k flash and 64k RAM.

## DIN only

*   text    data   bss(RAM) 
* 138568    2888   31692 

## DIN and ISO1 (ISO 2013)

(This is the combination which KIA EV6 uses, according to a log file which was recorded with pyPLC.)
In the makefile, set the compiler option -DUSE_ISO1
* 242028    2888   54892
 
## DIN and ISO2 (ISO 2016)

In the makefile, set the compiler option -DUSE_ISO2
* 740352    2888   54628

