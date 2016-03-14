//===== NFC cards

function changeNFC(e) {
  e.preventDefault();
  var url = "nfc?1=1";
  var i, inputs = document.querySelectorAll('#nfc-form input');
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type != "checkbox")
      url += "&" + inputs[i].name + "=" + inputs[i].value;
  };

  hideWarning();
  var cb = $("#nfc-button");
  addClass(cb, 'pure-button-disabled');
  ajaxSpin("POST", url, function (resp) {
    showNotification("NFC updated");
    removeClass(cb, 'pure-button-disabled');
  }, function (s, st) {
    showWarning("Error: " + st);
    removeClass(cb, 'pure-button-disabled');
    window.setTimeout(fetchNFC, 100);
  });
}

function displayNFC(data) {
  Object.keys(data).forEach(function (v) {
    el = $("#" + v);
    if (el != null) {
      if (el.nodeName === "INPUT") el.value = data[v];
      else el.innerHTML = data[v];
      return;
    }
    el = document.querySelector('input[name="' + v + '"]');
    if (el != null) {
      if (el.type == "checkbox") el.checked = data[v] > 0;
      else el.value = data[v];
    }
  });
  $("#nfc-spinner").setAttribute("hidden", "");
  $("#nfc-form").removeAttribute("hidden");

  var i, inputs = $("input");
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type == "checkbox")
      inputs[i].onclick = function () { setNFC(this.name, this.checked) };
  }
}

function fetchNFC() {
  ajaxJson("GET", "/nfc", displayNFC, function () {
    window.setTimeout(fetchNFC, 1000);
  });
}

function setNFC(name, v) {
  ajaxSpin("POST", "/nfc?" + name + "=" + (v ? 1 : 0), function () {
    var n = name.replace("-enable", "");
    showNotification(n + " is now " + (v ? "enabled" : "disabled"));
  }, function () {
    showWarning("Enable/disable failed");
    window.setTimeout(fetchNFC, 100);
  });
}
