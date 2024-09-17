var libInput = {
    inputBool:false,
    input:"",
    cursor:0,
    inputReturn:"",
    inputPrefix:""
}

function print(_text) {
    SH_Output = SH_Output + _text + "\n";
	console.log("\u001b[32m" + _text);
    reloadFrame();
}

function loadPaste() {
    let paste = document.getElementById("paster").value;
    console.log("paste: ", paste);
    libInput.input = libInput.input.slice(0, libInput.cursor) + paste + libInput.input.slice(libInput.cursor);
    if (libInput.cursor !== libInput.input.length) {
        libInput.cursor = libInput.cursor + paste.length;
    }
    SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + libInput.input.slice(libInput.cursor);
    reloadFrame();
}

document.getElementById('pasteclick').onclick = function(){loadPaste()}

let extrakeys = ['Escape', 'CapsLock', 'Tab', 'Backspace', 'Enter', 'ArrowUp', 'ArrowLeft', 'ArrowDown', 'ArrowRight', 'Delete', 'End', 'Home', 'Insert', 'PageDown', 'PageUp', 'ScrollLock', 'Pause'];
let ctrlDown = false;
document.addEventListener("keyup", function (e) {
    if (e.key === "Control" || e.key === "Meta") ctrlDown = false;
})

document.addEventListener("keydown", function (e) {
    //console.log(e.location);
    if (e.key === "Control" || e.key === "Meta") ctrlDown = true;

    if ((e.location === 0 || e.location === 3) && !ctrlDown && libInput.inputBool) { 
        if (extrakeys.indexOf(e.key) !== -1) {
            
            if (e.key === "Backspace") {
                if (libInput.cursor !== 0) {
                    libInput.input = libInput.input.slice(0, libInput.cursor - 1) + libInput.input.slice(libInput.cursor, libInput.input.length)
                    libInput.cursor = libInput.cursor - 1;
                }
                SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + libInput.input.slice(libInput.cursor);
                reloadFrame();
            }
            if (e.key === "ArrowLeft") {
                if (libInput.cursor !== 0) {
                    libInput.cursor = libInput.cursor - 1;
                }
                SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + libInput.input.slice(libInput.cursor);
                reloadFrame();
            }
            if (e.key === "ArrowRight") {
                if (libInput.cursor !== libInput.input.length) {
                    libInput.cursor = libInput.cursor + 1;
                }
                SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + libInput.input.slice(libInput.cursor);
                reloadFrame();
            }
            if (e.key === "Enter") {
                SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input + "\n";
                SH_OutputOld = SH_Output;
                libInput.inputBool = false;
                reloadFrame();
                document.getElementById("inputEnter").click();
            }

        } else {
            libInput.input = libInput.input.slice(0, libInput.cursor) + e.key + libInput.input.slice(libInput.cursor);
            if (libInput.cursor !== libInput.input.length) {
                libInput.cursor = libInput.cursor + 1;
            }
            SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + libInput.input.slice(libInput.cursor);
            reloadFrame();
        }
        
        
        /*
        console.log("blj"); 
        //let command = e.target.value;
        let command = e.target.value.split(" ");
        console.log(command);
        SH_Output.innerHTML = SH_Output.innerHTML + "&c green c&" + "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.sh.pwd() + "]$ " + "&c 255,255,255 c&" + e.target.value + "\n";
        libInput.inputPrefix = "";
        document.getElementById("input").value = "";
        document.getElementById("input").setAttribute("disabled", "true");
        if (command[0] !== "") {
            if (loadedCOMS.hasOwnProperty(command[0]) === true) {
                eval("loadedCOMS." + command[0] + "(" + JSON.stringify(command) + ")");
            }else {
                SH_Output.innerHTML = SH_Output.innerHTML + 'Command "' + command[0] + '" not found or loaded' + "\n";
            }
        }
        document.getElementById("input").removeAttribute("disabled");
        libInput.inputPrefix = "[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.sh.pwd() + "]$ ";
        SH_Output.click();
        */
    } else {
        //if (e.key === "Backspace") {
        //    input = "";
        //    libInput.cursor = 0;
        //    SH_Output = SH_OutputOld + libInput.inputPrefix + input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + input.slice(libInput.cursor);
        //    reloadFrame();
        //    console.log(SH_Output);
        //    console.log(SH_OutputOld);
        //}
    }
});