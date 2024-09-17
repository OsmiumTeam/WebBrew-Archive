function sleep(ms = 0) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

//less stuff
let less = false
document.addEventListener("keydown", function (e) {          
	if (less) {
		if (e.key === "q") {
			less = false
			SH_Output = "";
			SH_OutputOld = "";
			wbsh.returnsh()
		}
	}
})

//extras setup
let extras = document.createElement("div");
extras.setAttribute("id", "extra");
extras.setAttribute("style", "display:none;");

{

let a = document.createElement("input");
a.setAttribute("type", "text");
a.setAttribute("id", "paster");

let b = document.createElement("button")
b.setAttribute("id", "pasteclick")

let c = document.createElement("button")
c.setAttribute("id", "inputEnter")

extras.appendChild(a)
extras.appendChild(b)
extras.appendChild(c)
document.body.appendChild(extras)

}

//display
var SH_OutputOld = "";
var SH_Output = "";
var wconsole = "";

//hacky code to get paste working
document.addEventListener('paste', function (e) {
    let paste = (e.clipboardData || window.clipboardData).getData("text");
    out = document.getElementById("paster");
    out.value = paste;
    document.getElementById("pasteclick").click();
});

var bashBool = false;

//preloaded commands
let loadedCOMS = {
	debug_wbsh:function(_input) {
		if (_input[1] === "shout") {
			wbsh.print("DEBUG > SH_OUT: \n╟────────────────────────╢\n" + SH_Output + "\n╟────────────────────────╢\n")
			console.log("DEBUG > SH_OUT:", SH_Output);
		}
		wbsh.returnsh()
	},
    ver:function(_input) {
        wbsh.print("WebBrew Milestone 1:006");
        wbsh.returnsh();
    },
    mount:function(_input) {
        brewDisk.loadDisk(_input[1]);
        wbsh.print("&c 0,255,0 c&[DiskMan]&c 255,255,255 c& Mounted disk " + _input[1])
        wbsh.returnsh();
    },
    unmount:function(_input) {
        brewDisk.closeDisk(_input[1]);
        wbsh.print("&c 0,255,0 c&[DiskMan]&c 255,255,255 c& Unmounted disk " + _input[1])
        wbsh.returnsh();
    },
    mkdir:function(_input) {
        let path = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    path = path + _input[i];
                } else {
                    path = path + " " + _input[i];
                }
            }
        }
        brewDisk.info.sh.mkdirp(path, function(err){
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			} else {
				wbsh.print("&c 0,0,255 c&[FileMan]&c 255,255,255 c& Made new directory: " + brewDisk.info.sh.pwd() + path)
				wbsh.returnsh();
			}
		})

    },
    change:function(_input) {
        brewDisk.changeDisk(_input[1]);
        wbsh.print("&c 0,255,0 c&[DiskMan]&c 255,255,255 c& Changed disk to " + _input[1])
        wbsh.returnsh();
    },
    ls:function(_input) {
		if (_input[1] === undefined) {
			_input[1] = ""
		}
		brewDisk.info.sh.ls(brewDisk.info.sh.pwd() + _input[1], function(err, data){
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			} else {
				console.log(data);
    			wbsh.print(" Current Volume: " + brewDisk.info.currentDiskName);
        		wbsh.print(" Directory of " + brewDisk.info.sh.pwd() + _input[1] + "\n");
				for (let i = 0; i < data.length; i++) {
					if (data[i].type === "DIRECTORY") {
						wbsh.print(" <&c 0,0,255 c&DIR&c 255,255,255 c&> &c 0,191,255 c&" + data[i].name)
					} else {
						wbsh.print("       " + data[i].name)
					}
				}
				
			}
			print("")
			wbsh.returnsh();
		})

        // let list = brewDisk.listDir();
        // console.log(list);
        // wbsh.print("Current Disk: " + brewDisk.info.currentDiskName + "\n");
        // wbsh.print("\nDirectory of " + brewDisk.info.sh.pwd());
        // for(let i in list) {
        //     if(list[i].split(".").length === 1) {
        //         wbsh.print("&c 0,0,255 c&" + list[i] + "&c 255,255,255 c&")
        //     }else {
        //         wbsh.print(list[i]);
        //     }
        // }
    },
    disks:function(_input) {
        let list = brewDisk.info.disksName;
        wbsh.print(" Current Disk: " + brewDisk.info.currentDiskName);
        wbsh.print(" Disk Listing\n")
        if (list.length !== 0) {
            for(let i in list) {
                console.log(list[i]);
                wbsh.print(list[i]);
            }
        }
        wbsh.print("");
        wbsh.returnsh();
    },
    //clear:function(_input) {
    //    SH_Output = "";
    //    SH_OutputOld = ""
    //    wbsh.returnsh();
    //},
    loadcom:function(_input) {
        let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }

        brewDisk.info.currentDisk.readFile(brewDisk.info.sh.pwd() + filename, "utf8", function(err, data) {
            let comjson = data
            let com = JSON.parse(comjson)

            for (let i = 0; i < com.length; i++) {
                brewDisk.info.currentDisk.readFile(brewDisk.info.sh.pwd() + com[i].Command, "utf8", function(err, data) {
                    console.log(com[i].CommandName);
                    console.log(data);
                    eval("loadedCOMS." + com[i].CommandName + " = " + "function(_input) {" + data + "}");
					brewDisk.info.currentDisk.readFile(brewDisk.info.sh.pwd() + com[i].man, "utf8", function(err, data) {
						console.log(data);
						eval("loadedMANS." + com[i].CommandName + " = " + data);
					})
                })
            }

            wbsh.returnsh()
        });
    },
    post:function(_input) {
        let input = document.createElement("input");
        input.setAttribute("type", "file");
        input.setAttribute("id", "filein")
        input.setAttribute("multiple", "true")
        extras.appendChild(input);
        input.click();
        function getPromiseFromEvent(item, event) {
            return new Promise((resolve) => {
                const listener = () => {
                    item.removeEventListener(event, listener);
                    resolve();
                }
                item.addEventListener(event, listener);
            })
        }
        input.addEventListener("cancel", (evt) => {
            wbsh.print("&c 135,206,235 c&[POST]&c 255,255,255 c& No file uploaded");
            extras.removeChild(filein);
            wbsh.returnsh();
        });
        (async function() {
            await getPromiseFromEvent(input, "change");
            let number;
            for (let i = 0; i < input.files.length; i++) {
                number = +i + +1;
                wbsh.print("&c 135,206,235 c&[POST]&c 255,255,255 c& Uploading: " + brewDisk.info.sh.pwd() + "/" + input.files[i].name + "[" + number + "/" + input.files.length + "]");
                postFiles(input.files[i]);
                wbsh.print("&c 135,206,235 c&[POST]&c 255,255,255 c& File uploaded: " + brewDisk.info.sh.pwd() + "/" + input.files[i].name + "[" + number + "/" + input.files.length + "]");
            
            }

            wbsh.print("&c 135,206,235 c&[POST]&c 255,255,255 c& All files uploaded");
            extras.removeChild(filein);
            
            
            wbsh.returnsh();

        })();
        function postFiles(_file) {
            let fileReader = new FileReader();
            fileReader.onload = function(e) {
                let buffer = Filer.Buffer.from(fileReader.result);
                brewDisk.info.currentDisk.writeFile(brewDisk.info.sh.pwd() + "/" + _file.name, buffer)
            }
            fileReader.readAsArrayBuffer(_file)
        }
    },
    get:function(_input) {

        let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }

        brewDisk.info.currentDisk.readFile(brewDisk.info.sh.pwd() + filename, function(err, data) {
            let blob = new Blob([data]);
            console.log(blob);
            let fileReader = new FileReader();
            fileReader.onload = function(e) {
                wbsh.print("&c 255,255,0 c&[GET]&c 255,255,255 c& Downloading: " + filename);
                download(fileReader.result, filename)
                wbsh.print("&c 255,255,0 c&[GET]&c 255,255,255 c& Downloaded: " + filename);
                console.log(fileReader.result); 
                wbsh.returnsh();
            }
            fileReader.readAsDataURL(blob);
        });
    },
    cd:function(_input) {
        let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }

		brewDisk.info.sh.cd(filename, function(err){
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			} else {
				prefix = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.sh.pwd() + "]$ ";
			}
			wbsh.returnsh();
		})
        
    },
    format:function(_input) {
        if (_input.length > 1) {
            wbsh.print("&c 255,0,0 c&[Disk Format]&c 255,255,255 c& Formatting: " + _input[1])
            wbsh.print("&c 255,0,0 c&[Disk Format]&c 255,255,255 c& Formatted: " + _input[1])
            brewDisk.changeDisk(_input[1])
            brewDisk.formatDisk(_input[1]);
            
        }else {
            wbsh.print("&c 255,0,0 c&[Disk Format]&c 255,255,255 c& disk not included")
        }
        wbsh.returnsh();
    },
    rm:function(_input) {
        let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }

		brewDisk.info.sh.rm(filename, function(err){
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			} else {
				wbsh.print("&c 0,0,255 c&[FileMan]&c 255,255,255 c& Removed File: " + brewDisk.info.sh.pwd() + filename);
			}
			wbsh.returnsh()
		})

        /*if (_input.length > 1) {
            brewDisk.info.currentDisk.unlink(brewDisk.info.sh.pwd() + filename, function(err) {
                if(err) wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&");
                wbsh.print("&c 0,0,255 c&[FileMan]&c 255,255,255 c& Removed File: " + brewDisk.info.sh.pwd() + filename);
                
                wbsh.returnsh();
            });
        } else {
            wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + "No file included" + "&c 255,255,255 c&");
            wbsh.returnsh();
        }*/
    },
	rmdir:function(_input){
		brewDisk.info.sh.rm(filename, {recursive: true}, function(err){
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			} else {
				wbsh.print("&c 0,0,255 c&[FileMan]&c 255,255,255 c& Removed File: " + brewDisk.info.sh.pwd() + filename);
			}
			wbsh.returnsh()
		})
	},
    mv:function(_input) {
        if (_input.length > 2) {
			let path1
			let path2
			
			if (_input[1].charAt(0) === "/") {
				path1 = _input[1]
			} else {
				path1 = brewDisk.info.sh.pwd() + _input[1]
			}

			if (_input[2].charAt(0) === "/") {
				path2 = _input[2]
			} else {
				path2 = brewDisk.info.sh.pwd() + _input[2]
			}

            brewDisk.info.currentDisk.rename(path1, path2, function(err) {
                if (err) {
					wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
				} else {
					wbsh.print("&c 0,0,255 c&[FileMan]&c 255,255,255 c& File " + brewDisk.info.sh.pwd() + _input[1] + "moved to " + brewDisk.info.sh.pwd() + _input[2]);
					wbsh.returnsh();	
				}
			});
        } else {
            wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + "Incomplete command" + "&c 255,255,255 c&");
            wbsh.returnsh();
        }
    },
    rjs:function(_input) {
        brewDisk.info.currentDisk.readFile(brewDisk.info.sh.pwd() + _input[1], "utf8", function(err, data) {
            eval(data);
            wbsh.returnsh();
        })
    },
    wget:function(_input) {
        
        function reqListener() {
            startget();
        }
        
        function reqError(err) {
            console.log('Fetch Error :-S', err);
            wbsh.print('Fetch Error :-S' + JSON.stringify(err));
            wbsh.returnsh()
        }
        
        var oReq = new XMLHttpRequest();
        oReq.onload = reqListener;
        oReq.onerror = reqError;
        oReq.open('get', _input[2], true);
        oReq.responseType = "arraybuffer";
        oReq.send();

        function startget() {

            wbsh.print("&c 135,206,235 c&[WGET]&c 255,255,255 c& Uploading File");
            const byteArray = new Uint8Array(oReq.response);
            brewDisk.info.currentDisk.writeFile(brewDisk.info.sh.pwd() + _input[1], oReq.response)
            wbsh.print("&c 135,206,235 c&[WGET]&c 255,255,255 c& File uploaded");
            wbsh.print("&c 135,206,235 c&[WGET]&c 255,255,255 c& All files uploaded");
            
            wbsh.returnsh();
        };
    },
	pwd:function(_input) {
		wbsh.print(brewDisk.info.sh.pwd())
		wbsh.returnsh()
	},
	touch:function(_input) {
        let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }

		brewDisk.info.sh.touch(filename, function(err) {
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			}
			wbsh.returnsh()
		})
	},
	cat:function(_input) {
		let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0) {
                if (i === 1) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }	
		
		brewDisk.info.sh.cat(filename, function(err, data) {
			if (err) {
				wbsh.print("&c 139,0,0 c&[ERROR] &c 255,255,255 c&&c 255,0,0 c&" + err + "&c 255,255,255 c&")
			} else {
				print("Contents of " + filename + ": \n")
				print(data)
			}
			wbsh.returnsh()
		})
	},
	man:function(_input) {
		if (_input[1] === undefined || _input[1] === "") {
			print("All Manual Pages: \n\n &c 30,144,255 c&Category &c 255,215,0 c&► &c 255,255,255 c&COMMAND - Short Description\n")

			let sortedKeys = Object.keys(loadedMANS).sort();
			console.log(sortedKeys);
			for (let i = 0; i < Object.keys(loadedMANS).length; i++) {
				print(" &c 30,144,255 c&" + eval("loadedMANS." + sortedKeys[i] + ".category") + " &c 255,215,0 c&► &c 255,255,255 c&" + eval("loadedMANS." + sortedKeys[i] + ".title"))
			}
			wbsh.returnsh()

		} else if (!eval("loadedMANS." + _input[1])) {
			wbsh.print("Manual page for \"" + _input[1] + "\" does not exist!")
			wbsh.returnsh()
		} else {
			SH_Output = "&c 255,255,255 c&";
			SH_OutputOld = "";

			print(" \n MANUAL PAGE FOR \"" + _input[1] + "\": (q to exit)\n\n ╓─────────────────────────")
			print(" ║ " + eval("loadedMANS." + _input[1] + ".category") + " ► " + eval("loadedMANS." + _input[1] + ".title") + ": \n ║ ")
			print(" ║ SYNTAX: " + eval("loadedMANS." + _input[1] + ".command") + "\n ╙─────────────────────────")
			print(" DESCRIPTION: \n " + eval("loadedMANS." + _input[1] + ".body") + "\n")

			less = true
			reloadFrame()
		}        
	},
	reboot:function(_input) {
		document.location.reload()
	},
	setenv:function(_input) {
		let filename = "";
        for (let i = 0; i < _input.length; i++) {
            if (i !== 0 && i !== 1) {
                if (i === 2) {
                    filename = filename + _input[i];
                } else {
                    filename = filename + " " + _input[i];
                }
            }
        }	

		localStorage.setItem(_input[1], filename)
		wbsh.returnsh()
	},
	getenv:function(_input) {
		print(localStorage.getItem(_input[1]))
		wbsh.returnsh()
	}
}

