var SH_OutputOld = "";
var SH_Output = "";

document.addEventListener('paste', function (e) {
    //clipdata = e.clipboardData || window.clipboardData;
    let paste = (e.clipboardData || window.clipboardData).getData("text");
    //console.log(paste);
    out = document.getElementById("paster");
    out.value = paste;
    document.getElementById("pasteclick").click();
});

let display_canvas = document.getElementById("display");
let html = document.getElementById("html");
display_canvas.setAttribute("width", window.innerWidth)
display_canvas.setAttribute("height", html.clientHeight)

let display = display_canvas.getContext("2d");

window.addEventListener("resize", function() {
    let display_canvas = document.getElementById("display");
    let html = document.getElementById("html");
    display_canvas.setAttribute("width", window.innerWidth)
    display_canvas.setAttribute("height", html.clientHeight)
    reloadFrame();
})

//mkpixel(display, 255, 0, 0, 255, 0, 0)

function reloadFrame() {
    console.log("frame reloading");
    display.clearRect(0, 0, display_canvas.width, display_canvas.height);
    fText(display, SH_Output, 0, 0, 14, 0)

    let i = 0
    while(i<(html.clientHeight/14)) {
        mkpixel(display, 255, 0, 0, 255, 0, i*14)
        i++
    }

    console.log("frame reloaded");  
}

window.onload = function() {
    reloadFrame()
}

function fText(_ctx, _text, _x, _y, _fontSize, _fontSpacing) {
    let colors = _text.split("<color ");
    //console.log(colors);
    let meta;
    let rgba;
    let outarr;
    let __x = _x;
    //console.log(_y);
    let __y = _y + _fontSize;
    for (let i = 0; i < colors.length; i++) {
        meta = colors[i].split("/>");
        //console.log(meta);
        rgba = meta[0].split(",");
        //console.log(rgba);
        if (rgba.length > 3) {
            color(_ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
        } else if (rgba.length > 2) {
            color(_ctx, rgba[0], rgba[1], rgba[2], 255);
        }
        //console.log(meta[1]);
        outarr = nlText(_ctx, meta[1], __x, __y, _fontSize, _fontSpacing);
        __x = outarr[0];
        __y = outarr[1];
        //console.log(__y, " <-> ", __x);
    }
}

function nlText(_ctx, _text, _x, _y, _fontSize, _fontSpacing) {
    let lines = _text.split("<br>")
    let trueSize = _fontSize + _fontSpacing;
    let __x = _x;
    let outX;
    let out = [50, 50];
    for (let i = 0; i < lines.length; i++) {
        if (lines[i] !== "") {
            outX = text(_ctx, lines[i], __x, _y + (i*trueSize));
        } else {
            outX = text(_ctx, "░", __x, _y + (i*trueSize));
        }
        __x = 0;
        console.log("i: " + i);
    }
    console.log(_y, " : ", lines.length, " <- ", lines);
    console.log("x", outX, "lines: ", lines);
    if (lines.length > 1) {
        out = [outX, (((_fontSize/2) * lines.length) + (_fontSpacing * lines.length)) + _y];
    } else {
        out = [outX, _y];
    }
    console.log(out);
    return out;
}

function color(_ctx, _r, _g, _b, _a) {
    _ctx.fillStyle = "rgba("+_r+","+_g+","+_b+","+(_a/255)+")";
}

function mkpixel(_ctx, _r, _g, _b, _a, _x, _y) {
    _ctx.fillStyle = "rgba("+_r+","+_g+","+_b+","+(_a/255)+")";
    _ctx.fillRect( _x, _y, 1, 1 );
}
function text(_ctx, _text, _x, _y) {
    _ctx.font = "14px ibm";
    for (let i = 0; i < _text.length; i++) {
        _ctx.fillText(_text.charAt(i), _x + ((14*i)), _y);
    }
    //console.log("_text: ", _text.length);
    return _x + (14 * _text.length)
}