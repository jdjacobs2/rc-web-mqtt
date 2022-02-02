var socket;
var submitButton;
var socketStatus;
var listMsgs;

const submit = function (e) {
  e.preventDefault();
  console.log('in submit handler');

  // Recovering the message of the textarea.
  let msg =
    document.getElementById('device').value +
    ',' +
    document.getElementById('function').value;

  // Sending the msg via WebSocket.
  console.log('Sent msg:  ', msg);
  socket.send(msg);

  // Adding the msg in a list of sent messages.
  // litsMsgs.innerHTML += '<li class="sent"><span>Sent:</span>' + msg + '</li>';

  // Cleaning up the field after sending.
  // txtMsg.value = '';

  return false;
};

window.onload = function () {
  // get the references of the page elements.
  console.log('in onload');
  document.getElementById('submit').addEventListener('click', submit);
  // var txtMsg = document.getElementById('msg');
  listMsgs = document.getElementById('msgs');
  socketStatus = document.getElementById('status');
  var btnClose = document.getElementById('close');
  socket = new WebSocket("ws://10.0.0.26/ws");
  socket.onopen = onOpen;
  socket.onclose = onClose;
  socket.onmessage = onMessage

  // submitButton.addEventListener('click', submit);
};

onOpen = function (event) {
  socketStatus.innerHTML = 'Connected to: ' + event.currentTarget.URL;
  socketStatus.className = 'open';
};

// socket.onerror = function (error) {
//   console.log('WebSocket error: ' + error);
// };


onMessage = function (event) {
  console.log("in onmessage event.date =  ", event.data);
  var msg = event.data;
  listMsgs.innerHTML +=
    '<li class="received"><span>Received:</span>' + msg + '</li>';
};

onClose = function (event) {
  socketStatus.innerHTML = 'Disconnected from the WebSocket.';
  socketStatus.className = 'closed';
};
