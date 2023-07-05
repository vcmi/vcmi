const { invoke } = window.__TAURI__.tauri;
const { emit, listen } = window.__TAURI__.event;
const { fetch, Body } = window.__TAURI__.http;

let lobbyAddressInputEl;
let usernameInputEl;
let nodeAddressInputEl;
let programIdInputEl;
let programId2InputEl;
let programId3InputEl;
let accountIdInputEl;
let passwordInputEl;

let roomNameEl;
let roomPasswordEl;
let userPasswordEl;
let roomMaxPlayersEl;

let intervalId;
let isRoomCreator;
async function connect() {
  console.log("request mnemonic phrase");
  const url = 'https://vcmi.gear-tech.io/user/get_keys';
  const body = Body.json({
    nickname: usernameInputEl.value,
    password: userPasswordEl.value
  }
  );
  fetch(url, {
    method: "POST",
    headers: {
      'Content-Type': 'application/json'
    },
    body: body
  })
    .then(async response => {

      console.log(response.data)
      await invoke("connect", {
        lobbyAddress: lobbyAddressInputEl.value,
        username: usernameInputEl.value,
        programId: programIdInputEl.value,
        metaProgramId: programId2InputEl.value,
        battleProgramId: programId3InputEl.value,
        nodeAddress: nodeAddressInputEl.innerText,
        accountId: response.data.privateKey,
        password: ""
      })

    })
    .catch(error => {
      console.log('Error:', error);
    });
}

function checkIpfs() {
  const url = 'http://127.0.0.1:5001';
  fetch(url, {
    method: 'HEAD',
    timeout: 30,
    mode: 'cors'
  })
    .then(response => {
      if (response.ok) {
        console.log("her1")

      }
    })
    .then(data => {
      // clearInterval(intervalId);
      document.getElementById("connect-button").removeAttribute("disabled");
      let alert = document.getElementById("alert");
      clearInterval(intervalId)
      alert.hidden = true
      document.getElementById("connection-message").innerText = ""
    })
    .catch(error => {
      if (error instanceof TypeError && error.message.includes('redirect')) {
        console.log('Cross-origin redirection error occurred');
      } else {
        console.log('Error:', error);
        let alert = document.getElementById("alert");
        alert.hidden = false

        document.getElementById("connection-message").innerText = "Can't connect to IPFS:\n" + error
        document.getElementById("connect-button").setAttribute("disabled", "");
      }
    });
}

//create new room
//%1: room name
//%2: password for the room
//%3: max number of players
//%4: mods used by host
// each mod has a format modname&modversion, mods should be separated by ; symbol
// {CREATE, "<NEW>%1<PSWD>%2<COUNT>%3<MODS>%4"},

async function newRoom() {
  console.log(roomMaxPlayersEl.innerText, roomMaxPlayersEl.value)
  isRoomCreator = true;
  console.log("NewRoom", roomNameEl.value, roomPasswordEl.value, roomMaxPlayersEl.value, "isCreator:", isRoomCreator)
  await invoke("new_room", {
    roomName: roomNameEl.value,
    password: roomPasswordEl.value,
    maxPlayers: parseInt(roomMaxPlayersEl.innerText),
    mods: "h3-for-vcmi-englisation&1.2;vcmi&1.2;vcmi-extras&3.3.6;vcmi-extras.arrowtowericons&1.1;vcmi-extras.battlefieldactions&0.2;vcmi-extras.bonusicons&0.8.1;vcmi-extras.bonusicons.bonus icons&0.8;vcmi-extras.bonusicons.immunity icons&0.6;vcmi-extras.extendedrmg&1.2;vcmi-extras.extraresolutions&1.0;vcmi-extras.quick-exchange&1.0"
  });
}

async function joinRoom(roomName) {
  isRoomCreator = false;
  console.log("JOIN ROOM", roomName, "isCreator:", isRoomCreator);
  await invoke("join_room", {
    roomName: roomName,
    password: "",
    mods: ""
  });
}

async function ready(roomName) {
  console.log("ready", roomName);
  await invoke("ready", {
    roomName: roomName,
  });
}

