let extras = document.getElementById("extra");
let bin;
let prefix;

function print(_text) {
    SH_Output = SH_Output + _text + "<br>";
}

loadedCOMS.mount(["", "root"]);
loadedCOMS.change(["", "root"]);

prefix = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ ";

document.getElementById("input").addEventListener("keydown", function (e) {
    if (e.code === "Enter") { 
        console.log("blj"); 
        //let command = e.target.value;
        let command = e.target.value.split(" ");
        console.log(command);
        SH_Output.innerHTML = SH_Output.innerHTML + "<color green/>" + "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ " + "<color 255,255,255/>" + e.target.value + "<br>";
        prefix = "";
        document.getElementById("input").value = "";
        document.getElementById("input").setAttribute("disabled", "true");
        if (command[0] !== "") {
            if (loadedCOMS.hasOwnProperty(command[0]) === true) {
                eval("loadedCOMS." + command[0] + "(" + JSON.stringify(command) + ")");
            }else {
                SH_Output.innerHTML = SH_Output.innerHTML + 'Command "' + command[0] + '" not found or loaded' + "<br>";
            }
        }
        document.getElementById("input").removeAttribute("disabled");
        prefix = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.currentDir + "]$ ";
        SH_Output.click();
    }
});