----------------------------------------------------
WEBBREW 005 EXTENDED SUPPORT 2 INSTALL INSTRUCTIONS:
----------------------------------------------------

 1. extract this zip
 2. open the "payload/webbrew/apps/" folder (double-click)
 3. copy/extract contents of appdist.zip into the apps folder (you dont have to extract everything, only the apps that you want to use)
 4. go back 3 folders
 5. open index.html (double-click) and bookmark it (star)
 6. all done! Below are the instructions on how to use WebBrew.

---------------------------
GENERAL USAGE INSTRUCTIONS:
---------------------------

You can launch WebBrew 005 by pressing the Launch WebBrew button. You can use the launcher to open the apps that you saved in the apps folder.

-----------------------------
WEBBREW SHELL (COMMAND LINE):
-----------------------------

WebBrew Payload Loader (WBPL) also contains a recovery binary of WebBrew SHell. You can use this to view system specs (RAM, CPU, ETC), recover IDB files, and store stuff in a filesystem.

Useful commands:
 - man: WebBrew SHell Manual (more on that bellow)
 - spec: open system and browser specs screen
 - disktool: backup/restore IDB FilerFS
 - mkdir: make a folder
 - cd: go into folder
 - ls: show files in current folder
 - get: download file from Filer
 - push: put a file into Filer
 - wget: download file from internet
 - rm: remove file/directory

There are more commands than these, but these are the most useful.

------------
WBSH Manual:
------------

Dont know all the commands yet? Dont worry! WebBrew SHell has a manual! You can view all the commands available by running "man". Then, run "man <COMMAND>", were <COMMAND> is the command you want to learn more about!  
-----------------------
WBPL Change Background:
-----------------------
Its simple! just copy a PNG background to "payload/wbpl_extra/bg.png"!