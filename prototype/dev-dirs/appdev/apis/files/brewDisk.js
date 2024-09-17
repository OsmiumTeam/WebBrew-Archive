const brewDisk = {

    info:{
        currentDisk:NaN,
		sh:NaN,
        currentDir:NaN,
        currentDiskName:NaN,

        disks:{},
		shells:{},
        disksName:[]
    },

    loadDisk:function(_name) {
		let fs = new Filer.FileSystem({name: _name})
		eval("this.info.disks." + _name + ' =  fs');
		let shell = new fs.Shell()
		eval("this.info.shells." + _name + ' = shell');
        this.info.disksName.push(_name);
    },
    changeDisk:function(_name) {
        this.info.currentDisk = eval("this.info.disks." + _name);
        this.info.currentDir  = "/";
        this.info.currentDiskName = _name;
		this.info.sh = eval("this.info.shells." + _name);
    },
    closeDisk:function(_name) {
        eval("delete this.info.disks." + _name);
		eval("delete this.info.shells." + _name);
        this.info.currentDir = NaN;
        this.info.currentDisk = NaN;
        this.info.currentDiskName = NaN;
		this.info.sh = NaN;
        this.info.disksName.splice(this.info.disksName.indexOf(_name), 1)
    },
    formatDisk:function(_name) {
        this.closeDisk(_name);
		let fs = new Filer.FileSystem({name: _name,flags: ["FORMAT"]})
        eval("this.info.disks." + _name + ' = fs');
		eval("this.info.shells." + _name + ' = new fs.Shell()');
        this.changeDisk(_name);
    }
};