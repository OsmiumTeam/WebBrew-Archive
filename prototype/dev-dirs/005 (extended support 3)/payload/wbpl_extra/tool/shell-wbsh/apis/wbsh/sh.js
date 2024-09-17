
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
var compass;

//preloaded commands
let loadedCOMS = {
    ver:function(_input) {
        wbsh.print("WebBrew Pre-Milestone 1:006");
        wbsh.returnsh();
    },
    mount:function(_input) {
        brewDisk.loadDisk(_input[1]);
        wbsh.print("<color 0,255,0/>[DiskMan]<color 255,255,255/> Mounted disk " + _input[1])
        wbsh.returnsh();
    },
    unmount:function(_input) {
        brewDisk.closeDisk(_input[1]);
        wbsh.print("<color 0,255,0/>[DiskMan]<color 255,255,255/> Unmounted disk " + _input[1])
        wbsh.returnsh();
    },
    mkdir:function(_input) {
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
        brewDisk.makeDir(filename);
        brewDisk.refreshList();
        wbsh.print("<color 0,0,255/>[FileMan]<color 255,255,255/> Made new directory: " + brewDisk.info.currentDir + filename)
        wbsh.returnsh();
    },
    change:function(_input) {
        brewDisk.changeDisk(_input[1]);
        brewDisk.refreshList();
        wbsh.print("<color 0,255,0/>[DiskMan]<color 255,255,255/> Changed disk to " + _input[1])
        wbsh.returnsh();
    },
    ls:function(_input) {
        brewDisk.refreshList();
        let list = brewDisk.listDir();
        console.log(list);
        wbsh.print("Current Disk: " + brewDisk.info.currentDiskName + "<br>");
        wbsh.print("<br>Directory of " + brewDisk.info.currentDir);
        for(let i in list) {
            if(list[i].split(".").length === 1) {
                wbsh.print("<color 0,0,255/>" + list[i] + "<color 255,255,255/>")
            }else {
                wbsh.print(list[i]);
            }
        }
        wbsh.print("");
        wbsh.returnsh();
    },
    disks:function(_input) {
        let list = brewDisk.info.disksName;
        wbsh.print("Current Disk: " + brewDisk.info.currentDiskName);
        wbsh.print("<br><br>Disk Listing")
        if (list.length !== 0) {
            for(let i in list) {
                console.log(list[i]);
                wbsh.print(list[i]);
            }
        }
        wbsh.print("");
        wbsh.returnsh();
    },
    clear:function(_input) {
        SH_Output = "";
        SH_OutputOld = ""
        wbsh.returnsh();
    },
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

        brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + filename, "utf8", function(err, data) {
            let comjson = data
            let com = JSON.parse(comjson)

            for (let i = 0; i < com.length; i++) {
                compass = com[i];
                brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + com[i].Command, "utf8", function(err, data) {
                    console.log(compass.CommandName);
                    console.log(data);
                    eval("loadedCOMS." + compass.CommandName + " = " + "function(_input) {" + data + "}");
                })

                //this.comname = function(_input) {command}
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
            wbsh.print("<color 135,206,235/>[POST]<color 255,255,255/> No file uploaded");
            extras.removeChild(filein);
            wbsh.returnsh();
        });
        (async function() {
            await getPromiseFromEvent(input, "change");
            let number;
            for (let i = 0; i < input.files.length; i++) {
                number = +i + +1;
                wbsh.print("<color 135,206,235/>[POST]<color 255,255,255/> Uploading: " + brewDisk.info.currentDir + input.files[i].name + "[" + number + "/" + input.files.length + "]");
                postFiles(input.files[i]);
                wbsh.print("<color 135,206,235/>[POST]<color 255,255,255/> File uploaded: " + brewDisk.info.currentDir + input.files[i].name + "[" + number + "/" + input.files.length + "]");
            
            }

            wbsh.print("<color 135,206,235/>[POST]<color 255,255,255/> All files uploaded");
            extras.removeChild(filein);
            brewDisk.refreshList();
            brewDisk.refreshList();
            wbsh.returnsh();

        })();
        function postFiles(_file) {
            let fileReader = new FileReader();
            fileReader.onload = function(e) {
                let buffer = Filer.Buffer.from(fileReader.result);
                brewDisk.info.currentDisk.writeFile(brewDisk.info.currentDir + _file.name, buffer)
                
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

        brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + filename, function(err, data) {
            let blob = new Blob([data]);
            console.log(blob);
            let fileReader = new FileReader();
            fileReader.onload = function(e) {
                wbsh.print("<color 255,255,0/>[GET]<color 255,255,255/> Downloading: " + filename);
                download(fileReader.result, filename)
                wbsh.print("<color 255,255,0/>[GET]<color 255,255,255/> Downloaded: " + filename);
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

        brewDisk.info.currentDisk.exists(brewDisk.info.currentDir + filename, function (exists) {
            if (exists) {
                if (filename.charAt(0) === "/") {
                    brewDisk.goto(filename);
                }else {
                    brewDisk.changeDir(filename);
                }
                brewDisk.refreshList();
                prefix = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ ";
            }
            wbsh.returnsh();
        })
        
        
    },
    format:function(_input) {
        if (_input.length > 1) {
            wbsh.print("<color 255,0,0/>[Disk Format]<color 255,255,255/> Formatting: " + _input[1])
            wbsh.print("<color 255,0,0/>[Disk Format]<color 255,255,255/> Formatted: " + _input[1])
            brewDisk.changeDisk(_input[1])
            brewDisk.formatDisk(_input[1]);
            brewDisk.refreshList();
        }else {
            wbsh.print("<color 255,0,0/>[Disk Format]<color 255,255,255/> disk not included")
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

        if (_input.length > 1) {
            brewDisk.info.currentDisk.unlink(brewDisk.info.currentDir + filename, function(err) {
                if(err) wbsh.print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + err + "<color 255,255,255/>");
                wbsh.print("<color 0,0,255/>[FileMan]<color 255,255,255/> Removed File: " + brewDisk.info.currentDir + filename);
                brewDisk.refreshList();
                wbsh.returnsh();
            });
        } else {
            wbsh.print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + "No file included" + "<color 255,255,255/>");
            wbsh.returnsh();
        }
    },
    mv:function(_input) {
        if (_input.length > 2) {
            brewDisk.info.currentDisk.rename(brewDisk.info.currentDir + _input[1], brewDisk.info.currentDir + _input[2], function(err) {
                if(err) wbsh.print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + err + "<color 255,255,255/>");
                wbsh.print("<color 0,0,255/>[FileMan]<color 255,255,255/> File " + brewDisk.info.currentDir + _input[1] + "is now " + brewDisk.info.currentDir + _input[2]);
                brewDisk.refreshList();
                wbsh.returnsh();
            });
        } else {
            wbsh.print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + "Incomplete command" + "<color 255,255,255/>");
            wbsh.returnsh();
        }
    },
    rjs:function(_input) {
        brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + _input[1], "utf8", function(err, data) {
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

            wbsh.print("<color 135,206,235/>[WGET]<color 255,255,255/> Uploading File");
            const byteArray = new Uint8Array(oReq.response);
            brewDisk.info.currentDisk.writeFile(brewDisk.info.currentDir + _input[1], oReq.response)
            wbsh.print("<color 135,206,235/>[WGET]<color 255,255,255/> File uploaded");
            wbsh.print("<color 135,206,235/>[WGET]<color 255,255,255/> All files uploaded");
            brewDisk.refreshList();
            brewDisk.refreshList();
            wbsh.returnsh();

        };
    },
}

//library
var wbsh = {
    print:function(_text) {
        SH_Output = SH_Output + "<color 255,255,255/>" + _text + "<br>";
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