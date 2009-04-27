<?php 
// Set up the include paths
set_include_path( get_include_path().PATH_SEPARATOR.'../php' );

// Require the support files
require_once 'defines.inc.php';
require_once 'functions.inc.php';
require_once 'wow_functions.inc.php';


// Create the output XML object
$xml = new SimpleXMLElement_XSL('<xml></xml>');


// If the 'simulate' button was pressed, run the simulation
if( isset($_POST['simulate']) ) {

	// Develop the simcraft command from the form input, with a random file name for the output catcher
	$output_file = tempnam('/tmp', 'simcraft_output');
	$simcraft_command = generate_simcraft_command($_POST, $output_file );
	
	// Change to the simulationcraft directory
	chdir( get_valid_path(SIMULATIONCRAFT_PATH) );
	
	// Call the simcraft execution
	$simcraft_output = shell_exec( $simcraft_command );
	
//	// Add the simcraft command to the outgoing xml, just for documentation
//	$xml->addChild('simcraft_command', htmlentities($simcraft_command) );
//	
//	// Add the response to the outgoing xml
//	$xml->addChild('command_return', $simcraft_output);
//	
//	// Load the generated report output file
//	$file_contents = file_get_contents($output_file);
//	
//	// Add the generated report
//	$xml->addChild('generated_report', $file_contents );
//	
//	// Define the XSL that will style this XML
//	$xsl_path = 'xsl/result_display.xsl';

	// FIXME temporarily, just print the html, since the generated html isn't XHTML compliant and won't fit in an XML file
	print file_get_contents($output_file);
	die();
}

// Else, just show the simulation configuration form
else {
	
	// Define the XSL that will style this XML
	$xsl_path = 'xsl/config_form.xsl';
		
	// Add the simulation config form XML to the outgoing XML object
	append_simulation_config_form($xml, $_POST);
		
	// Add the wow servers
	$arr_servers = get_arr_wow_servers();
	$xml_servers = $xml->addChild('servers');
	foreach( $arr_servers as $arr_server ) {
		$new_server = $xml_servers->addChild('server');
		$new_server->addAttribute('name', $arr_server['name']);
	}
		
	// If a request was passed to add a raid member from the armory, do it now
	if( isset($_POST['add_from_armory']) ) {
		add_raider_from_web($xml, $_POST['add_from_armory']['name'], $_POST['add_from_armory']['server'] );

		// Add the selected server to the xml, for convenience
		$xml->options->addAttribute('selected_server', $_POST['add_from_armory']['server']);
	}
}


// Send the page header for xml content
header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT'); // always modified
header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
header('Cache-Control: post-check=0, pre-check=0', false); // HTTP/1.1
header('Pragma: no-cache'); // HTTP/1.0
header('Content-type: text/xml');

// Send the output string
echo $xml->asXML_with_XSL($xsl_path);
?>