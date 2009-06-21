<?php 
// Set up the include paths
set_include_path( get_include_path().PATH_SEPARATOR.'../php' );

// Require the support files
require_once 'defines.inc.php';
require_once 'functions.inc.php';
require_once 'wow_functions.inc.php';

// Set the error and exception handler
set_exception_handler('custom_exception_handler');


// If the 'simulate' button was pressed, run the simulation
if( isset($_POST['simulate']) && ALLOW_SIMULATION===true ) {

	// Develop the simcraft command from the form input, with a random file name for the output catcher
	$output_file = tempnam('/tmp', 'simcraft_output');
	$simcraft_command = generate_simcraft_command( $_POST, $output_file );
	
	// Change to the simulationcraft directory
	chdir( get_valid_path(SIMULATIONCRAFT_PATH) );
	
	// Call the simcraft execution
	$simcraft_output = shell_exec( $simcraft_command );

	// Fetch the result as an XML object
	$xml = new SimpleXMLElement_XSL(file_get_contents($output_file));
	
	// Send the page header for xml content
	header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
	header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT'); // always modified
	header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
	header('Cache-Control: post-check=0, pre-check=0', false); // HTTP/1.1
	header('Pragma: no-cache'); // HTTP/1.0
	header('Content-type: text/xml');
	
	// Send the output string
	echo $xml->asXML_with_XSL('xsl/results.xsl');
}


// Else, if the request was to export the config file
else if( isset($_POST['export_file']) ) {

	// Build the output string
	$output = build_config_file_from_array($_POST);
	
	// Send the page header and the file content
	header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
	header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT'); // always modified
	header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
	header('Cache-Control: post-check=0, pre-check=0', false); // HTTP/1.1
	header('Pragma: no-cache'); // HTTP/1.0
	header('Content-Disposition: attachment; filename="'.date('YmdHis').'.simcraft"');
	header("Content-length: " . strlen($output) );
	header('Content-type: text/text');
	print $output;	
}


// Else, show the simulation configuration form, and handle any requests to mod that form's content
else {

	// Create the output XML object
	$xml = new SimpleXMLElement_XSL('<xml></xml>');
		
	// Add the simulation config form XML to the outgoing XML object
	append_simulation_config_form($xml);

	// Add the wow servers
	add_wow_servers($xml);
		
	// If a request was passed to add a raid member from the armory
	if( isset($_POST['import_from_armory']) ) {
		
		// Re-set any submitted form values (to preserve the form's state)
		set_values_from_array($xml, $_POST);
		
		// Add the raider, importing them from the web
		add_raider_from_web($xml, $_POST['add_from_armory']['name'], $_POST['add_from_armory']['server'] );
		
		// Add the 'selected' server to the xml, for conveniently re-setting the selected value after reload
		$xml->options->addAttribute('selected_server', $_POST['add_from_armory']['server']);
	}
	
	
	// Else, if a file import was requested, import the file's contents
	else if( isset($_POST['import_file']) ) {
		
		// Unless we should clear the form before importing, go ahead and load the submitted field values again 
		if(!$_POST['clear_before_import']) {
			set_values_from_array($xml, $_POST);
		}
		
		// Set the values from the uploaded file
		set_values_from_uploaded_file($xml);
	}
	
	// Else, if any post values were submitted, set them in the form
	else if( !empty($_POST) ) {
		set_values_from_array($xml, $_POST);
	}
	
	// else just use the default values
	else {
		set_default_values($xml);
	}
		
	
	// Send the page header for xml content
	header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
	header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT'); // always modified
	header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
	header('Cache-Control: post-check=0, pre-check=0', false); // HTTP/1.1
	header('Pragma: no-cache'); // HTTP/1.0
	header('Content-type: text/xml');
	
	// Send the output string
	echo $xml->asXML_with_XSL('xsl/config_form.xsl');
}
?>