async function leave(roomName) {
  console.log("leave", roomName);
  await invoke("leave", {
    roomName: roomName,
  });
}

async function hostmode(mode) {
  console.log("hostmode", mode);
  await invoke("hostmode", {
    mode: mode,
  });
}

window.addEventListener("DOMContentLoaded", () => {
  checkIpfs()
  intervalId = setInterval(checkIpfs, 1000); // Fetch data every 5 seconds
  lobbyAddressInputEl = document.querySelector("#lobby-address")
  usernameInputEl = document.querySelector("#username")
  nodeAddressInputEl = document.querySelector("#node-address")
  programIdInputEl = document.querySelector("#program-id");
  programId2InputEl = document.querySelector("#program-id-meta");
  programId3InputEl = document.querySelector("#program-id-battle");
  accountIdInputEl = document.querySelector("#account-id")
  passwordInputEl = document.querySelector("#password")
  document.querySelector("#password-ok").addEventListener("click", () => connect());

  roomNameEl = document.querySelector("#room-name")
  roomPasswordEl = document.querySelector("#room-password")
  userPasswordEl = document.querySelector("#user-password")
  roomMaxPlayersEl = document.querySelector("#room-max-players")
  document.querySelector("#new-room-button").addEventListener("click", () => newRoom());
});

await listen('alert', (event) => {
  console.log("js: connection_view: " + event)
  let payload = event.payload;

  let alert = document.getElementById("alert");
  alert.hidden = false

  document.getElementById("connection-message").innerText = payload
})

await listen('showRooms', (event) => {
  let roomView = document.getElementById("collapseRoom");
  let bsCollapse = new bootstrap.Collapse(roomView);
  console.log("show Rooms:", users);
  bsCollapse.toggle();
  document.getElementById("connect-button").hidden = true
  const modalElement = document.getElementById("passwordModal");
  modalElement.classList.remove("show");
  modalElement.style.display = "none";
  modalElement.setAttribute("aria-hidden", "true");
  modalElement.removeAttribute("aria-modal");
  const modalBackdrop = document.getElementsByClassName("modal-backdrop")[0];
  modalBackdrop.parentNode.removeChild(modalBackdrop);
  document.body.classList.remove("modal-open");
})

await listen('addUsers', (event) => {
  let users = event.payload;
  console.log("add Users:", users);
  const list = document.getElementById("users");
  while (list.firstChild) {
    list.removeChild(list.firstChild);
  }
  for (let i = 0; i < users.length; ++i) {
    const listItem = document.createElement("li");
    listItem.className = "list-group-item";
    listItem.classList.add("bg-secondary");
    // listItem.classList.add("border-primary-subtle")

    listItem.style = "--bs-bg-opacity: .2;"
    listItem.textContent = users[i];
    list.appendChild(listItem);
  }
})

await listen('addSessions', (event) => {
  let sessions = event.payload;
  console.log("add Session:", sessions);
  const div = document.getElementById("sessions");
  if (div) {
    while (div.firstChild !== null) {
      div.removeChild(div.firstChild)
    }
    for (let i = 0; i < sessions.length; ++i) {
      const button = document.createElement("button");
      button.classList.add("btn")
      button.classList.add("btn-secondary")
      // button.classList.add("border-primary")
      // button.style = "background-color: rgb(0, 0, 50);"
      button.textContent = sessions[i].name
      button.value = sessions[i].name
      button.addEventListener("click", () => joinRoom(button.value));

      const badge = document.createElement("span");
      badge.classList.add("badge")
      badge.classList.add("text-bg-light")
      badge.classList.add("rounded-pill")
      badge.textContent = sessions[i].joined + "/" + sessions[i].total
      button.appendChild(badge)

      div.appendChild(button)
    }
  }
})

