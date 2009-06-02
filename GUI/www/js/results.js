
function hideElement(el) {
	if (el) 
		el.style.display='none';
}

function showElement(el) {
	if (el) 
		el.style.display='';
}

function hideElements(els) {
	if (els) {
		for (var i = 0; i < els.length; i++) 
			hideElement(els[i]);
	}
}

function showElements(els) {
	if (els) {
		for (var i = 0; i < els.length; i++) 
			showElement(els[i]);
	}
}
