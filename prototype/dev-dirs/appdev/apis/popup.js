const popup = {

	aboutFile: 
		function(url, width, height, title, icon="about:blank") {
			//if (!!parent.document.getElementById("popupcheck") === true) {
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
			//} else if (!!parent.document.getElementById("xcheck") === true) {
			//	parent.popup.open(url, width, height, title, icon)
			//}

		},
	file:
		function(url, width, height) {
			var win = window.open(url, "_blank", "popup,width=" + width + ",height=" + height);
		}

}

/*const window = {
	titles:[],
	id:[]
}*/

/*const window_control = {

	title:function(_name) {
		let popupcheck = !!parent.document.getElementById("popupcheck");
		if (popupcheck === true) {
			parent.document.title = _name;
		} else {
			parent.window.titles[parent.window.id] = _name
		}
	}

}*/