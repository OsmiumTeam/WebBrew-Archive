let bootjs = function(){
    
    print("<br><color 0,0,255/> ░░░░░░░░░░░░░░<color 255,255,255/>v006")
    print(" <color 0,0,255/>░░░<color 255,255,255/>WEBBREW<color 0,0,255/>░░░░░░░░")
    print(" <color 0,0,255/>░░░░░<color 255,255,255/>BOOTSCRIPT<color 0,0,255/>░░░")
    print(" <color 0,0,255/>░░░░░░░░░░░░░░░░░░")
    print(" <color 0,0,255/>░<color 255,255,255/>▓▓<color 0,0,255/>░░<color 255,255,255/>▓▓<color 0,0,255/>░░<color 255,255,255/>▓▓<color 0,0,255/>░░<color 255,255,255/>▓▓<color 0,0,255/>░░<color 255,255,255/>▓")
    print(" <color 255,255,255/>▓<color 0,191,255/>▒▒<color 255,255,255/>▓▓<color 0,191,255/>▒▒<color 255,255,255/>▓▓<color 0,191,255/>▒▒<color 255,255,255/>▓▓<color 0,191,255/>▒▒<color 255,255,255/>▓▓<color 0,191,255/>▒")
    print(" <color 0,191,255/>▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒")
    print(" <color 0,191,255/>▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒<br>")

    print(" <color 255,255,255/>╟────────────────╢<br>")

    print(" WebBrew bootscript<br>")
    print(" WebBrew version in script: v006")
    print(" Bootscript created by: The WebBrew Team<br>")
    print(" Script includes:<br>")
    print(" Installer Image: true")
    print(" Updater Image: false")
    print(" Recovery Image: false<br>")
    print(" <color 255,255,255/>╟────────────────╢<br>")
    
    print(" starting in 3 seconds...<br><color 255,255,255/> hold a boot key to go to the respective mode<br><color 255,255,255/> or wait to boot into webbrew!<br>")
    let bootmode = "normal"
    document.addEventListener("keydown", function (e) {          
        if (e.key === "r") {
            print(" STARTING INSTALLER<br>")
            bootmode = "install"
        }
    })
    document.addEventListener("keydown", function (e) {          
        if (e.key === "c") {
            print(" ENTERING RECOVERY MODE<br>")
            bootmode = "recovery"
        }
    })

    sleep(3000).then(() => {

        if (bootmode === "recovery"){
            SH_Output = ""
            print("WebBrew Recovery SHell (WBRSH)")
            print("By the WebBrew Team<br>")
            bashq = true;
            bash()            
        } else if (bootmode === "install") {
            popup.aboutFile("../wbInstaller/install.html", 600, 600, "WebBrew Installer")
            parent.close()
        } else {

            if (localStorage.getItem('ENV_WBVER') === "6") {
                print(" Detected WebBrew Version: v" + ("00" + localStorage.getItem("ENV_WBVER")).slice(-3))
                print(" booting webbrew...<br>")
                sleep(500).then(() => {
                    wbsh.run("mount " + localStorage.getItem("ENV_WBDISK"))
                    wbsh.run("change " + localStorage.getItem("ENV_WBDISK"))
                    reloadFrame();
                    sleep(1000).then(() => {
                        SH_Output = ""
                        print("WebBrew SHell (WBSH)")
                        print("By the WebBrew Team<br>")
                        wbsh.run("ver")
                        print("")
                        bashq = true;
                        bash()
                    })
                })            
            } else if (typeof localStorage.getItem('ENV_WBVER') === "number") {
                print(" Detected WebBrew Version: v" + ("00" + localStorage.getItem("ENV_WBVER")).slice(-3))
                print(" cannot boot WebBrew. Reason: version installed is newer or older, get correct BOOT.js file.<br>")
                //print(" WebBrew version 006 is the earliest version to use this type of boot, older versions didnt have the ENV_WBVER variable")
                //print(" The only 2 reasons you would be able to see this are as following: ")
                //print(" 1. you have a special version of webbrew \"most likely a dev build\"")
                //print(" 2. your version string is corrupt. if this is the case, contact me (Block_2222) or any other Webbrew team member at WBdevs@proton.me<br>")
                print(" press the \"r\" key to reinstall")
            } else {
                print(" cannot detect WebBrew Version")
                print(" trying recovery (just in case webbrew is installed)")
                print(" recovery did not return results<br>")
                print(" Press \"r\" to install WebBrew<br>")
            }
        }
    });
}   

BOOTSCRIPT = "let bootscript = " + String(bootjs) + ";bootscript();";