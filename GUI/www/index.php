<?php 
// Set up the include paths
set_include_path( get_include_path().PATH_SEPARATOR.'../php' );

// Require the support files
require_once 'defines.inc.php';
require_once 'functions.inc.php';


// Create the output XML object
$xml = new SimpleXMLElement_XSL('<xml></xml>');


// Determine the file that was requested in the URL
$request_file_name = basename($_SERVER['PATH_INFO']);


// If the request is to run a simulation, run it now
if( $request_file_name == 'simulation.xml' ) {
	
	// Develop the simcraft command from the form input
	$output_file = tempnam('/tmp', 'simcraft_output');
	$simcraft_command = generate_simcraft_command($_POST, $output_file );

	// Change to the simulationcraft directory
	chdir( get_valid_path(SIMULATIONCRAFT_PATH) );
	
	// Call the simcraft execution
	$simcraft_output = shell_exec( $simcraft_command );
	
	// FIXME temporarily, just print the html, since the generated html isn't XHTML compliant
	print file_get_contents($output_file);
	die();
	
//	// Add the simcraft command to the outgoing xml, just for posterity
//	$xml->addChild('simcraft_command', htmlentities($simcraft_command) );
//	
//	// Add the response to the outgoing xml
//	$xml->addChild('command_return', $simcraft_output);
//	
//	// Parse the output file
//	$file_contents = file_get_contents($output_file);
//	
//	// Add the generated report
//	$xml->addChild('generated_report', $file_contents );
//	
//	// Attach the appropriate XSL file
//	$xsl_path = 'xsl/result_display.xsl';
}

// Else, just show the simulation configuration form
else {

	// Add the sumulation config form XML to the outgoing XML object
	append_simulation_config_form($xml);
	
	// Attach the config form XSL
	$xsl_path = 'xsl/config_form.xsl';
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