document.getElementById("romin").onchange = function(){
    document.querySelector("desmond-player").loadURL(URL.createObjectURL(document.getElementById("romin").files[0]))
}