await listen('chatMessage', (event) => {
  let messages = event.payload;
  console.log("chat Message:", messages);
  const list = document.getElementById("messages");
  const listItem = document.createElement("li");

  listItem.className = "list-group-item";
  const strong = document.createElement("strong");
  strong.textContent = messages[0] + ": ";
  listItem.appendChild(strong);
  const text = document.createElement("text");
  text.textContent = messages[1];
  listItem.appendChild(text);

  listItem.classList.add("bg-dark");

  listItem.style = "--bs-bg-opacity: .2;"
  list.appendChild(listItem);
})

await listen('created', (event) => {
  let room_name = event.payload;
  console.log("created room:", room_name);
  const modalElement = document.getElementById("roomModal");
  modalElement.classList.remove("show");
  modalElement.style.display = "none";
  modalElement.setAttribute("aria-hidden", "true");
  modalElement.removeAttribute("aria-modal");
  const modalBackdrop = document.getElementsByClassName("modal-backdrop")[0];
  modalBackdrop.parentNode.removeChild(modalBackdrop);
  document.body.classList.remove("modal-open");
})

await listen('status', (event) => {
  const usersCount = event.payload[0]
  const statuses = event.payload[1]

  const players = document.getElementById("players")
  if (players) {
    while (players.firstChild) { players.removeChild(players.firstChild); }

    for (let i = 0; i < usersCount; ++i) {
      const listItem = document.createElement("li");
      const status = statuses[i];

      listItem.className = "list-group-item";
      listItem.classList.add("bg-secondary");
      listItem.style = "--bs-bg-opacity: .2;"

      if (status[1] === 'True') {
        const div = document.createElement("div");
        div.classList.add("form-check")

        const check = document.createElement("input");
        check.setAttribute("checked", "")
        check.setAttribute("type", "checkbox")
        check.classList.add("form-check-input");
        check.classList.add("form-check-input");
        check.style = "background-color: #28a745; border-color: #28a745;"
        check.id = status[0] + i
        div.appendChild(check)

        const label = document.createElement("label");
        label.classList.add("form-check-label");
        label.setAttribute("for", check.id)
        label.textContent = status[0]
        div.appendChild(label)

        listItem.classList.add("bg-success");
        listItem.classList.add("border-success-subtle")
        listItem.style = "--bs-bg-opacity: .2;"
        listItem.appendChild(div)
      } else {
        const text = document.createElement("text");
        text.textContent = status[0];
        listItem.appendChild(text);
        listItem.appendChild(text)
      }
      players.appendChild(listItem);
    }
  }
  console.log("statuses:", event.payload);
})

await listen('joined', (event) => {
  let joined = event.payload;
  let room_name = joined[0]
  let username = joined[1]

  if (username == document.getElementById("username").value) {
    const div = document.getElementById("room");
    while (div.firstChild) {
      div.removeChild(div.firstChild)
    }
    createPlayersInRoomWidget(div, room_name)
  }

  console.log("joined:", joined)
})

await listen('updateGameMode', (event) => {
  let game_mod = event.payload;
  console.log("game_mod:", game_mod);

  const newGame = document.getElementById("new-game");
  newGame.checked = game_mod == 0;
  

  const newGameLabel = document.getElementById("new-game-label");
  newGameLabel.setAttribute("for", "new-game")
  

  const loadGame = document.getElementById("load-game");
  loadGame.checked = game_mod == 1;
  

  const loadGameLabel = document.getElementById("load-game-label");
  loadGameLabel.setAttribute('for', "load-game")
  
  if (!isRoomCreator) {
    loadGameLabel.setAttribute("disabled", "")
    loadGame.setAttribute("disabled", '')
    newGameLabel.setAttribute("disabled", '')
    newGame.setAttribute("disabled", "")
  }
})

function setupDropdownMenu(buttonId, menuId) {
  const dropdownButton = document.querySelector('#' + buttonId);
  const dropdownMenu = document.querySelector('#' + menuId);
  dropdownMenu.addEventListener('click', function (e) {
    if (e.target.classList.contains('dropdown-item')) {
      const selectedItem = e.target.textContent;
      dropdownButton.textContent = selectedItem;
    }
  });
}
setupDropdownMenu("node-address", "node-addresses");
setupDropdownMenu("room-max-players", "players-count");


