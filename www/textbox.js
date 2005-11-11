
function focus_text(textInput, defval) {
    if (textInput.value == defval) {
	textInput.value = "";
	textInput.className = "textInput";
    }
}

function blur_text(textInput, defval) {
    // Strip whitespace.
    var sval = textInput.value.replace(/(\s+)/ig, "");
    if (sval == "") {
	textInput.value = defval;
	textInput.className = "textInputEmpty";
    }
}
