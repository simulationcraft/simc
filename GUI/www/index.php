<?php 
// Set up the include paths
set_include_path( get_include_path().PATH_SEPARATOR.'../php' );

// Require the support files
require_once 'defines.inc.php';
require_once 'functions.inc.php';

// Set the error and exception handler
set_exception_handler('custom_exception_handler');


// If the 'simulate' button was pressed, run the simulation
if( isset($_POST['simulate']) && ALLOW_SIMULATION===true ) {

	// Develop the simulationcraft command from the form input
	$simulationcraft_command = generate_simulationcraft_command( $_POST );
	
	// Execute the command
	list($file_contents, $simulationcraft_output) = execute_simulationcraft_command($simulationcraft_command);
	$file_contents = str_replace('&amp;amp;', '&amp;', str_replace('&','&amp;', $file_contents));
	
	// Fetch the result as an XML object, and release it to the browser as output
	try {
		$xml = new SimpleXMLElement_XSL($file_contents);
		$xml->release_to_browser('xsl/results.xsl');
	}
	catch( Exception $e) {
		throw new Exception("Simulationcraft did not return valid XML, caused problems ({$e->getMessage()}).\n\nsimulationcraft command:\n$simulationcraft_command\n\nsimulationcraft STDOUT:\n$simulationcraft_output\n\nsimulationcraft file content:\n$file_contents");
	}	
}


// Else, if the request was to export the config file
else if( isset($_POST['export_file']) ) {

	// Send the page headers to go with a text file
	header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
	header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT'); // always modified
	header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
	header('Cache-Control: post-check=0, pre-check=0', false); // HTTP/1.1
	header('Pragma: no-cache'); // HTTP/1.0
	header('Content-Disposition: attachment; filename="'.date('YmdHis').'.simc"');
	header("Content-length: " . strlen($output) );
	header('Content-type: text/text');
	
	// Build the output string and send it
	echo build_file_from_config_array($_POST);
}


// Else, show the simulation configuration form, and handle any requests to mod that form's content
else {

	// Create the output XML object
	$xml = XML_SimulationcraftConfigForm::factory();
		
	
	// If a request was passed to add a raid member from the armory
	if( isset($_POST['import_from_armory']) ) {
		
		// Re-set any submitted form values (to preserve the form's state)
		$xml->set_option_values_from_array($_POST);
		
		// Add the raider, importing them from the web
		$xml->add_raider_from_web( $_POST['add_from_armory']['name'], $_POST['add_from_armory']['server'] );
		
		// Add the 'selected' server to the xml, for conveniently re-setting the selected value after reload
		$xml->options->addAttribute('selected_server', $_POST['add_from_armory']['server']);
	}
	
	
	// Else, if a file import was requested, import the file's contents
	else if( isset($_POST['import_file']) ) {
		
		// Unless we should clear the form before importing, go ahead and load the submitted field values again 
		if(!$_POST['clear_before_import']) {
			$xml->set_option_values_from_array($_POST);
		}
		
		// Set the values from the uploaded file
		$xml->set_option_values_from_file($_FILES['import_file_path']['tmp_name']);
	}
	
	
	// Else, if any post values were submitted, set them in the form
	else if( !empty($_POST) ) {
		$xml->set_option_values_from_array($_POST);
	}
	
	
	// else just use the default values
	else {
		$xml->set_default_option_values();
	}
		

	// Send the page content to the end user
	$xml->release_to_browser('xsl/config_form.xsl');	
}
?>