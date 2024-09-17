let searchparam = new URLSearchParams(new URL(window.location.href).search)
let queriecore = "";
let corelistthing = document.getElementById("newcorelist")
let selcore = ""
//load disks
brewDisk.loadDisk(webretroconf.romdisk);
if (webretroconf.romdisk !== webretroconf.coredisk) {
    brewDisk.loadDisk(webretroconf.coredisk);
}
if (webretroconf.romdisk !== webretroconf.retroarchdisk && webretroconf.coredisk !== webretroconf.retroarchdisk) {
    brewDisk.loadDisk(webretroconf.retroarchdisk);
}

if (searchparam.has("coredir")) {
    document.getElementById("coreselect").setAttribute("style", "display:none")
    queriecore = "mgba";
} else {

brewDisk.changeDisk(webretroconf.coredisk)

brewDisk.info.currentDisk.readdir(webretroconf.coredir, function(err, files) {
    if (err) {
        console.log(err);
        corelistthing.innerHTML = corelistthing.innerHTML + "<button>" + "No Cores" + "</button>"
    } else {
        
        if (files.length !== 0) {
            for (let i = 0; i < files.length; i++) {

                corelistthing.innerHTML = corelistthing.innerHTML + "<a href='" + window.location.href + "?coredir=" + files[i] + "' id='" + files[i] + "_a" + "'></a>" + "<button onclick='" + "document.getElementById(&#39;" + files[i] + '_a' + "&#39;).click()" + "'>" + files[i] + "</button>"
            } 
        } else {
            corelistthing.innerHTML = corelistthing.innerHTML + "<button>" + "No Cores" + "</button>"
        }
    }
});
}