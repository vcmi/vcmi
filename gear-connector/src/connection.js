const { invoke } = window.__TAURI__.tauri;
const { appWindow, LogicalSize } = window.__TAURI__.window;
const { emit, listen } = window.__TAURI__.event;


window.addEventListener("DOMContentLoaded", () => {
    document
        .querySelector("#expand-log")
        .addEventListener("click", () => invoke("expand_log"));
});

await listen('log', (event) => {
    console.log("js: log: " + event)
    let incoming = event.payload;
    let textarea = document.getElementById("log");
    textarea.innerHTML += `<li class="list-group-item list-group-item-info text-break">${incoming}</li>`
})

await listen('debug', (event) => {
    console.log("js: log: " + event)
    let incoming = event.payload;
    let textarea = document.getElementById("log");
    textarea.innerHTML += `<li class="list-group-item">${incoming}</li>`
})

await listen('warn', (event) => {
    console.log("js: log: " + event)
    let incoming = event.payload;
    let textarea = document.getElementById("log");
    textarea.innerHTML += `<li class="list-group-item list-group-item-warning">${incoming}</li>`
})

await listen('error', (event) => {
    console.log("js: log: " + event)
    let incoming = event.payload;
    let textarea = document.getElementById("log");
    textarea.innerHTML += `<li class="list-group-item list-group-item-danger">${incoming}</li>`
})

await listen('update_balance', (event) => {
    console.log("js: log: " + event)
    let incoming = event.payload;
    let balance = document.getElementById("balance-value");

    let n = new Intl.NumberFormat("en-GB", {
        notation: "compact",
        compactDisplay: "short",
    }).format(incoming);
    balance.innerText = n
})

await listen('update_account_id', (event) => {
    console.log("js: log: " + event)
    let incoming = event.payload;
    let accountId = document.getElementById("account-id")
    accountId.innerText = incoming
})

function feedReducer(args) {
    return new Promise((res, rej) => {
        res(args);
    })
}

let showLog = async (...args) => {
    let result = await feedReducer(args);
    console.log(result);

    let size = appWindow.inner_size;
    console.log("showLog");
    await appWindow.setSize(new LogicalSize(size.width, 500));
}
