//Copyright (C) 2016 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.

function initExample() {
var menItm = `
	<tr><td width=1><input type=submit onclick="ShowHideEvent( 'Example' );" value="Example"></td><td>
	<div id=Example class="collapsible">
	<p>I'm an example feature found in ./web/page/feature.example.js.</p>
	<input type=button id=InfoBtn value="Display Info"><br>
	<p id=InfoDspl>&nbsp;</p>
	</div>
	</td></tr>
`;
	$('#MainMenu > tbody:last').after( menItm );

	$('#InfoBtn').click( function(e) {
		$('#InfoBtn').val('Getting data...');
		$('#InfoDspl').html('&nbsp;');
		QueueOperation( "I", clbInfoBtn ); // Send info request to ESP
	});
}

window.addEventListener("load", initExample, false);


// Handle request previously sent on button click
function clbInfoBtn(req,data) {
	$('#InfoBtn').val('Display Info');
	$('#InfoDspl').html('Info returned from esp:<br>'+ data);
}
