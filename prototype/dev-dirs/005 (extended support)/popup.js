const popup = {

	open:function(url, width, height, title, icon="about:blank") {
		windows.titles.push(title)
		windows.maximized.push(false)
		windows.minimized.push(false)
		
		let windowdiv = document.createElement("div")
		windowdiv.setAttribute("id", "window" + parseInt(parseInt(windows.titles.length) - 1))
		windowdiv.setAttribute("class", "window")
		let titlebar = document.createElement("div")
		titlebar.setAttribute("class", "title-bar")
		let titletext = document.createElement("div")
		titletext.setAttribute("class", "title-bar-text")
		let controlsdiv = document.createElement("div")
		controlsdiv.setAttribute("class", "title-bar-controls")
		let minimize = document.createElement("button")
		minimize.setAttribute("aria-label", "Minimize")
		let maximize = document.createElement("button")
		maximize.setAttribute("aria-label", "Maximize")
		let close = document.createElement("button")
		close.setAttribute("aria-label", "Close")
		let bodydiv = document.createElement("div")
		bodydiv.setAttribute("id", "bodydiv" + parseInt(parseInt(windows.titles.length) - 1))
		bodydiv.setAttribute("class", "window-body")

		titletext.innerHTML = "<img src='" + icon + "' width='16px' height='16px'> " + title;
		//bodydiv.style.margin = '0';
		bodydiv.style.height = '100vh';
		bodydiv.innerHTML = "<iframe style='border:none;width:100%;height:100%;margin:0;' src='" + url + "' id='iframe" + parseInt(parseInt(windows.titles.length) - 1) + "'>"
		bodydiv.setAttribute("style", "height:" + height + "px;")
		windowdiv.setAttribute("style", "width:" + width + "px;position: absolute;")
		//bodydiv.setAttribute("style", "position:relative")
		let currenti = parseInt(parseInt(windows.titles.length) - 1)
		close.onclick = function(){
			eval("document.getElementById('window" + currenti + "').remove();windows.titles.splice(" + currenti + ", 1);windows.maximized.splice(" + currenti + ", 1);windows.minimized.splice(" + currenti + ", 1);")
		}
		  
		maximize.onclick = function(){
			eval("if(!windows.maximized[" + currenti + "]) {document.getElementById('window" + currenti + "').setAttribute('style', 'display:block;position:absolute;top:0px;left:0px;width:100%;height:100%');document.getElementById('bodydiv" + currenti + "').setAttribute('style', 'top:33px;right:1px;left:1px;bottom:1px;padding:0px;position:absolute');windows.maximized[" + currenti + "] = true} else {document.getElementById('window" + currenti + "').setAttribute('style', 'width:" + width + "px;');document.getElementById('bodydiv" + currenti + "').setAttribute('style', 'height:" + height + "px;');windows.maximized[" + currenti + "] = false}")
		}

		windows.ontop = parseInt(parseInt(windows.titles.length) - 1)		

		controlsdiv.appendChild(minimize)
		controlsdiv.appendChild(maximize)
		controlsdiv.appendChild(close)
		titlebar.appendChild(titletext)
		titlebar.appendChild(controlsdiv)
		windowdiv.appendChild(titlebar)
		windowdiv.appendChild(bodydiv)
		document.body.appendChild(windowdiv)

		windowdiv.setAttribute("onmousedown", "document.getElementById('window' + windows.ontop).setAttribute('class', 'window');windows.ontop = " + currenti + ";document.getElementById('window' + windows.ontop).setAttribute('class', 'window active');document.getElementById('window' + windows.ontop).style.zIndex = parseInt(parseInt(windows.topindex) + 1);windows.topindex = parseInt(parseInt(windows.topindex) + 1);")
		windowdiv.setAttribute("onmouseup", "document.getElementById('iframe" + currenti +"').focus()")
		//drag code

		function onDrag({movementX, movementY}){
			let getStyle = window.getComputedStyle(windowdiv);
			let leftVal = parseInt(getStyle.left);
			let topVal = parseInt(getStyle.top);
			windowdiv.style.left = `${leftVal + movementX}px`;
			windowdiv.style.top = `${topVal + movementY}px`;
		}


		titlebar.addEventListener("mousedown", ()=>{
			window.addEventListener("mousemove", onDrag);
			document.getElementById("iframe" + currenti).style.display = "none"
		});
		document.addEventListener("mouseup", ()=>{
			window.removeEventListener("mousemove", onDrag);
			document.getElementById("iframe" + currenti).style.display = "block"
		});		

		windowdiv.dispatchEvent(new Event("mousedown"))
		windowdiv.dispatchEvent(new Event("mouseup"))

		windowdiv.style.position = "absolute"
		windowdiv.style.top = "100px"
		windowdiv.style.left = "100px"
	}
	
	/*aboutFile: 
		function(url, width, height, title, icon="about:blank") {
			var win = window.open("about:blank", "_blank", "popup,width=" + width + ",height=" + height);
			win.document.title = title;
			let favicon = win.document.createElement("link");
			favicon.setAttribute("rel", "icon");
			favicon.setAttribute("href", icon);
			favicon.setAttribute("id", "favicon");
			win.document.getElementsByTagName('head')[0].appendChild(favicon);
			let popupcheck = win.document.createElement("span")
			popupcheck.setAttribute("id", "popupcheck");
			win.document.getElementsByTagName('head')[0].appendChild(popupcheck);
			win.document.body.style.margin = '0';
			win.document.body.style.height = '100vh';
			var iframe = win.document.createElement('iframe');
			iframe.style.border = 'none';
			iframe.style.width = '100%';
			iframe.style.height = '100%';
			iframe.style.margin = '0';
			iframe.src = url;
			iframe.setAttribute("id", "frame")
			win.document.body.appendChild(iframe);
			iframe.focus()
		}*/

}

/*const windows = {
	titles:[],
	id:[]
}*/

const window_control = {

	title:function(_name) {
		let popupcheck = !!parent.document.getElementById("popupcheck");
		if (popupcheck === true) {
			parent.document.title = _name;
		} else {
			parent.window.titles[parent.window.id] = _name
		}
	}

}

console.log("[WBLIB] LOADED POPUP LIB");

// funny drag code