function createPlayersInRoomWidget(parentElement, roomName) {
  // Create label element
  const label = document.createElement("label");
  label.className = "form-label";
  label.innerHTML = `Players in the <strong>${roomName}</strong> room`;

  // Create div elements
  const divRow1 = document.createElement("div");
  divRow1.className = "row";

  const divCol1 = document.createElement("div");
  divCol1.className = "col";

  const divInner = document.createElement("div");
  divInner.style.height = "150px";
  divInner.className = "bg-dark-subtle rounded-2 overflow-y-auto";

  // Create ul element
  const ul = document.createElement("ul");
  ul.className = "list-group list-group-flush";
  ul.id = "players";

  // Create div elements
  const divRow2 = document.createElement("div");
  divRow2.className = "row pt-3 align-items-start justify-content-start";

  const divCol2 = document.createElement("div");
  divCol2.className = "col";

  // Create form-check elements
  const formCheck1 = document.createElement("div");
  formCheck1.className = "form-check form-check-inline";

  const newGame = document.createElement("input");
  newGame.className = "form-check-input";
  newGame.type = "radio";
  newGame.name = "inlineRadioOptions";
  newGame.id = "new-game";
  newGame.value = "option1";
  newGame.checked = true
  newGame.onchange = () => hostmode(0)

  const newGameLabel = document.createElement("label");
  newGameLabel.className = "form-check-label";
  newGameLabel.htmlFor = "inlineRadio1";
  newGameLabel.textContent = "New game";
  newGameLabel.id = "new-game-label";

  const formCheck2 = document.createElement("div");
  formCheck2.className = "form-check form-check-inline";

  const loadGame = document.createElement("input");
  loadGame.className = "form-check-input";
  loadGame.type = "radio";
  loadGame.name = "inlineRadioOptions";
  loadGame.id = "load-game";
  loadGame.value = "option2";
  loadGame.onchange = () => hostmode(1)

  const loadGameLabel = document.createElement("label");
  loadGameLabel.className = "form-check-label";
  loadGameLabel.htmlFor = "inlineRadio2";
  loadGameLabel.textContent = "Load game";
  loadGameLabel.id = "load-game-label";

  // Create div elements
  const divRow3 = document.createElement("div");
  divRow3.className = "row pt-3 align-items-start justify-content-start";

  const divCol3 = document.createElement("div");
  divCol3.className = "col col-auto";

  const divCol4 = document.createElement("div");
  divCol4.className = "col col-auto";

  // Create button elements
  const buttonLeave = document.createElement("button");
  buttonLeave.type = "button";
  buttonLeave.className = "btn btn-light";
  buttonLeave.setAttribute("data-bs-toggle", "modal");
  buttonLeave.innerHTML = '<i class="bi bi-plus"></i>Leave';
  buttonLeave.addEventListener("click", () => leave(roomName));

  const buttonReady = document.createElement("button");
  buttonReady.type = "button";
  buttonReady.className = "btn btn-success";
  buttonReady.setAttribute("data-bs-toggle", "modal");
  buttonReady.innerHTML = '<i class="bi bi-plus"></i>Ready';
  buttonReady.addEventListener("click", () => ready(roomName));

  // Append elements to their respective parent elements
  divInner.appendChild(ul);
  divCol1.appendChild(divInner);
  divRow1.appendChild(divCol1);

  formCheck1.appendChild(newGame);
  formCheck1.appendChild(newGameLabel);

  formCheck2.appendChild(loadGame);
  formCheck2.appendChild(loadGameLabel);

  divCol2.appendChild(formCheck1);
  divCol2.appendChild(formCheck2);
  divRow2.appendChild(divCol2);

  divCol3.appendChild(buttonLeave);
  divCol4.appendChild(buttonReady);
  divRow3.appendChild(divCol3);
  divRow3.appendChild(divCol4);

  parentElement.appendChild(label);
  parentElement.appendChild(divRow1);
  parentElement.appendChild(divRow2);
  parentElement.appendChild(divRow3);
}
