let wasmfileurl = "NaN";
let filename = "desmume.wasm";
brewDisk.loadDisk("test");
brewDisk.changeDisk("test");
brewDisk.changeDir("bin/wasm/");

brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + filename, function(err, data) {
    let blob = new Blob([data]);
    let fileReader = new FileReader();
    fileReader.onload = function(e) {
        wasmfileurl = fileReader.result;

        script1 = document.createElement("script");
        script1.setAttribute("src", "desmume.js");
        document.getElementById("reqs").appendChild(script1);

    }
    fileReader.readAsDataURL(blob);
});

var Module = {
    locateFile: function(s) {
        return wasmfileurl;
    }
};