const wb = {
	
	popup: {
		aboutFile: 
			function(url, width, height, title, icon="about:blank") {
				var win = window.open("about:blank", "_blank");
		    	win.document.title = title;
		    	let favicon = win.document.createElement("link");
		    	favicon.setAttribute("rel", "icon");
	            favicon.setAttribute("href", icon);
	            favicon.setAttribute("id", "favicon");
	            win.document.getElementsByTagName('head')[0].appendChild(favicon);
	            win.document.body.style.margin = '0';
	            win.document.body.style.height = '100vh';
	            var iframe = win.document.createElement('iframe');
	            iframe.style.border = 'none';
	            iframe.style.width = '100%';
	            iframe.style.height = '100%';
	            iframe.style.margin = '0';
	            iframe.src = url;
	            win.document.body.appendChild(iframe);
			},
		file:
			function(url, width, height) {
				var win = window.open(url, "_blank");
			}}
}