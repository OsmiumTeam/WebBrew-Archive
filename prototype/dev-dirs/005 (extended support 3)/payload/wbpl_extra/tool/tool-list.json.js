tool_list = tool_list.concat(["payload/wbpl_extra/tool/shell-wbsh/tool.js", "payload/wbpl_extra/tool/ABEPL/tool.js","payload/wbpl_extra/tool/ErudaDevtools/tool.js"])

tool_list.forEach(e => {
    let script = document.createElement("script")
    script.setAttribute("src", e)
    document.body.appendChild(script)
});