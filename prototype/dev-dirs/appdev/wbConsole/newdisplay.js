let ahconvert = new AnsiUp();

function reloadFrame() {
    //console.log("frame reloading");
    wconsole = ""
    fText(SH_Output)
    document.getElementById("output").innerHTML = ahconvert.ansi_to_html(wconsole);
    //console.log("frame reloaded");
    window.location.hash = "extra";
    window.location.hash = "bottom";
}

window.onload = function() {
    reloadFrame()
}
window.onresize = function() {
    reloadFrame()
}

function fText(_text) {
    //console.log(_text);
    let colors = _text.split("&c ");
    //console.log(colors);
    let meta;
    let rgb;
    for (let i = 0; i < colors.length; i++) {
        meta = colors[i].split(" c&");

		//console.log(meta);
		rgb = meta[0].split(",");
		//console.log(rgba);
		wconsole = wconsole + "\u001b[38;2;255;255;255m";
		if (rgb.length === 3) {
			wconsole = wconsole + "\u001b[38;2;" + rgb[0] + ";" + rgb[1] + ";" + rgb[2] + "m";
		}
		//console.log(meta[1]);
		if (meta.length > 1) {
			text(meta[1]);			
		}
    }
}

/*function fbText(_text) {
    //console.log(_text);
    let colors = _text.split("&b ");
    //console.log(colors);
    let meta;
    let rgb;
    for (let i = 0; i < colors.length; i++) {
        meta = colors[i].split(" b&");

		//console.log(meta);
		rgb = meta[0].split(",");
		console.log(rgb.length);
		//console.log(rgba);
		wconsole = wconsole + "\u001b[48;2;0;0;0m";
		if (rgb.length === 3) {
			wconsole = wconsole + "\u001b[48;2;" + rgb[0] + ";" + rgb[1] + ";" + rgb[2] + "m";
		}
		//console.log(meta[1]);
		if (meta.length > 1) {
			fText(meta[1]);			
		}
    }
}*/

/*function nlText(_text) {
    let lines = _text.split("<br>")
    for (let i = 0; i < lines.length; i++) {
        text(lines[i]);
        if (i !== 0) {
            wconsole = wconsole + "<br>";
        }
    }
}*/

//function color(_ctx, _r, _g, _b, _a) {
//    _ctx.fillStyle = "rgba("+_r+","+_g+","+_b+","+(_a/255)+")";
//}

//function mkpixel(_ctx, _r, _g, _b, _a, _x, _y) {
//    _ctx.fillStyle = "rgba("+_r+","+_g+","+_b+","+(_a/255)+")";
//    _ctx.fillRect( _x, _y, 1, 1 );
//}

function text(_text) {
    for (let i = 0; i < _text.length; i++) {
        wconsole = wconsole + _text.charAt(i);
    }
}