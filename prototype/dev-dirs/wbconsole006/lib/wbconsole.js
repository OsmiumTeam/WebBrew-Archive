let extras = document.getElementById("extra");
let wconsole = document.getElementById("output");
let bin;

function print(_text) {
    wconsole.innerHTML = wconsole.innerHTML + _text + "<br>";
}

let loadedCOMS = {
    ver:function(_input) {
        print("WebBrew Pre-Milestone 1:006");
    },
    mount:function(_input) {
        brewDisk.loadDisk(_input[1]);
        print("<span style='color:lime'>[DiskMan]</span> Mounted disk as " + _input[1])
    },
    unmount:function(_input) {
        brewDisk.closeDisk(_input[1]);
        print("<span style='color:lime'>[DiskMan]</span> Unmounted disk " + _input[1])
    },
    mkdir:function(_input) {
        brewDisk.makeDir(_input[1]);
        brewDisk.refreshList();
        print("<span style='color:blue'>[FileMan]</span> Made new directory: " + brewDisk.info.currentDir + _input[1])
    },
    change:function(_input) {
        brewDisk.changeDisk(_input[1]);
        brewDisk.refreshList();
        print("<span style='color:lime'>[DiskMan]</span> Changed disk to " + _input[1])
    },
    ls:function(_input) {
        brewDisk.refreshList();
        let list = brewDisk.listDir();
        console.log(list);
        print("Disk: " + brewDisk.info.currentDiskName + "<br><br>Directory of " + brewDisk.info.currentDir + "<br>");
        for(let i in list) {
            if(list[i].split(".").length === 1) {
                print("<span style='color:blue'>" + list[i] + "</span>")
            }else {
                print(list[i]);
            }
        }
        print("");
    },
    disks:function(_input) {
        let list = brewDisk.info.disksName;
        print("Current Disk: " + brewDisk.info.currentDiskName + "<br><br>Disk Listing<br>");
        for(let i in list) {
            print(list[i]);
        }
        print("");
    },
    clear:function(_input) {
        wconsole.innerHTML = "";
    },
    loadcom:function(_input) {
        //brewDisk.info.currentDisk.
    },
    post:function(_input) {
        let input = document.createElement("input");
        input.setAttribute("type", "file");
        input.setAttribute("id", "filein")
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
            print("<span style='color:skyblue'>[POST]</span> No file uploaded");
            extras.removeChild(filein);
            document.getElementById("prefix").style.display = "initial";
            document.getElementById("input").style.display = "inline-block";
        });
        (async function() {
            let fileReader = new FileReader();
            document.getElementById("prefix").style.display = "none";
            document.getElementById("input").style.display = "none";
            await getPromiseFromEvent(input, "change");
            print("<span style='color:skyblue'>[POST]</span> Uploading: " + brewDisk.info.currentDir + input.files[0].name + "[1/" + input.files.length + "]");
            fileReader.onload = function(e) {
                let buffer = Filer.Buffer.from(fileReader.result);
                brewDisk.info.currentDisk.writeFile(brewDisk.info.currentDir + input.files[0].name, buffer)
                console.log(fileReader.result);
                print("<span style='color:skyblue'>[POST]</span> File uploaded: " + brewDisk.info.currentDir + input.files[0].name + "[1/" + input.files.length + "]");
                print("<span style='color:skyblue'>[POST]</span> All files uploaded");
                console.log(URL.createObjectURL(input.files[0]));
                extras.removeChild(filein);
                document.getElementById("prefix").style.display = "initial";
                document.getElementById("input").style.display = "inline-block";
                brewDisk.refreshList();
                brewDisk.refreshList();
                wconsole.click();
            }
            fileReader.readAsArrayBuffer(input.files[0])
        })();
    },
    get:function(_input) {

        brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + _input[1], function(err, data) {
            document.getElementById("prefix").style.display = "none";
            document.getElementById("input").style.display = "none";
            //let blob = new Blob(data, { type: eval("mime." + _input[1].split(".")[_input[1].split(".").length - 1]) });
            let blob = new Blob([data]);
            console.log(blob);
            let fileReader = new FileReader();
            fileReader.onload = function(e) {
                print("<span style='color:yellow'>[GET]</span> Downloading: " + _input[1]);
                download(fileReader.result, _input[1])
                print("<span style='color:yellow'>[GET]</span> Downloaded: " + _input[1]);
                console.log(fileReader.result); 
                document.getElementById("prefix").style.display = "initial";
                document.getElementById("input").style.display = "inline-block";
            }
            fileReader.readAsDataURL(blob);
        });
    },
    cd:function(_input) {
        brewDisk.info.currentDisk.exists(brewDisk.info.currentDir + _input[1], function (exists) {
            if (exists) {
                if (_input[1].charAt(0) === "/") {
                    brewDisk.goto(_input[1]);
                }else {
                    brewDisk.changeDir(_input[1]);
                }
                brewDisk.refreshList();
                document.getElementById("prefix").innerHTML = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ ";
            }
        })
        
    },
    format:function(_input) {
        if (_input.length > 1) {
            print("<span style='color:red'>[Disk Format]</span> Formatting: " + _input[1])
            print("<span style='color:red'>[Disk Format]</span> Formatted: " + _input[1])
            brewDisk.changeDisk(_input[1])
            brewDisk.formatDisk(_input[1]);
            brewDisk.refreshList();
        }else {
            print("<span style='color:red'>[Disk Format]</span> disk not included")
        }
    },
    rm:function(_input) {
        if (_input.length > 1) {
            brewDisk.info.currentDisk.unlink(brewDisk.info.currentDir + _input[1], function(err) {
                if(err) print("<span style='color:darkred'>[ERROR] </span><span style='color:red'>" + err + "</span>");
                print("<span style='color:blue'>[FileMan]</span> Removed File: " + brewDisk.info.currentDir + _input[1]);
                brewDisk.refreshList();
            });
        } else {
            print("<span style='color:darkred'>[ERROR] </span><span style='color:red'>" + "No file included" + "</span>");
        }
    },
    mv:function(_input) {
        if (_input.length > 2) {
            brewDisk.info.currentDisk.rename(brewDisk.info.currentDir + _input[1], brewDisk.info.currentDir + _input[2], function(err) {
                if(err) print("<span style='color:darkred'>[ERROR] </span><span style='color:red'>" + err + "</span>");
                print("<span style='color:blue'>[FileMan]</span> File " + brewDisk.info.currentDir + _input[1] + "is now " + brewDisk.info.currentDir + _input[2]);
                brewDisk.refreshList();
            });
        } else {
            print("<span style='color:darkred'>[ERROR] </span><span style='color:red'>" + "Incomplete command" + "</span>");
        }
    }
    //rmdir:function(_input) {
    //    brewDisk.info.currentDisk.readdir(brewDisk.info.currentDir + _input[1], function(err, files) {if(err){console.log(err);}dirListing = files;});
    //}
}



loadedCOMS.mount(["", "root"]);
loadedCOMS.change(["", "root"]);

document.getElementById("prefix").innerHTML = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ ";

document.getElementById("input").addEventListener("keydown", function (e) {
    if (e.code === "Enter") { 
        console.log("blj"); 
        //let command = e.target.value;
        let command = e.target.value.split(" ");
        console.log(command);
        wconsole.innerHTML = wconsole.innerHTML + "<span style='color:green'>" + "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ " + "</span>" + e.target.value + "<br>";
        document.getElementById("prefix").innerHTML = "";
        document.getElementById("input").value = "";
        document.getElementById("input").setAttribute("disabled", "true");
        if (command[0] !== "") {
            if (loadedCOMS.hasOwnProperty(command[0]) === true) {
                eval("loadedCOMS." + command[0] + "(" + JSON.stringify(command) + ")");
            }else {
                wconsole.innerHTML = wconsole.innerHTML + 'Command "' + command[0] + '" not found or loaded' + "<br>";
            }
        }
        document.getElementById("input").removeAttribute("disabled");
        document.getElementById("prefix").innerHTML = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ ";
        wconsole.click();
    }
});