let MANcategories = [
	"MISC",
	"FILE MANAGEMENT",
	"BETA DEBUG",
	"DISK MANAGEMENT",
	"BINARIES",
	"FILE EDITOR"
]

let loadedMANS = {
	ver:
	{
		title:"VER - Version",
		category:"MISC",
		command:"ver",
		body:"Simple command that prints WebBrew version."
	},
	cat:
	{
		title:"CAT - Print File",
		category:"FILE MANAGEMENT",
		command:"cat <FILENAME>",
		body:"Prints out the contents of <filename>."
	},
	cd:
	{
		title:"CD - Change Directory",
		category:"FILE MANAGEMENT",
		command:"cd <DIRECTORY>",
		body:"Changes your current working directory to <DIRECTORY>. \n\n To go one directory up, do \"cd ..\"\n To go to the root directory, do \"cd /\"\n CD supports both relative (dir/) and absolute (/tmp/dir/) paths."
	},
	change:
	{
		title:"CHANGE - Change Disk",
		category:"DISK MANAGEMENT",
		command:"change <DISK>",
		body:"Changes your current disk to <DISK>."
	},
	//clear:
	//{
	//	title:"CLEAR",
	//	category:"MISC",
	//	command:"clear",
	//	body:"Clears terminal screen."
	//},
	debug_wbsh:
	{
		title:"debug_wbsh - Debug Commands",
		category:"BETA DEBUG",
		command:"debug_wbsh <COMMAND>",
		body:"NOTE TO DEVS: REMOVE THIS BEFORE PUBLIC RELEASE (NOT BETA RELEASE)\n\n Runs <COMMAND> from the list of debug commands.\n\n DEBUG COMMANDS: \n shout: prints SH_Output variable"
	},
	disks:
	{
		title:"DISKS - List Disks",
		category:"DISK MANAGEMENT",
		command:"disks",
		body:"Shows all currently mounted disks.\n Use change <DISK> to open a disk."
	},
	format:
	{
		title:"FORMAT - Format Disk",
		category:"DISK MANAGEMENT",
		command:"format <DISK>",
		body:"&c 255,0,0 c&*WARNING* DESTRUCTIVE OPERATION! USE AT YOUR OWN RISK!&c 255,255,255 c&\n Formats (clears) a disk.\n This will remove all files and folders from a disk.\n Make sure you're deleting the correct disk to prevent data loss."
	},
	get:
	{
		title:"GET - Download File",
		category:"FILE MANAGEMENT",
		command:"get <FILENAME>",
		body:"Downloads <FILENAME> from your WebBrew disk to your hard drive."
	},
	loadcom:
	{
		title:"LOADCOM - Load Command",
		category:"MISC",
		command:"loadcom <FILENAME>",
		body:"Loads commands from <FILENAME>."
	},
	ls:
	{
		title:"LS - List Directory",
		category:"FILE MANAGEMENT",
		command:"ls",
		body:"Shows all files and directories in the current working directory."
	},
	man:
	{
		title:"MAN - Command Manual",
		category:"MISC",
		command:"man <COMMAND>",
		body:"The WebBrew Command Manual provides manual pages for every loaded command.\n These pages provide helpful information on how to use a command and its syntax\n\n Running \"man <COMMAND>\" will show <COMMAND>'s manual page.\n Running \"man\" by itself will give you a directory of categories."
	},
	mkdir:
	{
		title:"MKDIR - Make Directory",
		category:"FILE MANAGEMENT",
		command:"mkdir <NAME>",
		body:"Makes a directory (folder) called <NAME> in the current working directory"
	},
	mount:
	{
		title:"MOUNT - Mount Disk",
		category:"DISK MANAGEMENT",
		command:"mount <DISK>",
		body:"Mounts the provided disk.\n If the disk does not exist, it will create it.\n To change into a mounted disk, use \"change <DISK>\"."
	},
	mv:
	{
		title:"MV - Move File",
		category:"FILE MANAGEMENT",
		command:"mv <FILE1> <FILE2>",
		body:"Moves <FILE1> to the new location of <FILE2>."
	},
	post:
	{
		title:"POST - Upload File",
		category:"FILE MANAGEMENT",
		command:"post",
		body:"Upload a file from your hard drive to the current working directory."
	},
	pwd:
	{
		title:"PWD - Print Working Directory",
		category:"MISC",
		command:"pwd",
		body:"Prints your current working directory (current directory your terminal is on)."
	},
	rjs:
	{
		title:"RJS - Execute JS File",
		category:"BINARIES",
		command:"rjs <JS-FILE>",
		body:"Runs <JS-FILE>"
	},
	rm:
	{
		title:"RM - Remove File/Directory",
		category:"FILE MANAGEMENT",
		command:"rm <PATH>",
		body:"Removes the file or directory <PATH>.\n If directory provided has files or directories inside, you wont be able to remove it.\n \"rmdir <PATH>\" does'nt have this restriction."
	},
	rmdir:
	{
		title:"RMDIR - Remove Recursive Directory",
		category:"FILE MANAGEMENT",
		command:"rmdir <PATH>",
		body:"Removes the directory <PATH>.\n Can remove directories with files or directories inside."
	},
	touch:
	{
		title:"TOUCH - Create Empty File",
		category:"FILE MANAGEMENT",
		command:"touch <FILE>",
		body:"Creates an empty file at <FILE>"
	},
	unmount:
	{
		title:"UNMOUNT - Unmount Disk",
		category:"DISK MANAGEMENT",
		command:"unmount <DISK>",
		body:"Unmounts <ejects/closes> the provided disk. Remount it with \"mount <DISK>\""
	},
	wget:
	{
		title:"WGET - Upload File From URL",
		category:"FILE MANAGEMENT",
		command:"wget <URL>",
		body:"Uploads a file from provided url to the current working directory."
	},
	reboot:
	{
		title:"REBOOT - Reload Terminal",
		category:"MISC",
		command:"reboot",
		body:"Reloads and reboots the terminal (sends you back to your bootloader)."
	},
}

//library
var wbsh = {
    print:function(_text) {
        SH_Output = SH_Output + "&c 255,255,255 c&" + _text + "\n";
		console.log("\u001b[32m" + _text);
    },
    returnsh:function() {      
        
    },
    run:function(_command){
        
        let command = _command.split(" ");

        if (command[0] !== "") {
            if (loadedCOMS.hasOwnProperty(command[0]) === true) {
                eval("loadedCOMS." + command[0] + "(" + JSON.stringify(command) + ")");
            }else {
                print('Command "' + command[0] + '" not found or loaded')
                this.returnsh();
            }
        } else {
            this.returnsh();
        }   

    }
};

console.log("WBSH (Webbrew shell) LOADED");
console.log("WebBrew Milestone 1:006");