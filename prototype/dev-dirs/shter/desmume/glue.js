//var Module = {
//onRuntimeInitialized: function() {



window.onload = function() {
    brewDisk.loadDisk("root")
    brewDisk.changeDisk("root")
    brewDisk.changeDir("desmume")
    
    brewDisk.info.currentDisk.readFile(brewDisk.info.currentDir + "rom.nds", function(err, data) {
        let blob = new Blob([data]);
        let fileReader = new FileReader();
        fileReader.onload = function(e) {
            document.getElementById("player").loadURL(fileReader.result);
        }
        fileReader.readAsDataURL(blob);
    });
}



//}
//};