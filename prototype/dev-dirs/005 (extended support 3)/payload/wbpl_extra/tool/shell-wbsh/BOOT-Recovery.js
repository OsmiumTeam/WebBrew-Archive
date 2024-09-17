/*
	~~~~~~~~~~~~~~~~~~~~~~~~~~
	WebBrew Firmware v1.1
	Recovery Edition
	
	<C> 2023-2024 Osmium Team
	~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

let rcy_bootjs = async function(){

    print(" &c 0,0,255 c& \n ░░░░░░░░░░░░░░&c 255,255,255 c&v1.1")
    print(" &c 0,0,255 c&░░░&c 255,255,255 c&WEBBREW&c 0,0,255 c&░░░░░░░░")
    print(" &c 0,0,255 c&░░░░░&c 255,255,255 c&FIRMWARE&c 0,0,255 c&░░░░░")
    print(" &c 0,0,255 c&░░░░░░░░░░░░░░░░░░")
    print(" &c 0,0,255 c&░&c 255,255,255 c&▓▓&c 0,0,255 c&░░&c 255,255,255 c&▓▓&c 0,0,255 c&░░&c 255,255,255 c&▓▓&c 0,0,255 c&░░&c 255,255,255 c&▓▓&c 0,0,255 c&░░&c 255,255,255 c&▓")
    print(" &c 255,255,255 c&▓&c 0,191,255 c&▒▒&c 255,255,255 c&▓▓&c 0,191,255 c&▒▒&c 255,255,255 c&▓▓&c 0,191,255 c&▒▒&c 255,255,255 c&▓▓&c 0,191,255 c&▒▒&c 255,255,255 c&▓▓&c 0,191,255 c&▒")
    print(" &c 0,191,255 c&▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒")
    print(" &c 0,191,255 c&▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒\n")

    print(" &c 255,255,255 c&WebBrew firmware \"BIOS\" Recovery Mode\n <C> 2023-2024 Osmium Dev Team (Originally WebBrew Team)\n")

	if ("hardwareConcurrency" in navigator) {
		cpuNum = JSON.stringify(navigator.hardwareConcurrency)
	} else {
		cpuNum = "Not Available"
	}

	if ("deviceMemory" in navigator) {
		ramNum = JSON.stringify(navigator.deviceMemory) + "GB"
	} else {
		ramNum = "Not Available"
	}

	if ("connection" in navigator) {
		if ("effectiveType" in navigator.connection) {
			conNum = navigator.connection.effectiveType
		} else {
			conNum = "Not Available"
		}
	} else {
		conNum = "Not Available"
	}

	if ("connection" in navigator) {
		if ("downlink" in navigator.connection) {
			if (navigator.connection.downlink === 10) {
				downNum = JSON.stringify(navigator.connection.downlink) + "+mbps"
			} else {
				downNum = JSON.stringify(navigator.connection.downlink) + "mbps"
			}
		} else {
			downNum = "Not Available"
		}
	} else {
		downNum = "Not Available"
	}

	if ("storage" in navigator) {
		storEst = await navigator.storage.estimate()
		storQuota = convert.convert(storEst.quota, "b").to("best")
		storUsed = convert.convert(storEst.usage, "b").to("best")

		storQuota = JSON.stringify(Math.round(storQuota.quantity)) + storQuota.unit
		storUsed = JSON.stringify(Math.round(storUsed.quantity)) + storUsed.unit

		storNum = storUsed + "/" + storQuota
	} else {
		storNum = "Not Available"
	}

	let uap = new UAParser();
	let UserAgentOBJ = uap.getResult();
	console.log(UserAgentOBJ);

    print(" ╔═════════════════════════════════╗")
    print(" ║          Device Specs:          ║")
    print(" ╟─────────────────────────────────╢")
    print(" ║ RAM: &c 150,150,150 c&" + ramNum + (" ".repeat((28 - ramNum.length) - 1)) + "&c 255,255,255 c&║")
    print(" ║ CPU cores: &c 150,150,150 c&" + cpuNum + (" ".repeat((22 - cpuNum.length) - 1)) + "&c 255,255,255 c&║")
    print(" ║ IDB Storage: &c 150,150,150 c&" + storNum + (" ".repeat((20 - storNum.length) - 1)) + "&c 255,255,255 c&║")
	print(" ╚═════════════════════════════════╝\n")

	print(" starting in 5 seconds...\n&c 255,255,255 c& Press b to attempt WebBrew 006 boot, s to view detailed system specs")
	
	let bootmode = "recovery"
	let bootcount = 0

	document.addEventListener("keydown", function (e) {          
		if (bootmode === "recovery") {
			if (e.key === "b") {
				bootmode = "normal"
			}
		}
		if (e.key === "s" && bootcount < 2) {
			if (bootmode === "stop" && bootcount > 0) {
				bootmode = "recovery"
				bootcount = 2
				boot()
			} else {
				bootmode = "stop"
			}
		}
	})
	
	function boot() {
		if (bootmode === "recovery"){
			SH_Output = ""
			wbsh.run("mount mbr")
			wbsh.run("change mbr")
			print("\nWebBrew Recovery SHell (WBRSH)")
			print("By the WebBrew Team")
			wbsh.print("\nUse \"man\" to show all commands")
			wbsh.print("Use \"man <COMMAND>\" to view manual page for a command\n")
			bashq = true;
			bash()
		} else if (bootmode === "stop") {
			SH_Output = "<br>"

			print(" &c 255,255,255 c& SYSTEM INFORMATION:\n")
		
			print(" ╔═══════════════════════════════════════╗")
			print(" ║              Device Specs:            ║")
			print(" ╟───────────────────────────────────────╢")
			print(" ║ RAM: &c 150,150,150 c&" + ramNum + (" ".repeat((34 - ramNum.length) - 1)) + "&c 255,255,255 c&║")
			print(" ║ CPU cores: &c 150,150,150 c&" + cpuNum + (" ".repeat((28 - cpuNum.length) - 1)) + "&c 255,255,255 c&║")
			print(" ║ IDB Storage: &c 150,150,150 c&" + storNum + (" ".repeat((26 - storNum.length) - 1)) + "&c 255,255,255 c&║")
			print(" ║ Aprox Internet Speed: &c 150,150,150 c&" + conNum + (" ".repeat((17 - conNum.length) - 1)) + "&c 255,255,255 c&║")
			print(" ║ Aprox Download Speed: &c 150,150,150 c&" + downNum + (" ".repeat((17 - downNum.length) - 1)) + "&c 255,255,255 c&║")
			print(" ╚═══════════════════════════════════════╝\n")

			print(" ╔═══════════════════════════════════════╗")
			print(" ║              User Agent:              ║")
			print(" ╟───────────────────────────────────────╢")
			print(" ║ Browser: &c 150,150,150 c&" + UserAgentOBJ.browser.name + "&c 255,255,255 c& | Version: &c 150,150,150 c&" + UserAgentOBJ.browser.major + (" ".repeat((18 - (UserAgentOBJ.browser.major.length + UserAgentOBJ.browser.name.length)) - 1)) + "&c 255,255,255 c&║")
			print(" ║ OS: &c 150,150,150 c&" + UserAgentOBJ.os.name + "&c 255,255,255 c& | Version: &c 150,150,150 c&" + UserAgentOBJ.os.version + (" ".repeat((23 - (UserAgentOBJ.os.version.length + UserAgentOBJ.os.name.length)) - 1)) + "&c 255,255,255 c&║")
			print(" ║ Engine: &c 150,150,150 c&" + UserAgentOBJ.engine.name + "&c 255,255,255 c& | Version: &c 150,150,150 c&" + UserAgentOBJ.engine.version + (" ".repeat((19 - (UserAgentOBJ.engine.version.length + UserAgentOBJ.engine.name.length)) - 1)) + "&c 255,255,255 c&║")
			print(" ║ CPU Architecture: &c 150,150,150 c&" + UserAgentOBJ.cpu.architecture + (" ".repeat((21 - UserAgentOBJ.cpu.architecture.length) - 1)) + "&c 255,255,255 c&║")
			print(" ╚═══════════════════════════════════════╝\n")

			print(" press s again to resume startup")
			bootcount = 1
		} else {
			SH_Output = ""
			brewDisk.loadDisk("mbr")
			brewDisk.changeDisk("mbr")
			brewDisk.info.currentDisk.exists("/init.js", function(exists){
			
				if (exists){
					wbsh.print("Detected WebBrew Version: v" + ("00" + localStorage.getItem("ENV_WBVER")).slice(-3))
					wbsh.print("booting webbrew...\n")
					sleep(500).then(() => {
						wbsh.run("mount " + localStorage.getItem("ENV_WBDISK"))
						wbsh.run("change " + localStorage.getItem("ENV_WBDISK"))
						reloadFrame();
						sleep(1000).then(() => {
							SH_Output = ""
							wbsh.print("WebBrew SHell (WBSH)")
							wbsh.print("By the WebBrew Team\n")
							wbsh.run("ver")
							wbsh.print("\nUse \"man\" to show all commands")
							wbsh.print("Use \"man <COMMAND>\" to view manual page for a command\n")
							bashq = true;
							bash()
						})
					})
				} else {
					print("&c 255,255,255 c& Cannot boot into the Master Boot Record (MBR)")
					print(" Rebooting in 3 seconds")
					sleep(3000).then(() => {
						if (bootmode === "install") {
							windows.top.popup.open("../wbInstaller/install.html", 867, 652, "WebBrew Installer")
							parent.close()
						} else {
							print("\n Rebooting NOW!!!")
							sleep(500).then(() => {document.location.reload()})
						}
					})
				}
			})
		}	
	}

	sleep(5000).then(() => {
		boot()
	});

	window.error_SH_Output = undefined
	window.errorOBJ = {}
	window.in_error = false

	window.addEventListener("keydown", function(e) {
		if (in_error) {
			if (e.key === "q") {
				wbsh.run("reboot")
			}
			if (e.key === "p") {
				let logString = ` [WebBrew Firmware] An error has occurred! The error is displayed below:\n\n ERROR!\n -------------\n\n message: ${errorOBJ.msg}\n source: ${errorOBJ.source}\n lineno: ${errorOBJ.lineno}\n colno: ${errorOBJ.colno}\n error: ${errorOBJ.err}\n\n END ERROR \n\n SH_Output DUMP: \n\n ---BEGIN DUMP---\n${error_SH_Output}\n ---END DUMP---\n\n SYSTEM SPECS (AS REPORTED IN THE "spec" COMMAND): \n\n ---BEGIN SPECS---\n ╔═══════════════════════════════════════╗\n ║              Device Specs:            ║\n ╟───────────────────────────────────────╢\n ║ RAM: ${ramNum + (" ".repeat((34 - ramNum.length) - 1))}║\n ║ CPU cores: ${cpuNum + (" ".repeat((28 - cpuNum.length) - 1))}║\n ║ IDB Storage: ${storNum + (" ".repeat((26 - storNum.length) - 1))}║\n ║ Aprox Internet Speed: ${conNum + (" ".repeat((17 - conNum.length) - 1))}║\n ║ Aprox Download Speed: ${downNum + (" ".repeat((17 - downNum.length) - 1))}║\n ╚═══════════════════════════════════════╝\n\n ╔═══════════════════════════════════════╗\n ║              User Agent:              ║\n ╟───────────────────────────────────────╢\n ║ Browser: ${UserAgentOBJ.browser.name} | Version: ${UserAgentOBJ.browser.major + (" ".repeat((18 - (UserAgentOBJ.browser.major.length + UserAgentOBJ.browser.name.length)) - 1))}║\n ║ OS: ${UserAgentOBJ.os.name} | Version: ${UserAgentOBJ.os.version + (" ".repeat((23 - (UserAgentOBJ.os.version.length + UserAgentOBJ.os.name.length)) - 1))}║\n ║ Engine: ${UserAgentOBJ.engine.name} | Version: ${UserAgentOBJ.engine.version + (" ".repeat((19 - (UserAgentOBJ.engine.version.length + UserAgentOBJ.engine.name.length)) - 1))}║\n ║ CPU Architecture: ${UserAgentOBJ.cpu.architecture + (" ".repeat((21 - UserAgentOBJ.cpu.architecture.length) - 1))}║\n ╚═══════════════════════════════════════╝\n\n ---END SPECS---`

				download("data:," + encodeURIComponent(logString), "log.txt")
			}
			if (e.key === "\\") {
				eval(prompt("Enter some javascript: "))
			}	
		}
	})

	window.onerror = async function(msg, source, lineno, colno, err) {
		in_error = true

		error_SH_Output = SH_Output

		SH_Output = "&c 255,255,255 c&";
		SH_OutputOld = "";

		errorOBJ.msg = msg
		errorOBJ.source = source
		errorOBJ.lineno = lineno
		errorOBJ.colno = colno
		errorOBJ.err = err
	
		let audio = new Audio(DEATH)
		audio.play()
	
		print(" [WebBrew Firmware] A FATAL bootloader error has occurred!\n The error is displayed below:\n")
		print(`&c 255,0,0 c& ERROR!\n -------------\n &c 255,255,255 c&`)
		print(` message: ${msg}`)
		print(` source: ${source}`)
		print(` lineno: ${lineno}`)
		print(` colno: ${colno}`)
		print(` error: ${err}`)
		print(`\n&c 255,0,0 c& END ERROR &c 255,255,255 c&\n`)

		//get specs

		if ("hardwareConcurrency" in navigator) {
			cpuNum = JSON.stringify(navigator.hardwareConcurrency)
		} else {
			cpuNum = "Not Available"
		}
	
		if ("deviceMemory" in navigator) {
			ramNum = JSON.stringify(navigator.deviceMemory) + "GB"
		} else {
			ramNum = "Not Available"
		}
	
		if ("connection" in navigator) {
			if ("effectiveType" in navigator.connection) {
				conNum = navigator.connection.effectiveType
			} else {
				conNum = "Not Available"
			}
		} else {
			conNum = "Not Available"
		}
	
		if ("connection" in navigator) {
			if ("downlink" in navigator.connection) {
				if (navigator.connection.downlink === 10) {
					downNum = JSON.stringify(navigator.connection.downlink) + "+mbps"
				} else {
					downNum = JSON.stringify(navigator.connection.downlink) + "mbps"
				}
			} else {
				downNum = "Not Available"
			}
		} else {
			downNum = "Not Available"
		}
	
		if ("storage" in navigator) {
			storEst = await navigator.storage.estimate()
			storQuota = convert.convert(storEst.quota, "b").to("best")
			storUsed = convert.convert(storEst.usage, "b").to("best")
	
			storQuota = JSON.stringify(Math.round(storQuota.quantity)) + storQuota.unit
			storUsed = JSON.stringify(Math.round(storUsed.quantity)) + storUsed.unit
	
			storNum = storUsed + "/" + storQuota
		} else {
			storNum = "Not Available"
		}
	
		let uap = new UAParser();
		let UserAgentOBJ = uap.getResult();
		console.log(UserAgentOBJ);
	
		print(" press q to restart, p to dump log into log.txt, \\ to run javascript")

		return true
	}

	//Begin Firmware sounds:

	DEATH = "data:audio/mpeg;base64,SUQzBAAAAAAAI1RTU0UAAAAPAAADTGF2ZjU4LjI5LjEwMAAAAAAAAAAAAAAA//NYwAAAAAAAAAAAAEluZm8AAAAPAAAAqQAAGHgACg0QERQXGRseICMmJyotMDI0Nzk8P0BDRkhLTVBSVVhZXF9hZGZoa25vcnV4en2AgYSHiIuOkJOWmZqdoKGkp6msr7Cztre6vcDCxcjJzM/Q09bY297h4uXo6ezv8fT3+Pv+AAAAAExhdmM1OC41NAAAAAAAAAAAAAAAACQDAAAAAAAAABh4ibgMEwAAAAAAAAAAAAAA//MYxAAE2Ab2WUEQApJcMA5QAHBAEHPLv1HPggZlAQ0qf/////MYxAMFsQsAAYEQAP53wfBt///v8HlOj/T53qf6fo+m/xIN//MYxAMF0j7kAcA4AP////+4b/////8bkU/////5D//6agww//MYxAIEya70BAAEHCf/////qb/////wf//y///01Rgwi3////MYxAUFkj7sBAFFEP//1BP/////Cv/////4T//6VcAAABAN//MYxAUFaAdCXhBEAgUUACP//kP/+pp3//1P//9aDHPub/9T//MYxAYFAAq0AUYQAH//rSCwjM//4FFSX//rVTH7X///2VwP//MYxAkHMdbQAYEoAP+///85xf//AhP///6CApP//9Zef/////MYxAMFsQsAAYEQAP6Po+r///7fBaXae/y/nOn6fo+mAgIa//MYxAMFASMgV8AQAqAf4P/////43//2//+h3//rwE/////8//MYxAYEij7sAAFFABF/////8KP/////49XJnf////84P/////MYxAoEoj7sAAHFEP//IP/////4hcBb/////gxX/////hX///MYxA4D+a7sAACLEP8uwl/////6gn/////hnf/+Ugow//////MYxBUD8a7wAAFFEP/8E///D3//20YB/////6C//////B////MYxBwDeSL4CgCHDPyyxT/////9Qb/////xF///lRMOUBPf//MYxCUD6a70AgAEHP/KlUf/9QNN//6gagYeihwAD1EvYrBD//MYxCwEAa7sAACLAJVtn///////////6wAMKGKAGAABgP6B//MYxDMEMJLcABgEHEAAAGMEgxgkb2+uTp/T///qd/qdyHuR//MYxDkAoAagAFjEAP/6agb6OQk+QM38tH/AgrMsDjukAhX4//MYxE0FABpMB1sAArHzP49Qt/TVgAgpADg35Ea0qbWv2d89//MYxFAJEBqmX5YUgv/p3LIf//6m7FfykP/1//yiBACCAA9P//MYxEIGyFJ4AZAQANL//f/cov/97f/+ceUX//z7f//+48oH//MYxD0H6CVYq4cQABfb////7lFxegyl////+ayABnKv//////MYxDQJYmacFYEoANTRMA95v////1NAgRO/////x0eCX//k//MYxCUJUj7UAcI4AOoQATBhk3////2dDx4eGzf////8VhxP//MYxBYIYs7cHAHVAv////QL3/////ypesBn/////4Mn//////MYxAsEqj7sAAFFAP4cb/////xVwv/////4Z/////4X//////MYxA8EQj7sAAFFGP/GJ/////zhG8C///Df//qeEgL//UBS//MYxBUDuSLsABAHKB//yIaCgV//1lm1GjRv////6nF/Z/xx//MYxB0EGAbIAUIQAM/R/////m1////+R8EPuP///r8J8zfn//MYxCMEsJ7IAYIQAP3/8/+Uv8M/yn1/Uv8Jp/////0Bf/////MYxCcG6dL0AYEQAP/xMd//9dXAP/////BC//////C///ll//MYxCIESbbwAcAoAMGN/////1D//////HS//////kXAVv////MYxCcD4a7sAACLEP//gyf////+EH/////8UsQf/////Dv///MYxC4Esj7oAAFPAP///xB/////8arAdP////+CL/////4Y//MYxDIEqj7sAAFFAG/////8XcWf////+gf/////4cb///////MYxDYEUj7oAAHFGMQqAEAECEAGBP/////6K//+3//5+hLW//MYxDsEqj7sAAFFADQBD3/////AUCkv//d//8O1GaoDARTP//MYxD8Eij7oAAHFEGKJPu//////6AChIBAN+uBAlgTn1aYf//MYxEMEoSMwfgCHDv8P1QSSSAuP////9BAUFP/b///EBT////MYxEcEmSLYABgKHAI3///+SBG/////5xRlF////4TJhY7K//MYxEsAWAq8ADDEAhn//+F/EPhQ78v4//yl/hT/wm3/jv/j//MYxGADQAX5SPDAAP/45RAAnAP8Fhpv////6HCoRTn///////MYxGoECBJmQVkQApo6Ljf////6HAHmnHf////yqBgBmC8G//MYxHAIYmrQK4EoALO/////uaRBH/////4UA7/////1ER////MYxGUIomLsAYEoAP//+VLVAgdA5wDArf////PMG4QFRK////MYxFkJes7gH8A4AP///Kf////+UEf/////1LLCJ/////+V//MYxEoIGs7UHAgUHAx/////4Yb/////wX//0cJ/////+gL///MYxEAIas8EXgFPAv///+Hd//5VXhxv////+MF//////CP///MYxDUFej7kAAHFEP8sEhr//iUQ//+WCgd//4dDf//lagLk//MYxDYD8a7sAAFFEEwPBAwQKsVWKN4r/FYflfSKYT0AwEUA//MYxD0EOa7sAAgKHAAAEAoFDkOREsjW1Nu///6b/5YWFRT///MYxEMEkAbUABBEAIsqQAaaTAxt////9J2ICf2///qf6in+//MYxEcDsC5AAKiEAFvy3//6ECf8mn///U9v/HHqR/in//////MYxE8BaBJoAVkAA/+or/////wf/////w/CLf////9QT/////MYxGAHCBaluYoAAP/8o//////glRABCGBn////88wfQM////MYxFoJUmbsy4E4AP///igv/////0C///SqFIFBAwXp//////MYxEsEuj70A8AQAP6iYS7/////4gb/////qBf//TXHN/////MYxE8Ekj7sAAFFEP/9Qb/////zDf////+JwGf/////g0////MYxFMGyj7kHAHFGv///Djf////+OrEP/////w7/////xP///MYxE4G2j7gHgHFGP////IVwg/////8EA/////+Uf/////x//MYxEkEgj7sAAFFEGpf////+34P//8j//62VRuVMDR//0BX//MYxE4Emj7sAAFFAP/8qR//9ZZcBr1COzAGIBwcz+K9TeKt//MYxFIEWj7oAAAOHNQuPoBcwYJAEKFBH1d+XjJKBAAAYAEg//MYxFcEmj7oAAHFALBQLQAGQzlf//7v//uUL/KWHy6/+Xzn//MYxFsDSSL0AAgHKP/9+7LHlEpWxyv////5MviEz7f//2J///MYxGQDwJLEADAEHBD/O/GP//zSZ38RTf//+rp/5Zv//9EQ//MYxGwEeBYIKPCEAAGYfzQBSP////9RqFQszf////4EBcb///MYxHEDoB4wAVwAA////ziQb/////8oRYBQBSwF4Kf////3//MYxHkJMA68/4UQAmLwZ/////4gb////o2rjUJP/////ExZ//MYxGsJOmLkAYI4ABQBQgF1AfT////+hGLhEu/////+PhQn//MYxF0I8s7UHcA4AP////nCL/////8owIX/////oG////////MYxFAIcs7kvggOHhW3/////jnAz/////8aDf////+Fv/////MYxEUIesrUHggUHP/+NcIt/////1BP/////KP/////4ZXC//MYxDoEuj7oAAFPAJ/////6Av/////hT///KsB//////GC///MYxD4Esj7oAAAKHP////4U3/////iqxzf////+G/////+Y//MYxEIEkj7sAAFFEG/////8asBn/////41Lf////4W3//////MYxEYD+a7sAAHLAP49wNb/////wZ/////4hf/////yCsIP//MYxE0Eoj7sAAFFGP////wQn/////yjr//5esD/////9Af///MYxFEEYj7sAAFFEP///8IP/////4hVfhzf////9Q3//8KK//MYxFYEqj7oAAFFGBQAGFAXHf////6Aw1v////8G7/////0//MYxFoEmj7kAAAOHFUYghhhf////p2JKHf////+Vf/////5//MYxF4EIbboAAHFAAUlAHF/////4UBf/////Yf/////6sAs//MYxGQEij7sAACLAO/////48Cd/////8UOqwDSP/////HwL//MYxGgC2SLsAAgOHDf////8VNWgBxf/////icJLf////7lk//MYxHMFya7sHAHFAKwxof////84LX/////ytsFi/////8EH//MYxHIFoa8AHAFPAr/////4V8Nnf////9wa3/////YdqA03//MYxHIFAbLkAgKLAP////5Uet/////ihxjHhOMb////+pGK//MYxHUDybLkAAAOHC3////+LjE7/////qE0t/wzKv/////U//MYxHwD2bLkAAAOHBMF/8ON/////9MMP/////g7O/4P//////MYxIMEAbbgAAKPAP+T/////+BCS/8Ij//////W/////9AD//MYxIoDwbLgAAAaHIv/wg//////rv/////WGND/iB////////MYxJIDgbLsAAHFAF3/////1BNEX/wkKIpv////+wF2M0j///MYxJsDcbLoAAHFEIL4cRP//////////6lqI3/////MwBdH//MYxKQDwbLoAAAOHI/6xVR///////////6z1W/////1mAFa//MYxKwEcbLoAAgUHCXf+oTQtn///////////8sqCGH///////MYxLECibLwAFAFMKwmaH/CAav//////////+g6I3/////S//MYxL0DubbkAGgFMAHQSL/0BEe/////8hU9/////xzACowa//MYxMUDwa70BDgFMLv84AUwPhz///////////KjyrdsBgAA//MYxM0DubLkAFALMGBQABAhCMnKEhGTmOYx9v7K5hn+b/5x//MYxNUDsbLoAFAFML/////1VQysiuNbebGsYGMBAhxv//////MYxN0DqbLkAGgPMPhVEpOKlcAFFB8oI5hoMBiECxgMACtq//MYxOUC2bbkAGgLMHW5c1YDD9P/////////+3////SqAAAA//MYxPAF4s7IAIgVMEwAAA4AAQFAMGGGADNbQvKFxHS59rf7//MYxO8F2s7QAmgbML/u/L/If8h+k///03f//70VQgwgEAoA//MYxO4GCs7MAJAVMAMAAAVH1ZZu3t+PQz4r/X/4z////+uA//MYxOwFss7kDGgLMAYQBwMAg6SEX+RLA1/////////+CvLH//MYxOwEma7UAmgVMJUJVUxBTUUzLjEwMFVVVVVVVVVVVUxB//MYxPAG2s60AKAbME1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOsISbMeVlAFMk1FMy4xMDBVVVVVVVVVVVVVVVVVVUxB//MYxOAEsJ7EAEgGUE1FMy4xMDBVVVVVVVVVVVVVVVUV1QCG//MYxOQA8AqQAHjAARBgUDAYDgYAbsAbfd0f0/6f8+CH/9Rj//MYxPcJCCYcR10AAv/aXD8cY//9+owq/1YjBCj2mFvf4cIL//MYxOkKmBaPHZYUAnhK5JsP/JvgXLugg6Qg/ko2KJqt/rc3//MYxNUHAA6gX4YQAOTCP/9N+YH0n+jGjf/X///oEJiJOiij//MYxNAGEA1FZcYAACVBKwzI7f/+aiW7eX/11TVutAmwILCu//MYxM4AUAQAAGAAAB/UKif9D/KH/Pf1qj30DcLJAWkmyPg7//MYxOMAAAAAAAAAABd/P/xen//lv9EM/+QpQNQy7F//1h1K//MYxOgAAANIAAAAAB/R/ygv/b/u/qUQzG/OBLA6TZvCA3+n//MYxOgAAANIAAAAAPl/+/+pA88G/UEMFH/En/jf8M/xJ/sV//MYxOgAAAAAAAAAAFkbVhsg1iXMdl/1F0wFHs/5Rd8RKgz0//MYxOoAYAQAAU8AAJs4eK6gQco/4aN9X7VEj+78v2d3/9cM//MYxP8IyAIA/4IQAv1DgrwgX8p/C//v3oInt7/y39+pAovc//MYxPINAR6oAZpoAMNvrA9h1jMc5+sXJ/P/kl+v/8b//9YM//MYxNUFCHbEAdIQAP1DiL7CAN8o30Cj/KP1w0d+d/L1Ba4b//MYxNcEyLq0AJgaaP50nUZ4LOhb8CX5R/oJfZ36+W/t/Qob//MYxNoFOP6wAKAOaPgcgd6AfRvwF/T9Qh/d+ioSgeCCEZzS//MYxNwFoLqwAC5aJDhjiJ36Gt2qCPoBHITp6nOAtq////////MYxNwEcLrAyJgOaP/6vL0M+uiVzWWgfWeHf54x/jhfyf////MYxOEEULbMUIgKaP/9vs/7VdvWz/sUPKoghuOgGI20Daxo//MYxOYFCJ7EAAvKJGf4WO3Xwp/8b/1////r//1eViKpRNs3//MYxOgF+JqsAC5UICYEdtT86e8b6+Em8lWLS0YbNygNbR+V//MYxOcFMJqwAC5UIG/xO+Yb1ItaBhq3EwNbKPyuUf/Bu6ol//MYxOkGORbBkJAFQCe4SQFTNnH51/Qf/B/f////////8tVu//MYxOcFGJqoAC6UIBAwGRBeDH/wT+t/X////////V5VDee4//MYxOkGGJq0yBZOIEHAfJssNzpb8K9PIP8l///////v9vn1//MYxOcEEJq4ABZKIAKtM9c3AsBHRMY25Q3r4xvbhT+Yhz1G//MYxO0IaJ64AB4ECNa3EAyajo3HBnxIM6+Md1IhywQWigBu//MYxOIHeJKsAC6OAKEAcNhHeca7oBg/+cb+ooZS8O/9jjTA//MYxNsHcLa88BUKABxmDSoLiNIk1n+h4BSRgwp3/CmNxWoM//MYxNQEWJLiQGgKoEoFSKfBgJioLRK7///lZ7/+3k1ulqjy//MYxNkD+JcWQDgE4upqhKHVf/Gf//2//rT2bdjv/9SKAbpM//MYxOAD8I8WQDgEhkFNRTMuMTAwqqqqqqqqqqqqqqqqqkxB//MYxOcFmI7AwJgEoE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOcFMGbAAJgEIE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOkGYJK4wJgKoE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOYFAJLWQGhKIE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOkEqI8KQDgKok1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxO0HAJL2WEoEKk1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgF0GaoAMDERE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOcGEBqMGGDEAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOUEoAHoAAjCAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOkAcAQAAHgAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgAAAAAAAAAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgAAANIAAAAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgAAANIAAAAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgAAANIAAAAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgAAANIAAAAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqkxB//MYxOgAAANIAAAAAE1FMy4xMDCqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//MYxOgAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
}   

var rcy_BOOT = [720, 400, "WebBrew SHell", 
"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAbNJREFUeNqkUztuwkAQnf16ndihgjJFWh8gCgdBokfKBWhSQJuKiIYmt4AT0OYAER1FGioUBQtk8IfMjLFRFClSkpWeZi3ve/P27a44Ho/wn6E7nc4N1ltE+EtujHjReZ7f9fv9h1arFeV5BlmWI1JIU5pnUBQFlCYVCKEZWltYr9evz89PjxoXXG2322i5XPJiAooyfpojoul0eqmNMcVgMIBms8m+KJOy67GeE9I0hSQ5gJTkQLNLzgBtCiIPh0OYz+ewWq3YOhEqbDYxYgeNRsj2CbvdhgUkLpbUaTweQ7vdBswCOyU13t8/II4TMMaB510wnPPx254FyGKv14PZbAaLxQK63S5EUYTELez3OZL8L7CWBEy5BbQolVIwmUwgCAK2PxqNMG2BgWnuWNo23JWqUporCxwOB+V5Hvi+z5ZpzyRYFAo7uROpJFJ4UpKAxKpqAUk/iFyFR+fu3FUdWNWVQM5oVJUdOOdwr3sWoHO2NjhZPxOrjt+uMl4iY62tz70cBgNc10QpxamjqIlJEuOxNgIRhuE97vn6Lw8J3b6RpH96SPqX/IwelPjvc/4UYABrxgUeC3Nd3QAAAABJRU5ErkJggg=="
];

rcy_BOOTLOADER = "let bootscript = " + String(rcy_bootjs) + ";let DEATH;bootscript();";



