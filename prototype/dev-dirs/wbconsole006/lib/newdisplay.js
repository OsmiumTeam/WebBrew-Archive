var SH_OutputOld = "";
var SH_Output = "";
var wconsole = "";

document.addEventListener('paste', function (e) {
    //clipdata = e.clipboardData || window.clipboardData;
    let paste = (e.clipboardData || window.clipboardData).getData("text");
    //console.log(paste);
    out = document.getElementById("paster");
    out.value = paste;
    document.getElementById("pasteclick").click();
});

let html = document.getElementById("html");

//mkpixel(display, 255, 0, 0, 255, 0, 0)

function reloadFrame() {
    //console.log("frame reloading");
    wconsole = ""
    fText("white/>" + SH_Output)
    document.getElementById("output").innerHTML = "<span>" + wconsole + "<span>";
    //console.log("frame reloaded");
    window.location.hash = "extra";  
    window.location.hash = "bottom";  
}

window.onload = function() {
    reloadFrame()
}

function fText(_text) {
    //console.log(_text);
    let colors = _text.split("<color ");
    //console.log(colors);
    let meta;
    let rgba;
    for (let i = 0; i < colors.length; i++) {
        meta = colors[i].split("/>");
        //console.log(meta);
        rgba = meta[0].split(",");
        //console.log(rgba);
        wconsole = wconsole + "</span>";
        if (rgba.length > 3) {
            wconsole = wconsole + "<span style='color:rgba(" + rgba[0] + "," + rgba[1] + "," + rgba[2] + "," + (rgba[3]/255) + "'>"
        } else if (rgba.length > 2) {
            wconsole = wconsole + "<span style='color:rgb(" + rgba[0] + "," + rgba[1] + "," + rgba[2] + "'>"
        } else {
            wconsole = wconsole + "<span style='color:" + rgba[0] + "'>"
        }
        //console.log(meta[1]);
        nlText(meta[1]);
    }
}

function nlText(_text) {
    let lines = _text.split("<br>")
    for (let i = 0; i < lines.length; i++) {
        text(lines[i]);
        if (i !== 0) {
            wconsole = wconsole + "<br>";
        }
    }
}

//function color(_ctx, _r, _g, _b, _a) {
//    _ctx.fillStyle = "rgba("+_r+","+_g+","+_b+","+(_a/255)+")";
//}

//function mkpixel(_ctx, _r, _g, _b, _a, _x, _y) {
//    _ctx.fillStyle = "rgba("+_r+","+_g+","+_b+","+(_a/255)+")";
//    _ctx.fillRect( _x, _y, 1, 1 );
//}

function text(_text) {
    for (let i = 0; i < _text.length; i++) {
        wconsole = wconsole + "<span>" + _text.charAt(i) + "</span>";
    }
}