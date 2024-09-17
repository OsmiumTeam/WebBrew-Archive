function addApp() {
	if (localStorage.getItem("wblAppNum") === null) {
		localStorage.setItem("wblAppNum", 0);
	}
	localStorage.setItem("wbapp" + (parseInt(localStorage.getItem("wblAppNum")) + 1), document.getElementById("addID").value);
	localStorage.setItem("wblAppNum", parseInt(localStorage.getItem("wblAppNum")) + 1);
}

function reload() {
	rela = [];	
	for (let i = 1; i <= localStorage.getItem("wblAppNum"); i++) {
		rela.push(localStorage.getItem("wbapp" + i));
		let script = document.createElement("script");
		script.setAttribute("rel", "apps/" + localStorage.getItem("wbapp" + i) + "/wbmanifest.js");
		document.head.appendChild(script);
	}
	let part2 = document.createElement("script");
	part2.innerHTML = "reload2()";
	document.head.appendChild(part2);
}

function reload2() {
	
}