var bashBool = false;
var compass;

let loadedCOMS = {
    ver:function(_input) {
        print("WebBrew Pre-Milestone 1:006");
        bash();
    },
    mount:function(_input) {
        brewDisk.loadDisk(_input[1]);
        print("<color 0,255,0/>[DiskMan]<color 255,255,255/> Mounted disk " + _input[1])
        bash();
    },
    unmount:function(_input) {
        brewDisk.closeDisk(_input[1]);
        print("<color 0,255,0/>[DiskMan]<color 255,255,255/> Unmounted disk " + _input[1])
        bash();
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
        print("<color 0,0,255/>[FileMan]<color 255,255,255/> Made new directory: " + brewDisk.info.currentDir + filename)
        bash();
    },
    change:function(_input) {
        brewDisk.changeDisk(_input[1]);
        brewDisk.refreshList();
        print("<color 0,255,0/>[DiskMan]<color 255,255,255/> Changed disk to " + _input[1])
        bash();
    },
    ls:function(_input) {
        brewDisk.refreshList();
        let list = brewDisk.listDir();
        console.log(list);
        print("Current Disk: " + brewDisk.info.currentDiskName + "<br>");
        print("<br>Directory of " + brewDisk.info.currentDir);
        for(let i in list) {
            if(list[i].split(".").length === 1) {
                print("<color 0,0,255/>" + list[i] + "<color 255,255,255/>")
            }else {
                print(list[i]);
            }
        }
        print("");
        bash();
    },
    disks:function(_input) {
        let list = brewDisk.info.disksName;
        print("Current Disk: " + brewDisk.info.currentDiskName);
        print("<br><br>Disk Listing")
        if (list.length !== 0) {
            for(let i in list) {
                console.log(list[i]);
                print(list[i]);
            }
        }
        print("");
        bash();
    },
    clear:function(_input) {
        SH_Output = "";
        SH_OutputOld = ""
        bash();
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

            bash()
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
            print("<color 135,206,235/>[POST]<color 255,255,255/> No file uploaded");
            extras.removeChild(filein);
            bash();
        });
        (async function() {
            await getPromiseFromEvent(input, "change");
            let number;
            for (let i = 0; i < input.files.length; i++) {
                number = +i + +1;
                print("<color 135,206,235/>[POST]<color 255,255,255/> Uploading: " + brewDisk.info.currentDir + input.files[i].name + "[" + number + "/" + input.files.length + "]");
                postFiles(input.files[i]);
                print("<color 135,206,235/>[POST]<color 255,255,255/> File uploaded: " + brewDisk.info.currentDir + input.files[i].name + "[" + number + "/" + input.files.length + "]");
            
            }

            print("<color 135,206,235/>[POST]<color 255,255,255/> All files uploaded");
            extras.removeChild(filein);
            brewDisk.refreshList();
            brewDisk.refreshList();
            bash();

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
                print("<color 255,255,0/>[GET]<color 255,255,255/> Downloading: " + filename);
                download(fileReader.result, filename)
                print("<color 255,255,0/>[GET]<color 255,255,255/> Downloaded: " + filename);
                console.log(fileReader.result); 
                bash();
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
            bash();
        })
        
        
    },
    format:function(_input) {
        if (_input.length > 1) {
            print("<color 255,0,0/>[Disk Format]<color 255,255,255/> Formatting: " + _input[1])
            print("<color 255,0,0/>[Disk Format]<color 255,255,255/> Formatted: " + _input[1])
            brewDisk.changeDisk(_input[1])
            brewDisk.formatDisk(_input[1]);
            brewDisk.refreshList();
        }else {
            print("<color 255,0,0/>[Disk Format]<color 255,255,255/> disk not included")
        }
        bash();
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
                if(err) print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + err + "<color 255,255,255/>");
                print("<color 0,0,255/>[FileMan]<color 255,255,255/> Removed File: " + brewDisk.info.currentDir + filename);
                brewDisk.refreshList();
                bash();
            });
        } else {
            print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + "No file included" + "<color 255,255,255/>");
            bash();
        }
    },
    mv:function(_input) {
        if (_input.length > 2) {
            brewDisk.info.currentDisk.rename(brewDisk.info.currentDir + _input[1], brewDisk.info.currentDir + _input[2], function(err) {
                if(err) print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + err + "<color 255,255,255/>");
                print("<color 0,0,255/>[FileMan]<color 255,255,255/> File " + brewDisk.info.currentDir + _input[1] + "is now " + brewDisk.info.currentDir + _input[2]);
                brewDisk.refreshList();
                bash();
            });
        } else {
            print("<color 139,0,0/>[ERROR] <color 255,255,255/><color 255,0,0/>" + "Incomplete command" + "<color 255,255,255/>");
            bash();
        }
    },
    rjs:function(_input) {
        brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + _input[1], "utf8", function(err, data) {
            eval(data);
            bash();
        })
    },
    wget:function(_input) {
        
        function reqListener() {
            startget();
        }
        
        function reqError(err) {
            console.log('Fetch Error :-S', err);
            print('Fetch Error :-S' + JSON.stringify(err));
            bash()
        }
        
        var oReq = new XMLHttpRequest();
        oReq.onload = reqListener;
        oReq.onerror = reqError;
        oReq.open('get', _input[2], true);
        oReq.responseType = "arraybuffer";
        oReq.send();

        function startget() {

            print("<color 135,206,235/>[WGET]<color 255,255,255/> Uploading File");
            const byteArray = new Uint8Array(oReq.response);
            brewDisk.info.currentDisk.writeFile(brewDisk.info.currentDir + _input[1], oReq.response)
            print("<color 135,206,235/>[WGET]<color 255,255,255/> File uploaded");
            print("<color 135,206,235/>[WGET]<color 255,255,255/> All files uploaded");
            brewDisk.refreshList();
            brewDisk.refreshList();
            bash();

        };
    },
}

function bash() {
    libInput.input = "";
    libInput.cursor = 0;
    libInput.inputReturn = "";
    libInput.inputPrefix = "<color 0,128,0/>[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$<color 255,255,255/> "
    libInput.inputBool = true;
    bashBool = true;
    SH_OutputOld = SH_Output;
    SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "<color 128,128,128/>_<color 255,255,255/>" + libInput.input.slice(libInput.cursor);
    reloadFrame()
}

document.getElementById("inputEnter").addEventListener("click", function() {
    //console.log("YEAH");
    if (bashBool) {
        bashBool = false;
        runBashCOM(libInput.input);
    }
});

function runBashCOM(_command) {
    let command = _command.split(" ");
    //SH_Output = SH_Output + libInput.inputPrefix + _command + "<br>";
    //console.log("COMMAND LOG: ");
    //console.log(_command);
    //console.log(command);
    //console.log("END CMD LOG");

    if (command[0] !== "") {
        if (loadedCOMS.hasOwnProperty(command[0]) === true) {
            eval("loadedCOMS." + command[0] + "(" + JSON.stringify(command) + ")");
        }else {
            print('Command "' + command[0] + '" not found or loaded')
            //SH_Output = SH_Output + 'Command "' + command[0] + '" not found or loaded' + "<br>";
            bash();
        }
    } else {
        bash();
    }
}