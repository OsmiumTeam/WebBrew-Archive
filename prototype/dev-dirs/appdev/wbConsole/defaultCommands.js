var bashq = false
function bash() {
    if (bashq) {
        libInput.input = "";
        libInput.cursor = 0;
        libInput.inputReturn = "";
        libInput.inputPrefix = "&c 0,128,0 c&[" + brewDisk.info.currentDiskName + " > " + brewDisk.info.sh.pwd() + "]#&c 255,255,255 c& "
        libInput.inputBool = true;
        bashBool = true;
        SH_OutputOld = SH_Output;
        SH_Output = SH_OutputOld + libInput.inputPrefix + libInput.input.slice(0, libInput.cursor) + "&c 128,128,128 c&_&c 255,255,255 c&" + libInput.input.slice(libInput.cursor);
        reloadFrame()
    }
}

document.getElementById("inputEnter").addEventListener("click", function() {
    if (bashBool) {
        bashBool = false;
        wbsh.run(libInput.input);
    }
});

wbsh.returnsh = function() {
    bash();
}