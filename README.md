# dumpfiles
Windbg extension to extract file content from Windows Cache Manager.
All files are store in C:\output\

# How to use ?
!dumpfiles <fileptr>

# example :
<pre>
0: kd> !ca 0
...
ffffe00137036010 0000000000000000 0       File:  \ProgramData\Microsoft\User Account Pictures\user.png
...

0: kd> !ca ffffe00137036010 
ControlArea  @ ffffe00137036010
  Segment      ffffc00102f453b0  Flink      ffffe001370b34c8  Blink        ffffe0013794b528
  Section Ref                 0  Pfn Ref                   4  Mapped Views                0
  User Ref                    0  WaitForDel                0  Flush Count                 0
  File Object  ffffe001365cc3d0  ModWriteCount             0  System Views                0
  WritableRefs                0  
  Flags (8008080) File WasPurged UserWritable 

      \ProgramData\Microsoft\User Account Pictures\user.png

Segment @ ffffc00102f453b0
  ControlArea     ffffe00137036010  ExtendInfo    0000000000000000
  Total Ptes                    40
  Segment Size               40000  Committed                    0
  Flags (c0000) ProtectionMask 

Subsection 1 @ ffffe00137036088
  ControlArea  ffffe00137036010  Starting Sector        0  Number Of Sectors   40
  Base Pte     ffffc00102f46e00  Ptes In Subsect       40  Unused Ptes          0
  Flags                   8000d  Sector Offset          0  Protection           6
  Accessed 
  Flink        0000000000000000  Blink   0000000000000000  MappedViews          0


0: kd> .load dumpfiles; !dumpfiles ffffe001365cc3d0; .unload
################DataSectionObject###############
4096 bytes written @0000000000000000!
4096 bytes written @0000000000001000!
4096 bytes written @0000000000002000!
4096 bytes written @0000000000003000!
################ImageSectionObject###############
Nothing to extract from ImageSectionObject 
################SharedCacheMap###################
Nothing to extract from SharedCacheMap 
File saved at C:\output\\ProgramData\Microsoft\User Account Pictures\user.png
Unloading dumpfiles extension DLL
</pre>