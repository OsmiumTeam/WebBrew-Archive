let dirListing = ["not loaded"];

const brewDisk = {

    info:{
        currentDisk:NaN,
        currentDir:NaN,
        currentDiskName:NaN,

        disks:{},
        disksName:[]
    },

    loadDisk:function(_name) {
        eval("this.info.disks." + _name + ' = new Filer.FileSystem({name: _name})');
        this.info.disksName.push(_name);
    },
    changeDisk:function(_name) {
        this.info.currentDisk = eval("this.info.disks." + _name);
        this.info.currentDir  = "/";
        this.info.currentDiskName = _name;
    },
    makeDir:function(_name) {
        this.info.currentDisk.mkdir(this.info.currentDir + _name, function(err) {if(err){console.log(err);}});
        console.log(dirListing);
    },
    listDir:function() {
        let output = dirListing;
        return output;
    },
    refreshList:function() {
        this.info.currentDisk.readdir(this.info.currentDir, function(err, files) {if(err){console.log(err);}dirListing = files;});
    },
    removeDir:function(_name) {
        this.info.currentDisk.rmdir(_name, function(err) {if(err){console.log(err);}})
    },
    changeDir:function(_name) {
        if (_name.charAt(_name.length - 1) === "/") {
            this.info.currentDir = this.info.currentDir + _name;
        }else {
            this.info.currentDir = this.info.currentDir + _name + "/";
        }
    },
    goto:function(_name) {
        if (_name.charAt(_name.length - 1) === "/") {
            this.info.currentDir = _name;
        }else {
            this.info.currentDir = _name + "/";
        }
    },
    closeDisk:function(_name) {
        eval("delete this.info.disks." + _name);
        this.info.currentDir = NaN;
        this.info.currentDisk = NaN;
        this.info.currentDiskName = NaN;
        dirListing = ["not loaded"]
        this.info.disksName.splice(this.info.disksName.indexOf(_name), 1)
    },
    formatDisk:function(_name) {
        this.closeDisk(_name);
        eval("this.info.disks." + _name + ' = new Filer.FileSystem({name: _name,flags: ["FORMAT"]})');  
        this.changeDisk(_name);
    }
};