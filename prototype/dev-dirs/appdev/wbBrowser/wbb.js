let urlbar = document.getElementById("urlbar")
let site;
let sitejson;
let jsonrequest;
const fakedoc = document.createElement("html");

document.getElementById("urlgo").addEventListener("click", function(){
    console.log(urlbar.value);

    let proxyURL = "https://api.allorigins.win/get?url=" + encodeURIComponent(urlbar.value)

    let xhr = new XMLHttpRequest();
    xhr.open('GET', proxyURL, true);
    //xhr.responseType = 'document';
    xhr.onload = function(e) {
        console.log(JSON.parse(e.target.response));
        sitejson = JSON.parse(e.target.response);
        site = JSON.parse(e.target.response).contents;

        fakedoc.innerHTML = site;
        
        let atag = fakedoc.querySelectorAll("a");
        let imgtag = fakedoc.querySelectorAll("img");
        let linktag = fakedoc.querySelectorAll("link");
        let scripttag = fakedoc.querySelectorAll("script");

        for (let i = 0; i < atag.length; i++) {
            if (atag[i].getAttribute("href")  !== null) {
                atr = atag[i].getAttribute("href")
                console.log(atag[i]);
                console.log(atr);
                console.log(typeof atr);
                if (atr.includes("https://") || atr.includes("http://" || atr.includes("data://"))) {
                    atag[i].setAttribute("href", "https://api.allorigins.win/get?url=" + encodeURIComponent(atr))
                } else [
                    atag[i].setAttribute("href", "https://api.allorigins.win/get?url=" + encodeURIComponent(sitejson.status.url) + encodeURIComponent(atr))
                ]                    
            }
            
        }
        for (let i = 0; i < imgtag.length; i++) {
            if (imgtag[i].getAttribute("src")  !== null) {
                atr = imgtag[i].getAttribute("src")
                console.log(imgtag[i]);
                console.log(atr);
                console.log(typeof atr);
                if (atr.includes("https://") || atr.includes("http://" || atr.includes("data://"))) {
                    imgtag[i].setAttribute("src", "https://api.allorigins.win/get?url=" + encodeURIComponent(atr))
                } else [
                    imgtag[i].setAttribute("src", "https://api.allorigins.win/get?url=" + encodeURIComponent(sitejson.status.url) + encodeURIComponent(atr))
                ]
            }
        }
        for (let i = 0; i < linktag.length; i++) {
            if (linktag[i].getAttribute("href")  !== null) {
                atr = linktag[i].getAttribute("href")
                console.log(linktag[i]);
                console.log(atr);
                console.log(typeof atr);
                if (atr.includes("https://") || atr.includes("http://" || atr.includes("data://"))) {
                    linktag[i].setAttribute("href", "https://api.allorigins.win/get?url=" + encodeURIComponent(atr))
                } else [
                    linktag[i].setAttribute("href", "https://api.allorigins.win/get?url=" + encodeURIComponent(sitejson.status.url) + encodeURIComponent(atr))
                ]
            }
        }
        for (let i = 0; i < scripttag.length; i++) {
            if (scripttag[i].getAttribute("src")  !== null) {
                atr = scripttag[i].getAttribute("src")
                console.log(scripttag[i]);
                console.log(atr);
                console.log(typeof atr);
                if (atr.includes("https://") || atr.includes("http://" || atr.includes("data://"))) {
                    scripttag[i].setAttribute("src", "https://api.allorigins.win/get?url=" + encodeURIComponent(atr))
                } else [
                    scripttag[i].setAttribute("src", "https://api.allorigins.win/get?url=" + encodeURIComponent(sitejson.status.url) + encodeURIComponent(atr))
                ]
            }
        }
    
        let wbbcontainer = document.createElement("wbb-container");
        document.body.appendChild(wbbcontainer);
    
    };
    xhr.send()
    
})

customElements.define('wbb-container', class extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({mode: 'open'}).appendChild(
            fakedoc.cloneNode(true)
        );
    }
})