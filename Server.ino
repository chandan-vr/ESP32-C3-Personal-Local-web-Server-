/*
========================================================
              TECHNICAL BD SERVER
              ESP32-C3 SUPER MINI
========================================================
WiFi Name : Technical BD Server
Password  : ChangeMe123
Dashboard : http://192.168.4.1

Required:
- ESP32 board package
- ArduinoJson 6.x

Features:
- Account Setup / Login / Logout
- File Manager
- Create Folder / Text Document
- Open / Edit / Save
- Folder Navigation
- Three-dot Menu
- Delete Box / Restore
- Permanent Delete (recursive)
- Storage Information
- Light / Dark Theme
- Accent Color
- Change Password
- JSON Backup
- All Data Reset
- LittleFS persistent storage
========================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <vector>
#include "mbedtls/sha256.h"

// ---------------- CONFIGURATION ----------------

const char* AP_SSID = "Technical BD Server";
const char* AP_PASS = "ChangeMe123";

const char* DB_FILE  = "/vault.json";
const char* TMP_FILE = "/vault.tmp";
const char* BAK_FILE = "/vault.bak";

WebServer server(80);
Preferences prefs;

DynamicJsonDocument db(30000);

bool fsReady = false;

String vaultUser;
String vaultHash;
String vaultSalt;
String sessionID;

String currentTheme = "light";
String currentAccent = "#2563eb";

// ---------------- WEB PAGE ----------------

const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Technical BD Server</title>

<style>
:root{--a:#2563eb}
*{box-sizing:border-box}

body{
 margin:0;
 font-family:Arial,sans-serif;
 background:#f1f5f9;
 color:#172033;
}

body.dark{
 background:#111827;
 color:white;
}

header{
 background:var(--a);
 color:white;
 padding:18px;
 font-size:23px;
 font-weight:bold;
}

main{
 max-width:1000px;
 margin:auto;
 padding:12px;
}

.card{
 background:white;
 color:#172033;
 border-radius:16px;
 padding:18px;
 margin-bottom:14px;
}

.dark .card{
 background:#1f2937;
 color:white;
}

input,textarea,select{
 width:100%;
 padding:12px;
 margin:7px 0;
 border:1px solid #cbd5e1;
 border-radius:8px;
 font-size:16px;
}

button{
 border:0;
 background:var(--a);
 color:white;
 padding:11px 15px;
 border-radius:8px;
 margin:3px;
 cursor:pointer;
 font-size:15px;
}

button:active{opacity:.75}

.red{background:#dc2626}
.gray{background:#64748b}
.green{background:#15803d}

.hidden{display:none!important}

nav{
 display:flex;
 flex-wrap:wrap;
 gap:5px;
}

#files{
 display:grid;
 grid-template-columns:repeat(auto-fill,minmax(145px,1fr));
 gap:10px;
}

.file{
 min-width:0;
 padding:15px 8px;
 border:1px solid #cbd5e1;
 border-radius:12px;
 text-align:center;
 overflow:visible;
 position:relative;
}

.dark .file{border-color:#475569}

.ico{font-size:42px}

.fname{
 margin-top:8px;
 font-size:15px;
 font-weight:bold;
 overflow-wrap:anywhere;
}

.actions{
 margin-top:12px;
 display:flex;
 align-items:center;
 justify-content:center;
 gap:5px;
}

.openBtn{
 background:#15803d;
 min-width:105px;
 font-weight:bold;
}

.menuWrap{
 position:relative;
 display:inline-block;
}

.menuBtn{
 background:#64748b;
 font-size:23px;
 line-height:1;
 padding:10px 12px;
 margin:0;
 min-width:44px;
}

.menuPanel{
 display:none;
 position:absolute;
 left:calc(100% + 8px);
 right:auto;
 top:0;
 width:180px;
 max-width:calc(100vw - 35px);
 background:white;
 color:#172033;
 border:1px solid #cbd5e1;
 border-radius:10px;
 padding:6px;
 z-index:1000;
 box-shadow:0 5px 18px rgba(0,0,0,.25);
}

.dark .menuPanel{
 background:#1f2937;
 color:white;
 border-color:#475569;
}

.menuPanel button{
 display:block;
 width:100%;
 text-align:center;
 margin:4px 0;
 padding:12px 6px;
 white-space:normal;
 font-size:14px;
}

#crumb{
 padding:12px 4px;
 overflow-wrap:anywhere;
}

#crumb span{
 color:var(--a);
 cursor:pointer;
 font-weight:bold;
}

#editor{
 position:fixed;
 inset:0;
 background:#f1f5f9;
 z-index:2000;
 display:flex;
 flex-direction:column;
 padding:10px;
}

.dark #editor{background:#111827}

#editor input{flex:none}

#editor textarea{
 flex:1;
 resize:none;
 min-height:0;
 font-family:monospace;
 line-height:1.6;
}

#storageBar{
 height:12px;
 background:#cbd5e1;
 border-radius:10px;
 overflow:hidden;
}

#storageUsed{
 height:100%;
 width:0;
 background:var(--a);
}

.small{
 font-size:12px;
 opacity:.7;
}

hr{
 border:0;
 border-top:1px solid #cbd5e1;
 margin:18px 0;
}

.modal{
 position:fixed;
 inset:0;
 z-index:3000;
 background:rgba(0,0,0,.7);
 display:flex;
 align-items:center;
 justify-content:center;
 padding:15px;
}

.modalBox{
 width:100%;
 max-width:420px;
 background:white;
 color:#172033;
 border-radius:14px;
 padding:20px;
 box-shadow:0 10px 35px rgba(0,0,0,.35);
}

.dark .modalBox{
 background:#1f2937;
 color:white;
}

.modalBox h3{margin-top:0}

.dangerText{
 color:#dc2626;
 font-weight:bold;
 line-height:1.5;
}

.checkLine{
 display:flex;
 align-items:center;
 gap:8px;
 margin:12px 0;
}

.checkLine input{
 width:auto;
 margin:0;
}

.modalActions{
 display:flex;
 flex-wrap:wrap;
 justify-content:flex-end;
 gap:5px;
 margin-top:12px;
}

</style>
</head>

<body>

<header>📁 Technical BD Server</header>

<main>

<!-- AUTH -->

<div id="auth" class="card">
 <h2 id="authTitle">Login</h2>

 <input id="username" placeholder="Username"
 autocomplete="username">

 <input id="password" type="password"
 placeholder="Password" autocomplete="current-password">

 <button onclick="login()">Continue</button>

 <p id="authMsg"></p>
</div>

<div id="app" class="hidden">

<!-- NAVIGATION -->

<div class="card">
 <nav>
  <button onclick="newFolder()">+ Folder</button>
  <button onclick="newNote()">+ Text Document</button>
  <button onclick="goBack()" class="gray">← Back</button>
  <button onclick="showTrash()" class="gray">Delete Box</button>
  <button onclick="showSettings()" class="gray">Settings</button>
  <button onclick="logout()" class="red">Logout</button>
 </nav>
</div>

<!-- STORAGE -->

<div id="storageCard" class="card">
 <h3>Storage Information</h3>

 <div id="storageBar">
  <div id="storageUsed"></div>
 </div>

 <p id="storageText">Loading...</p>
</div>

<!-- FILES -->

<div id="filesCard" class="card">
 <h3>My Files</h3>

 <div id="crumb"></div>
 <div id="files"></div>

 <p id="emptyMsg" class="small"></p>
</div>

<!-- DELETE BOX -->

<div id="trash" class="card hidden">
 <h3>Delete Box</h3>

 <div id="trashFiles"></div>

 <button class="red" onclick="emptyTrash()">
  Empty Delete Box
 </button>

 <button class="gray" onclick="showHome()">Back</button>
</div>

<!-- SETTINGS -->

<div id="settings" class="card hidden">

 <h3>Settings</h3>

 <label>Theme</label>

 <select id="theme">
  <option value="light">Light</option>
  <option value="dark">Dark</option>
 </select>

 <label>Accent Color</label>
 <input id="accent" type="color" value="#2563eb">

 <button onclick="saveSettings()">Save Settings</button>

 <hr>

 <h3>Change Password</h3>

 <input id="oldPass" type="password"
 placeholder="Current password">

 <input id="newPass" type="password"
 placeholder="New password">

 <button onclick="changePassword()">Change Password</button>

 <button class="gray" onclick="backup()">Export Backup</button>

 <hr>

 <h3>All Data Reset</h3>

 <p class="dangerText">
 Warning: This will permanently erase all files,
 folders, documents, Delete Box data and account settings.
 This action cannot be undone.
 </p>

 <button class="red" onclick="openResetModal()">
 All Data Reset
 </button>

 <br>

 <button class="gray" onclick="showHome()">Back</button>

</div>

</div>
</main>

<!-- EDITOR -->

<div id="editor" class="hidden">

 <div>
  <button onclick="saveNote()">Save</button>
  <button class="gray" onclick="closeEditor()">Close</button>
 </div>

 <input id="editTitle" placeholder="Document name">

 <textarea id="editContent"
 placeholder="Start writing here..."></textarea>

</div>

<!-- RESET MODAL -->

<div id="resetModal" class="modal hidden">

 <div class="modalBox">

  <h3>⚠️ Confirm All Data Reset</h3>

  <p class="dangerText">
   All your data will be permanently erased.
   Your account will also be removed.
  </p>

  <input id="resetUser" placeholder="Current username"
  autocomplete="username">

  <input id="resetPass" type="password"
  placeholder="Current password"
  autocomplete="current-password">

  <label>Type RESET to confirm</label>

  <input id="resetWord" placeholder="Type RESET">

  <label class="checkLine">
   <input type="checkbox" id="resetCheck">
   I understand that all data will be erased.
  </label>

  <div class="modalActions">

   <button class="gray" onclick="closeResetModal()">
    Cancel
   </button>

   <button class="red" onclick="performReset()">
    Permanently Reset
   </button>

  </div>

  <p id="resetMsg"></p>

 </div>
</div>

<script>

const E=id=>document.getElementById(id);

let items=[];
let currentFolder="";
let editID="";
let editType="note";
let path=[];

// ---------------- API ----------------

function api(url,method,data,cb){

 let x=new XMLHttpRequest();

 x.open(method,url,true);
 x.setRequestHeader("Content-Type","application/json");

 x.onreadystatechange=function(){

  if(x.readyState!==4)return;

  let result={};

  try{
   result=JSON.parse(x.responseText);
  }catch(e){}

  if(x.status>=200 && x.status<300){
   cb(null,result);
  }else{
   cb(result.error || x.responseText ||
   ("HTTP Error "+x.status),result);
  }

 };

 x.onerror=function(){
  cb("Network connection failed",{});
 };

 x.send(data===null?null:JSON.stringify(data));
}

// ---------------- INITIALIZE ----------------

function initialize(){

 api("/api/status","GET",null,(e,d)=>{

  if(e){
   E("authMsg").innerText=e;
   return;
  }

  if(d.loggedIn){
   openApp();
  }else{
   E("authTitle").innerText=
    d.setup?"Create Account":"Login";
  }

 });

}

// ---------------- LOGIN ----------------

function login(){

 let u=E("username").value.trim();
 let p=E("password").value;

 api("/api/status","GET",null,(e,s)=>{

  if(e){
   alert(e);
   return;
  }

  if(s.setup && (u.length<3 || p.length<8)){
   E("authMsg").innerText=
   "Username minimum 3 and password minimum 8 characters";
   return;
  }

  let endpoint=s.setup?"/api/setup":"/api/login";

  api(endpoint,"POST",{
   username:u,
   password:p
  },err=>{

   if(err){
    E("authMsg").innerText=err;
    return;
   }

   openApp();

  });

 });

}

// ---------------- APP ----------------

function openApp(){

 E("auth").classList.add("hidden");
 E("app").classList.remove("hidden");

 showHome();
 loadSettings();
 loadStorage();

}

function showHome(){

 E("trash").classList.add("hidden");
 E("settings").classList.add("hidden");
 E("filesCard").classList.remove("hidden");
 E("storageCard").classList.remove("hidden");

 currentFolder="";
 path=[];

 loadItems();
 loadStorage();

}

// ---------------- LOAD ITEMS ----------------

function loadItems(){

 api("/api/items","GET",null,(e,d)=>{

  if(e){
   alert("File loading failed: "+e);
   return;
  }

  items=d.items||[];
  renderFiles();

 });

}

// ---------------- FILE MANAGER ----------------

function renderFiles(){

 let box=E("files");
 box.innerHTML="";

 let visible=items.filter(x=>
  !x.deleted &&
  String(x.parent||"")===currentFolder
 );

 visible.sort((a,b)=>{

  if(a.type===b.type){
   return a.title.localeCompare(b.title);
  }

  return a.type==="folder"?-1:1;

 });

 E("emptyMsg").innerText=
  visible.length?"":"This folder is empty.";

 visible.forEach(x=>{

  let el=document.createElement("div");
  el.className="file";

  let ico=document.createElement("div");
  ico.className="ico";
  ico.textContent=x.type==="folder"?"📁":"📄";

  let name=document.createElement("div");
  name.className="fname";
  name.textContent=x.title;

  el.append(ico,name);

  let actions=document.createElement("div");
  actions.className="actions";

  // OPEN BUTTON

  let open=document.createElement("button");
  open.textContent="OPEN";
  open.className="openBtn";

  open.onclick=()=>{

   if(x.type==="folder"){
    enterFolder(x.id,x.title);
   }else{
    openNote(x);
   }

  };

  // THREE DOT MENU

  let wrap=document.createElement("div");
  wrap.className="menuWrap";

  let menuBtn=document.createElement("button");
  menuBtn.className="menuBtn";
  menuBtn.textContent="⋮";

  let menu=document.createElement("div");
  menu.className="menuPanel";

  // DELETE BOX

  let trashBtn=document.createElement("button");
  trashBtn.textContent="Delete Box";
  trashBtn.className="gray";

  trashBtn.onclick=()=>{

   menu.style.display="none";

   if(confirm("Move "+x.title+" to Delete Box?")){
    moveToTrash(x.id);
   }

  };

  // PERMANENT DELETE

  let deleteBtn=document.createElement("button");
  deleteBtn.textContent="Delete";
  deleteBtn.className="red";

  deleteBtn.onclick=()=>{

   menu.style.display="none";

   if(confirm(
    "Permanently delete "+x.title+
    " and everything inside it? This cannot be undone."
   )){
    trashAction("/api/purge",x.id);
   }

  };

  menu.append(trashBtn,deleteBtn);

  menuBtn.onclick=(ev)=>{

   ev.stopPropagation();

   document.querySelectorAll(".menuPanel").forEach(m=>{
    if(m!==menu)m.style.display="none";
   });

   if(menu.style.display==="block"){
    menu.style.display="none";
    return;
   }

   // First open on the right.

   menu.style.left="calc(100% + 8px)";
   menu.style.right="auto";
   menu.style.display="block";

   requestAnimationFrame(()=>{

    let rect=menu.getBoundingClientRect();

    // Open left only if right side has no space.

    if(rect.right>window.innerWidth-8){
     menu.style.left="auto";
     menu.style.right="calc(100% + 8px)";
    }

   });

  };

  wrap.append(menuBtn,menu);

  actions.append(open,wrap);

  el.appendChild(actions);
  box.appendChild(el);

 });

 renderBreadcrumb();

}

// Close open menus when clicking outside.

document.addEventListener("click",()=>{

 document.querySelectorAll(".menuPanel").forEach(m=>{
  m.style.display="none";
 });

});

// ---------------- BREADCRUMB ----------------

function renderBreadcrumb(){

 let c=E("crumb");
 c.innerHTML="";

 let root=document.createElement("span");
 root.textContent="Home";

 root.onclick=()=>{
  currentFolder="";
  path=[];
  renderFiles();
 };

 c.appendChild(root);

 path.forEach((p,i)=>{

  c.appendChild(document.createTextNode(" / "));

  let s=document.createElement("span");
  s.textContent=p.title;

  s.onclick=()=>{
   path=path.slice(0,i+1);
   currentFolder=p.id;
   renderFiles();
  };

  c.appendChild(s);

 });

}

// ---------------- FOLDER ----------------

function enterFolder(id,title){

 currentFolder=id;
 path.push({id:id,title:title});

 renderFiles();

}

function goBack(){

 if(path.length){

  path.pop();

  currentFolder=
   path.length?path[path.length-1].id:"";

  renderFiles();

 }else{

  showHome();

 }

}

// ---------------- NEW ITEMS ----------------

function newFolder(){

 editID="";
 editType="folder";

 E("editTitle").value="";
 E("editContent").value="";

 E("editContent").classList.add("hidden");
 E("editTitle").placeholder="Folder name";

 E("editor").classList.remove("hidden");

}

function newNote(){

 editID="";
 editType="note";

 E("editTitle").value="";
 E("editContent").value="";

 E("editContent").classList.remove("hidden");
 E("editTitle").placeholder="Document name";

 E("editor").classList.remove("hidden");

}

function openNote(x){

 editID=x.id;
 editType="note";

 E("editTitle").value=x.title;
 E("editContent").value=x.content||"";

 E("editContent").classList.remove("hidden");
 E("editTitle").placeholder="Document name";

 E("editor").classList.remove("hidden");

}

function closeEditor(){

 E("editor").classList.add("hidden");

}

// ---------------- SAVE ITEM ----------------

function saveNote(){

 let title=E("editTitle").value.trim();

 if(!title){
  alert("Please enter a name");
  return;
 }

 let content=
  editType==="folder"?"":E("editContent").value;

 api("/api/save","POST",{

  id:editID,
  type:editType,
  title:title,
  content:content,
  parent:currentFolder

 },(e,d)=>{

  if(e){
   alert("Save failed: "+e);
   return;
  }

  closeEditor();

  loadItems();
  loadStorage();

  alert("Saved successfully");

 });

}

// ---------------- DELETE BOX ----------------

function moveToTrash(id){

 api("/api/trash","POST",{id:id},e=>{

  if(e){
   alert("Delete failed: "+e);
   return;
  }

  loadItems();
  loadStorage();

 });

}

function showTrash(){

 E("filesCard").classList.add("hidden");
 E("storageCard").classList.add("hidden");
 E("settings").classList.add("hidden");
 E("trash").classList.remove("hidden");

 api("/api/items","GET",null,(e,d)=>{

  if(e){
   alert(e);
   return;
  }

  let box=E("trashFiles");
  box.innerHTML="";

  let deleted=(d.items||[]).filter(x=>x.deleted);

  if(!deleted.length){
   box.innerText="Delete Box is empty.";
  }

  deleted.forEach(x=>{

   let div=document.createElement("div");
   div.className="file";

   let name=document.createElement("b");

   name.textContent=
    (x.type==="folder"?"📁 ":"📄 ")+x.title;

   let restore=document.createElement("button");
   restore.textContent="Restore";
   restore.className="green";

   restore.onclick=()=>{
    trashAction("/api/restore",x.id);
   };

   let del=document.createElement("button");
   del.textContent="Delete Permanently";
   del.className="red";

   del.onclick=()=>{

    if(confirm(
     "Permanently delete this item and its contents?"
    )){
     trashAction("/api/purge",x.id);
    }

   };

   div.append(
    name,
    document.createElement("br"),
    restore,
    del
   );

   box.appendChild(div);

  });

 });

}

function trashAction(url,id){

 api(url,"POST",{id:id},e=>{

  if(e){
   alert(e);
   return;
  }

  showTrash();
  loadStorage();

 });

}

function emptyTrash(){

 if(!confirm(
  "Permanently delete all items in Delete Box?"
 ))return;

 api("/api/empty","POST",{},e=>{

  if(e){
   alert(e);
   return;
  }

  showTrash();
  loadStorage();

 });

}

// ---------------- SETTINGS ----------------

function showSettings(){

 E("filesCard").classList.add("hidden");
 E("storageCard").classList.add("hidden");
 E("trash").classList.add("hidden");
 E("settings").classList.remove("hidden");

}

function loadSettings(){

 api("/api/settings","GET",null,(e,d)=>{

  if(e)return;

  E("theme").value=d.theme;
  E("accent").value=d.accent;

  applyTheme(d.theme,d.accent);

 });

}

function applyTheme(t,c){

 document.body.classList.toggle("dark",t==="dark");

 document.documentElement.style.setProperty("--a",c);

}

function saveSettings(){

 let t=E("theme").value;
 let c=E("accent").value;

 api("/api/settings","POST",{
  theme:t,
  accent:c
 },e=>{

  if(e){
   alert(e);
   return;
  }

  applyTheme(t,c);

  alert("Settings saved");

 });

}

// ---------------- CHANGE PASSWORD ----------------

function changePassword(){

 let oldp=E("oldPass").value;
 let newp=E("newPass").value;

 api("/api/password","POST",{
  oldpass:oldp,
  newpass:newp
 },e=>{

  if(e){
   alert(e);
   return;
  }

  alert("Password changed");

  E("oldPass").value="";
  E("newPass").value="";

 });

}

// ---------------- BACKUP ----------------

function backup(){

 let a=document.createElement("a");

 a.href="/api/export";
 a.download="TechnicalBDServer.json";

 document.body.appendChild(a);
 a.click();
 a.remove();

}

// ---------------- ALL DATA RESET ----------------

function openResetModal(){

 E("resetUser").value="";
 E("resetPass").value="";
 E("resetWord").value="";
 E("resetCheck").checked=false;
 E("resetMsg").innerText="";

 E("resetModal").classList.remove("hidden");

}

function closeResetModal(){

 E("resetModal").classList.add("hidden");

}

function performReset(){

 let username=E("resetUser").value.trim();
 let password=E("resetPass").value;
 let word=E("resetWord").value.trim();

 if(!username || !password){
  E("resetMsg").innerText=
   "Enter your current username and password.";
  return;
 }

 if(word!=="RESET"){
  E("resetMsg").innerText=
   "Please type RESET correctly.";
  return;
 }

 if(!E("resetCheck").checked){
  E("resetMsg").innerText=
   "Please confirm that you understand.";
  return;
 }

 if(!confirm(
  "FINAL WARNING: All files, folders, account data and settings will be erased. Continue?"
 ))return;

 E("resetMsg").innerText="Verifying and resetting...";

 api("/api/reset","POST",{
  username:username,
  password:password,
  confirm:"RESET"
 },(e,d)=>{

  if(e){
   E("resetMsg").innerText=e;
   return;
  }

  E("resetMsg").innerText=
   "Reset successful. ESP32 is restarting...";

  setTimeout(()=>{
   location.reload();
  },4000);

 });

}

// ---------------- LOGOUT ----------------

function logout(){

 api("/api/logout","POST",{},()=>{
  location.reload();
 });

}

// ---------------- STORAGE ----------------

function loadStorage(){

 api("/api/storage","GET",null,(e,d)=>{

  if(e){
   E("storageText").innerText=
    "Storage information unavailable: "+e;
   return;
  }

  let total=d.total||0;
  let used=d.used||0;
  let free=d.free||0;

  let percent=total?used*100/total:0;

  E("storageUsed").style.width=percent+"%";

  E("storageText").textContent=
   "Total: "+formatBytes(total)+
   " | Used: "+formatBytes(used)+
   " | Free: "+formatBytes(free)+
   " | "+percent.toFixed(1)+"% used";

 });

}

function formatBytes(n){

 if(n<1024)return n+" B";

 if(n<1048576){
  return (n/1024).toFixed(1)+" KB";
 }

 return (n/1048576).toFixed(2)+" MB";

}

initialize();

</script>
</body>
</html>
)HTML";

// ---------------- SHA256 ----------------

String sha256Hex(String input){

 uint8_t hash[32];

 mbedtls_sha256(
  (const unsigned char*)input.c_str(),
  input.length(),
  hash,
  0
 );

 const char h[]="0123456789abcdef";

 String out="";

 for(int i=0;i<32;i++){
  out+=h[hash[i]>>4];
  out+=h[hash[i]&15];
 }

 return out;

}

// ---------------- ID ----------------

String makeID(){

 char b[17];

 snprintf(
  b,sizeof(b),"%08lx%08lx",
  (unsigned long)esp_random(),
  (unsigned long)esp_random()
 );

 return String(b);

}

// ---------------- AUTH ----------------

bool loggedIn(){

 if(sessionID=="")return false;

 if(!server.hasHeader("Cookie"))return false;

 String c=server.header("Cookie");

 return c.indexOf("session="+sessionID)>=0;

}

bool auth(){

 if(loggedIn())return true;

 server.send(
  401,
  "application/json",
  "{\"error\":\"Login required. Please login again.\"}"
 );

 return false;

}

void setCookie(){

 server.sendHeader(
  "Set-Cookie",
  "session="+sessionID+
  "; HttpOnly; SameSite=Lax; Path=/"
 );

}

// ---------------- JSON HELPERS ----------------

bool readJSON(DynamicJsonDocument &d){

 if(!server.hasArg("plain"))return false;

 DeserializationError e=
  deserializeJson(d,server.arg("plain"));

 return !e;

}

void sendError(int code,String message){

 DynamicJsonDocument d(512);

 d["error"]=message;

 String out;

 serializeJson(d,out);

 server.send(code,"application/json",out);

}

// ---------------- DATABASE SAVE ----------------

bool saveDB(){

 if(!fsReady)return false;

 if(db.overflowed())return false;

 if(LittleFS.exists(TMP_FILE)){
  LittleFS.remove(TMP_FILE);
 }

 File temp=LittleFS.open(TMP_FILE,"w");

 if(!temp)return false;

 size_t expected=measureJson(db);
 size_t written=serializeJson(db,temp);

 temp.flush();
 temp.close();

 if(written==0 || written!=expected){
  LittleFS.remove(TMP_FILE);
  return false;
 }

 File check=LittleFS.open(TMP_FILE,"r");

 if(!check){
  LittleFS.remove(TMP_FILE);
  return false;
 }

 DynamicJsonDocument verify(30000);

 DeserializationError err=deserializeJson(verify,check);

 check.close();

 if(err || !verify["items"].is<JsonArray>()){
  LittleFS.remove(TMP_FILE);
  return false;
 }

 if(LittleFS.exists(BAK_FILE)){
  LittleFS.remove(BAK_FILE);
 }

 if(LittleFS.exists(DB_FILE)){

  if(!LittleFS.rename(DB_FILE,BAK_FILE)){
   LittleFS.remove(TMP_FILE);
   return false;
  }

 }

 if(!LittleFS.rename(TMP_FILE,DB_FILE)){

  if(LittleFS.exists(BAK_FILE)){
   LittleFS.rename(BAK_FILE,DB_FILE);
  }

  return false;

 }

 return true;

}

// ---------------- DATABASE LOAD ----------------

bool loadDB(){

 db.clear();

 if(!fsReady)return false;

 const char* candidates[]={DB_FILE,BAK_FILE};

 for(int i=0;i<2;i++){

  if(!LittleFS.exists(candidates[i]))continue;

  File f=LittleFS.open(candidates[i],"r");

  if(!f)continue;

  db.clear();

  DeserializationError err=deserializeJson(db,f);

  f.close();

  if(!err && db["items"].is<JsonArray>()){

   Serial.print("Database loaded: ");
   Serial.println(candidates[i]);

   return true;

  }

 }

 db.clear();

 db.createNestedArray("items");

 return saveDB();

}

// ---------------- PREFERENCES ----------------

void loadPreferences(){

 prefs.begin("vault",false);

 vaultUser=prefs.getString("username","");
 vaultHash=prefs.getString("passhash","");
 vaultSalt=prefs.getString("salt","");

 currentTheme=prefs.getString("theme","light");
 currentAccent=prefs.getString("accent","#2563eb");

}

void savePreferences(){

 prefs.putString("username",vaultUser);
 prefs.putString("passhash",vaultHash);
 prefs.putString("salt",vaultSalt);

 prefs.putString("theme",currentTheme);
 prefs.putString("accent",currentAccent);

}

// ---------------- STATUS ----------------

void handleStatus(){

 DynamicJsonDocument d(256);

 d["setup"]=vaultUser.length()==0;
 d["loggedIn"]=loggedIn();

 String out;

 serializeJson(d,out);

 server.send(200,"application/json",out);

}

// ---------------- ACCOUNT SETUP ----------------

void handleSetup(){

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 if(vaultUser.length()){
  sendError(403,"Account already exists");
  return;
 }

 String u=d["username"]|"";
 String p=d["password"]|"";

 if(u.length()<3 || p.length()<8){
  sendError(
   400,
   "Username minimum 3 and password minimum 8 characters"
  );
  return;
 }

 vaultUser=u;
 vaultSalt=makeID();
 vaultHash=sha256Hex(vaultSalt+p);

 savePreferences();

 sessionID=makeID();

 setCookie();

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- LOGIN ----------------

void handleLogin(){

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String u=d["username"]|"";
 String p=d["password"]|"";

 if(u!=vaultUser || sha256Hex(vaultSalt+p)!=vaultHash){
  sendError(401,"Wrong username or password");
  return;
 }

 sessionID=makeID();

 setCookie();

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- LOGOUT ----------------

void handleLogout(){

 sessionID="";

 server.sendHeader(
  "Set-Cookie",
  "session=; Max-Age=0; Path=/"
 );

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- ITEMS ----------------

void handleItems(){

 if(!auth())return;

 String out="{\"items\":";

 serializeJson(db["items"],out);

 out+="}";

 server.send(200,"application/json",out);

}

// ---------------- VALID PARENT ----------------

bool validParent(String parent){

 if(parent=="")return true;

 for(JsonObject x:db["items"].as<JsonArray>()){

  if(String(x["id"]|"")==parent &&
     String(x["type"]|"")=="folder" &&
     !(x["deleted"]|false)){

   return true;

  }

 }

 return false;

}

// ---------------- SAVE ITEM ----------------

void handleSave(){

 if(!auth())return;

 DynamicJsonDocument d(4096);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String id=d["id"]|"";
 String type=d["type"]|"note";
 String title=d["title"]|"";
 String content=d["content"]|"";
 String parent=d["parent"]|"";

 if(title.length()==0){
  sendError(400,"Document name is empty");
  return;
 }

 if(type!="folder" && type!="note"){
  sendError(400,"Invalid item type");
  return;
 }

 if(!validParent(parent)){
  sendError(400,"Parent folder does not exist");
  return;
 }

 if(content.length()>12000){
  sendError(413,"Text is too large");
  return;
 }

 JsonArray arr=db["items"].as<JsonArray>();

 JsonObject target;

 bool found=false;

 if(id.length()){

  for(JsonObject x:arr){

   if(String(x["id"]|"")==id){

    if(x["deleted"]|false){
     sendError(400,"This item is in Delete Box");
     return;
    }

    target=x;
    found=true;
    break;

   }

  }

 }

 if(!found){

  target=arr.createNestedObject();

  if(target.isNull()){
   sendError(500,"Not enough database memory");
   return;
  }

  target["id"]=makeID();

 }

 target["type"]=type;
 target["title"]=title;
 target["content"]=type=="folder"?"":content;
 target["parent"]=parent;
 target["deleted"]=false;

 if(!saveDB()){
  sendError(500,"Database write failed. Check LittleFS storage.");
  return;
 }

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- MOVE TO DELETE BOX ----------------

void handleTrash(){

 if(!auth())return;

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String id=d["id"]|"";

 bool found=false;

 for(JsonObject x:db["items"].as<JsonArray>()){

  if(String(x["id"]|"")==id){

   x["deleted"]=true;
   found=true;
   break;

  }

 }

 if(!found){
  sendError(404,"Item not found");
  return;
 }

 if(!saveDB()){
  sendError(500,"Could not save Delete Box");
  return;
 }

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- RESTORE ----------------

void handleRestore(){

 if(!auth())return;

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String id=d["id"]|"";

 bool found=false;

 for(JsonObject x:db["items"].as<JsonArray>()){

  if(String(x["id"]|"")==id){

   x["deleted"]=false;
   found=true;
   break;

  }

 }

 if(!found){
  sendError(404,"Item not found");
  return;
 }

 if(!saveDB()){
  sendError(500,"Restore could not be saved");
  return;
 }

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- PERMANENT DELETE ----------------

void handlePurge(){

 if(!auth())return;

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String rootID=d["id"]|"";

 if(rootID==""){
  sendError(400,"Invalid item ID");
  return;
 }

 JsonArray arr=db["items"].as<JsonArray>();

 bool found=false;

 // Collect root and all descendant IDs.

 std::vector<String> removeIDs;

 removeIDs.push_back(rootID);

 for(int pass=0;pass<30;pass++){

  bool changed=false;

  for(JsonObject x:arr){

   String parent=x["parent"]|"";

   bool parentFound=false;

   for(const String &rid:removeIDs){

    if(parent==rid){
     parentFound=true;
     break;
    }

   }

   if(parentFound){

    String childID=x["id"]|"";

    bool already=false;

    for(const String &rid:removeIDs){

     if(rid==childID){
      already=true;
      break;
     }

    }

    if(!already && childID.length()){
     removeIDs.push_back(childID);
     changed=true;
    }

   }

  }

  if(!changed)break;

 }

 // Remove all collected items.

 for(int i=(int)arr.size()-1;i>=0;i--){

  JsonObject x=arr[i];

  String itemID=x["id"]|"";

  for(const String &rid:removeIDs){

   if(itemID==rid){

    arr.remove(i);
    found=true;
    break;

   }

  }

 }

 if(!found){
  sendError(404,"Item not found");
  return;
 }

 if(!saveDB()){
  sendError(500,"Permanent delete failed to save");
  return;
 }

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- EMPTY DELETE BOX ----------------

void handleEmpty(){

 if(!auth())return;

 JsonArray arr=db["items"].as<JsonArray>();

 // Collect IDs of deleted items and their descendants.

 std::vector<String> removeIDs;

 for(JsonObject x:arr){

  if(x["deleted"]|false){

   String id=x["id"]|"";

   if(id.length()){
    removeIDs.push_back(id);
   }

  }

 }

 for(int pass=0;pass<30;pass++){

  bool changed=false;

  for(JsonObject x:arr){

   String parent=x["parent"]|"";

   for(const String &rid:removeIDs){

    if(parent==rid){

     String id=x["id"]|"";

     bool exists=false;

     for(const String &oldID:removeIDs){

      if(oldID==id){
       exists=true;
       break;
      }

     }

     if(!exists && id.length()){
      removeIDs.push_back(id);
      changed=true;
     }

     break;

    }

   }

  }

  if(!changed)break;

 }

 for(int i=(int)arr.size()-1;i>=0;i--){

  String id=arr[i]["id"]|"";

  for(const String &rid:removeIDs){

   if(id==rid){
    arr.remove(i);
    break;
   }

  }

 }

 if(!saveDB()){
  sendError(500,"Could not empty Delete Box");
  return;
 }

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- STORAGE ----------------

void handleStorage(){

 if(!auth())return;

 DynamicJsonDocument d(256);

 uint32_t total=0;
 uint32_t used=0;

 if(fsReady){
  total=LittleFS.totalBytes();
  used=LittleFS.usedBytes();
 }

 d["total"]=total;
 d["used"]=used;
 d["free"]=total>used?total-used:0;
 d["ready"]=fsReady;

 String out;

 serializeJson(d,out);

 server.send(200,"application/json",out);

}

// ---------------- GET SETTINGS ----------------

void handleGetSettings(){

 if(!auth())return;

 DynamicJsonDocument d(256);

 d["theme"]=currentTheme;
 d["accent"]=currentAccent;

 String out;

 serializeJson(d,out);

 server.send(200,"application/json",out);

}

// ---------------- SAVE SETTINGS ----------------

void handleSaveSettings(){

 if(!auth())return;

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String t=d["theme"]|"light";
 String c=d["accent"]|"#2563eb";

 if(t!="light" && t!="dark"){
  t="light";
 }

 if(c.length()!=7 || c[0]!='#'){
  c="#2563eb";
 }

 currentTheme=t;
 currentAccent=c;

 savePreferences();

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- CHANGE PASSWORD ----------------

void handlePassword(){

 if(!auth())return;

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String oldp=d["oldpass"]|"";
 String newp=d["newpass"]|"";

 if(sha256Hex(vaultSalt+oldp)!=vaultHash){
  sendError(401,"Wrong current password");
  return;
 }

 if(newp.length()<8){
  sendError(400,"New password must be at least 8 characters");
  return;
 }

 vaultSalt=makeID();
 vaultHash=sha256Hex(vaultSalt+newp);

 savePreferences();

 server.send(200,"application/json","{\"ok\":true}");

}

// ---------------- EXPORT ----------------

void handleExport(){

 if(!auth())return;

 if(!fsReady){
  sendError(500,"LittleFS unavailable");
  return;
 }

 File f=LittleFS.open(DB_FILE,"r");

 if(!f){
  sendError(500,"Backup file not found");
  return;
 }

 server.sendHeader(
  "Content-Disposition",
  "attachment; filename=TechnicalBDServer.json"
 );

 server.streamFile(f,"application/json");

 f.close();

}

// ---------------- ALL DATA RESET ----------------

void handleReset(){

 // Reset request must contain valid JSON.

 DynamicJsonDocument d(512);

 if(!readJSON(d)){
  sendError(400,"Invalid JSON");
  return;
 }

 String u=d["username"]|"";
 String p=d["password"]|"";
 String confirmWord=d["confirm"]|"";

 // Server-side credential verification.

 if(u!=vaultUser ||
    sha256Hex(vaultSalt+p)!=vaultHash){

  sendError(401,"Incorrect username or password");
  return;

 }

 if(confirmWord!="RESET"){
  sendError(400,"Reset confirmation is missing");
  return;
 }

 if(!fsReady){
  sendError(500,"LittleFS is unavailable. Reset cancelled.");
  return;
 }

 // First send response, then erase data and restart.

 server.send(
  200,
  "application/json",
  "{\"ok\":true,\"message\":\"Reset accepted. Restarting.\"}"
 );

 delay(500);

 Serial.println();
 Serial.println("================================");
 Serial.println("TECHNICAL BD SERVER DATA RESET");
 Serial.println("================================");

 // Clear persistent file storage.

 bool formatOK=LittleFS.format();

 Serial.print("LittleFS format: ");
 Serial.println(formatOK?"SUCCESS":"FAILED");

 // Clear account and settings storage.

 prefs.clear();
 prefs.end();

 sessionID="";
 vaultUser="";
 vaultHash="";
 vaultSalt="";

 Serial.println("Preferences cleared.");
 Serial.println("Restarting ESP32...");

 delay(700);

 ESP.restart();

}

// ---------------- ROOT ----------------

void handleRoot(){

 server.send_P(200,"text/html",PAGE);

}

// ---------------- SETUP ----------------

void setup(){

 Serial.begin(115200);

 delay(1000);

 Serial.println();
 Serial.println("Starting Technical BD Server");

 // WiFi Access Point

 WiFi.mode(WIFI_AP);
 WiFi.setSleep(false);

 bool ap=WiFi.softAP(
  AP_SSID,
  AP_PASS,
  1,
  0,
  4
 );

 if(!ap){

  Serial.println("WiFi AP FAILED");
  return;

 }

 Serial.println("WiFi AP STARTED");

 Serial.print("SSID: ");
 Serial.println(AP_SSID);

 Serial.print("Password: ");
 Serial.println(AP_PASS);

 Serial.print("IP: ");
 Serial.println(WiFi.softAPIP());

 // LittleFS

 fsReady=LittleFS.begin(false);

 if(!fsReady){

  Serial.println("LittleFS mount failed.");
  Serial.println("Automatic formatting disabled.");

 }else{

  Serial.println("LittleFS READY");

 }

 // Preferences

 loadPreferences();

 // Database

 if(fsReady){

  if(!loadDB()){
   Serial.println("Database load failed.");
  }

 }else{

  db.createNestedArray("items");

 }

 // Cookie header

 const char* headers[]={"Cookie"};

 server.collectHeaders(headers,1);

 // Routes

 server.on("/",HTTP_GET,handleRoot);

 server.on("/api/status",HTTP_GET,handleStatus);

 server.on("/api/setup",HTTP_POST,handleSetup);
 server.on("/api/login",HTTP_POST,handleLogin);
 server.on("/api/logout",HTTP_POST,handleLogout);

 server.on("/api/items",HTTP_GET,handleItems);
 server.on("/api/save",HTTP_POST,handleSave);

 server.on("/api/trash",HTTP_POST,handleTrash);
 server.on("/api/restore",HTTP_POST,handleRestore);
 server.on("/api/purge",HTTP_POST,handlePurge);
 server.on("/api/empty",HTTP_POST,handleEmpty);

 server.on("/api/storage",HTTP_GET,handleStorage);

 server.on("/api/settings",HTTP_GET,handleGetSettings);
 server.on("/api/settings",HTTP_POST,handleSaveSettings);

 server.on("/api/password",HTTP_POST,handlePassword);

 server.on("/api/export",HTTP_GET,handleExport);

 server.on("/api/reset",HTTP_POST,handleReset);

 server.onNotFound([](){

  server.send(
   404,
   "application/json",
   "{\"error\":\"API endpoint not found\"}"
  );

 });

 server.begin();

 Serial.println("HTTP SERVER STARTED");
 Serial.println("Open http://192.168.4.1");

}

// ---------------- LOOP ----------------

void loop(){

 server.handleClient();

}
