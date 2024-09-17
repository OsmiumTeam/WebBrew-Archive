addTool("Shell (WBSH-Recovery)").addEventListener("click", function() {
    let script = document.createElement("script")
    script.setAttribute("src", "payload/wbpl_extra/tool/shell-wbsh/BOOT-Recovery.js")
    document.body.appendChild(script)

    sleep(100).then(() => {
        const searchParams = new URLSearchParams(`run=${encodeURIComponent(rcy_BOOTLOADER)}`)

        popup.open("payload/wbpl_extra/tool/shell-wbsh/wbConsole/wbsh.html" + "?" + searchParams.toString(), 720, 400, "WebBrew recovery SHell", 
            "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAbNJREFUeNqkUztuwkAQnf16ndihgjJFWh8gCgdBokfKBWhSQJuKiIYmt4AT0OYAER1FGioUBQtk8IfMjLFRFClSkpWeZi3ve/P27a44Ho/wn6E7nc4N1ltE+EtujHjReZ7f9fv9h1arFeV5BlmWI1JIU5pnUBQFlCYVCKEZWltYr9evz89PjxoXXG2322i5XPJiAooyfpojoul0eqmNMcVgMIBms8m+KJOy67GeE9I0hSQ5gJTkQLNLzgBtCiIPh0OYz+ewWq3YOhEqbDYxYgeNRsj2CbvdhgUkLpbUaTweQ7vdBswCOyU13t8/II4TMMaB510wnPPx254FyGKv14PZbAaLxQK63S5EUYTELez3OZL8L7CWBEy5BbQolVIwmUwgCAK2PxqNMG2BgWnuWNo23JWqUporCxwOB+V5Hvi+z5ZpzyRYFAo7uROpJFJ4UpKAxKpqAUk/iFyFR+fu3FUdWNWVQM5oVJUdOOdwr3sWoHO2NjhZPxOrjt+uMl4iY62tz70cBgNc10QpxamjqIlJEuOxNgIRhuE97vn6Lw8J3b6RpH96SPqX/IwelPjvc/4UYABrxgUeC3Nd3QAAAABJRU5ErkJggg==")
    })
})