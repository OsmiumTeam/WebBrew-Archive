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
    let fileReader = new FileReader();
    fileReader.onload = function(e) {
        
        var win = window.open("about:blank", "_blank", "popup");
    	win.document.title = "WebBrew Image Viewer";
    	let favicon = win.document.createElement("link");
		favicon.setAttribute("rel", "icon");
        favicon.setAttribute("href", "about:blank");
	    favicon.setAttribute("id", "favicon");
        win.document.getElementsByTagName('head')[0].appendChild(favicon);
        let img = document.createElement("img");
        img.setAttribute("src", fileReader.result);
        win.document.getElementsByTagName('body')[0].appendChild(img);
        bash();
    }
    fileReader.readAsDataURL(blob);
});