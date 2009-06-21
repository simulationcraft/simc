<?php 
// === EXPORTING CONFIG FILE, BUILDING SIMCRAFT COMMAND ===

/**
 * Generate a flat array of key/value pairs, suitable for a config file or generating the simcraft command
 * 
 * Note that options in the passed form array will not be present in the output, if the are not
 * 'official' options present in the C++ code. 
 * @param $arr_options
 * @return array
 */
function generate_config_array( array $arr_options )
{
	// Fetch the possible config values
	$xml_options = new SimpleXMLElement_XSL(read_cache_value('config_options_xml'));
	
	// Prime the output array
	$return_array = array();
	
	// If the array of values contains a 'globals' sub-array
	if( is_array($arr_options['globals']) ) {
		$return_array[] = "GLOBAL SETTINGS";
		
		// Loop over the globals options, as they were defined for the form's creation
		foreach( $xml_options->global_options[0]->option as $xml_option) {
			
			// Pull out some useful attributes of the xml elements
			$option_name = (string)$xml_option['name'];
			$option_cpp_type = (string)$xml_option['cpp_type'];
			$submitted_value = $arr_options['globals'][$option_name];
			
			// If the option is a boolean type, and no value was submitted (a checkbox with no submitted value)
			if( $option_cpp_type==='OPT_BOOL' && is_null($submitted_value) ) {
				$submitted_value = 0;
			}
			
			// If the submitted value is not empty, add it to the config array
			if( $submitted_value!=='' ) {
				$return_array[] = array($option_name => $submitted_value);
			}
		}
	}	
	

	
	// If the array of values contains a 'raider' sub-array
	if( is_array($arr_options['raider']) ) {
		$return_array[] = "CHARACTER DEFINITIONS";
		
		// Loop over each of the submitted raider characters
		foreach($arr_options['raider'] as $arr_values ) {
			
			$return_array[] = "CHARACTER  {$arr_values['name']} ({$arr_values['class']})";			
			
			// Pull out the options that go with this raider's class, including the options all classes share
			$class_options = $xml_options->supported_classes->xpath("class[@class='all_classes' or @class='{$arr_values['class']}']/option");
			
			// Add the atypical class=character_name option (doesn't actually appear in the config list, for obvious reasons)
			$return_array[] = array($arr_values['class'] => $arr_values['name']);
			
			// Loop over the raider's options, as they were defined for the form's creation
			foreach( $class_options as $xml_option) {
			
				// Pull out some useful attributes of the xml elements
				$option_name = (string)$xml_option['name'];
				$option_cpp_type = (string)$xml_option['cpp_type'];
				$submitted_value = $arr_values[$option_name];
				
				// If the attribute name is 'class' or 'name', skip it; it was already handled
				if( $option_name=='class' || $option_name=='name' ) {
					continue;
				}
				
				// If the option is a boolean type, and no value was submitted (a checkbox with no submitted value)
				if( $option_cpp_type==='OPT_BOOL' && is_null($submitted_value) ) {
					$submitted_value = 0;
				}
				
				// If the submitted value is not empty, add it to the config array
				if( $submitted_value!=='' ) {
					$return_array[] = array($option_name => $submitted_value);
				}
			}
		}
	}

	// Return the output
	return $return_array;	
}

/**
 * Build a .simcraft config file from an array of options
 *  
 * @param array $arr_options
 * @return string
 */
function build_file_from_config_array( array $arr_options )
{
	// Prime the output string
	$return_string = "#!simcraft\n\n";
	
	// Fetch the configuration array, given the array of options
	$arr_settings = generate_config_array($arr_options);
	
	// Loop over the options, building the config file
	foreach( $arr_settings as $value ) {
		
		// If the attribute is a comment, add the comment line
		if( !is_array($value) ) {
			$return_string .= "\n# " . ltrim($value, '# ') . "\n";
		}
		
		// Else, add the configuration value
		else {
			$opt_name = array_keys($value);
			$opt_value = array_values($value);
			$return_string .= "{$opt_name[0]}={$opt_value[0]}\n";
		}
	}
	
	// Return the output
	return $return_string;
}

/**
 * Generate the simcraft system command, from the given array of options
 * @param array $arr_options
 * @return string
 */
function generate_simcraft_command( array $arr_options, $output_file=null )
{
	// Prime the output string
	$return_string = './simcraft';

	// Fetch the configuration array, given the array of options
	$arr_settings = generate_config_array($arr_options);
	
	// Loop over the options, building the config file
	foreach( $arr_settings as $value ) {
		
		// If the attribute is a comment, add the comment line
		if( !is_array($value) ) {
			continue;
		}
		
		// Else, add the configuration value
		else {
			$opt_name = array_keys($value);
			$opt_value = array_values($value);
			$return_string .= " {$opt_name[0]}='{$opt_value[0]}'";
		}
	}
	
	// Add the output directives
	if( !is_null($output_file) ) {
		$return_string .= " xml='$output_file'";
	}
		
	// Return the output
	return $return_string;
} 

// === GENERATING CONFIG OPTIONS FROM C++ SOURCE ===

/**
 * Parse one of the simulationcraft C++ source files for the command-line options it handles
 * 
 * this method relies heavily on regular expressions, and assumes the existence of a get_options()
 * method in the file, which must contain an array of options.  If this source format changes,
 * this function will need to be rebuilt.
 * @param string $file_path
 * @return array
 */
function parse_source_file_for_options( $file_path ) 
{
	// Insist the file exists
	if( !is_file($file_path)) {
		throw new Exception("The given file path ($file_path) is not a file");
	}

	// Load the file in question's contents, stripping out any new-lines
	$file_contents = file_get_contents( $file_path );
	
	// Match the file contents for the get_options() method's 'option_t options[] = ' array
	preg_match('/^[^\n\r]+::get_options\([^)]*\)\s*{.*option_t\s+options\[\]\s*=\s*[^;]*{(.*)}\s*;\s*$/Usm', $file_contents, $parse_matches);
	$options = $parse_matches[1];
	
	// Match the options array contents for for individual option or option_doc strings
	preg_match_all('/^\s*{\s*"([^"]*)",\s*([^\s,]*),.*},\s*$|^\s*\/\/\s*@option_doc(\s*(loc|title)=("[^"]*"|[^"=\s]*)\s*)*$/Usm', $options, $parse_matches, PREG_SET_ORDER);
	
	// Arrange the options in a sane array
	$arr_output = array();
	$last_loc = null;
	$last_title = null;
	foreach($parse_matches as $array) {
		
		// If this is a comment line, assume its an optiondoc, set the new last-loc and last-title
		if( substr(trim($array[0]), 0, 2)=='//' ) {
			
			// Keep everything after the '@option_doc' 
			$str_optdoc = substr($array[0], strpos($array[0], '@option_doc')+strlen('@option_doc')+1 );

			// Match on the loc and title option doc directives			
			preg_match_all('/(loc|title)=("[^"]*"|[^"=\s]*)\s*/', $str_optdoc, $arr_optiondoc_matches, PREG_SET_ORDER);
			
			// For the identified option_doc assertions
			foreach( $arr_optiondoc_matches as $arr_match) {
				if($arr_match[1] === 'loc') {
					$last_loc = trim($arr_match[2], '" ');
				}
				else if($arr_match[1] === 'title') {
					$last_title = trim($arr_match[2], '" ');
				}
			}
		}

		// Else, this is an option line, add it to the array of options
		else {
			$arr_output[] = array(
					'name' => trim($array[1]), 
					'type' => trim($array[2]), 
					'optdoc_loc' => $last_loc,
					'optdoc_title' => $last_title, 
					'file' => $file_path
				);
		}
	}

	// Return the array of matches
	return $arr_output;
}


// === DATA CACHING ===

/**
 * Given any arbitrary variable, stash it in the data cache
 * @param $name
 * @param $value
 * @return unknown_type
 */
function write_cache_value( $name, $value )
{
	// If the cache file already exists, fetch it's content array
	if( is_file(CACHE_FILE_PATH) ) { 
		$arr_cache = unserialize(file_get_contents(CACHE_FILE_PATH));
	}
	
	// else, create the blank array
	else {
		$arr_cache = array();
	}	

	// Write the new element to the array
	$arr_cache[$name] = $value;
	
	// Write the array back to the cache file
	file_put_contents(CACHE_FILE_PATH, serialize($arr_cache));
}

/**
 * Given a variable name, read it from the data cache
 * @param $name
 * @return unknown_type
 */
function read_cache_value( $name )
{
	// If the cache file already exists, fetch it's content array
	if( is_file(CACHE_FILE_PATH) ) { 
		$arr_cache = unserialize(file_get_contents(CACHE_FILE_PATH));
		return $arr_cache[$name];
	}
	
	// Else, there's no cache value to report, return null
	else {
		return null;
	}
} 

// === SUPPORT FUNCTIONS ===

/**
 * Handle implicit class-file loading
 * @param string $class_name
 * @return null
 */ 
function __autoload( $class_name ) 
{
	require_once "$class_name.inc.php";
}

/**
 * Return a full path corresponding with the input path, and throw an exception if the path doesn't exist
 * @param string $path
 * @return string
 */
function get_valid_path( $path )
{
	// If the path supplied isn't legit, throw an exception
	if( ! realpath($path) ) {
		throw new Exception("The supplied path ($path) does not have a valid real path.");
	}
	
	// Return the real path, with a single directory separator on the right
	return rtrim(realpath($path), '/') . '/';
}

/**
 * Convert php ini directive shorthand byte-counts into integers
 * @param mixed $val
 * @return integer
 */
function return_bytes( $val ) 
{
	$val = trim($val);

	// Interpret the value, based on the final character (g, m, or k)
	switch( strtolower($val[strlen($val)-1]) ) {
		// The 'G' modifier is available since PHP 5.1.0
		case 'g':
			$val *= 1024;
		case 'm':
			$val *= 1024;
		case 'k':
			$val *= 1024;
	}
	
	// Return the value
	return (int)$val;
}  

// === ERROR HANDLING ===

/**
 * A custom exception handler for uncaught exceptions
 * @param Exception $exception the thrown exception
 * @return null
 */
function custom_exception_handler( $exception ) 
{
	// Log the exception details in the log file
	error_log($exception);

	// Attempt to recover the form contents, as much as possible
	try {

		// Create the output XML object
		$xml = XML_SimcraftConfigForm::factory();

		// Add the exception description to the XML
		$exceptions = $xml->exceptions ? $xml->exceptions : $xml->addChild('exceptions');
		$xml_exc = $exceptions->addChild('exception');
		$xml_exc->addAttribute('code', $exception->getCode() );
		$xml_exc->addAttribute('message', $exception->getMessage() );
		
		// Make sure the user's settings stay preserved
		if( !empty($_POST) ) {
			$xml->set_option_values_from_array($_POST);
		}
		
		// else just use the default values
		else {
			$xml->set_default_option_values();
		}
	}
	
	// If the attempt to recover the form contents failed, add an additional error message for the user - hey, we tried
	catch( Exception $e) {
		$exceptions = $xml->exceptions ? $xml->exceptions : $xml->addChild('exceptions');
		$xml_exc = $exceptions->addChild('exception');
		$xml_exc->addAttribute('message', 'An attempt was made to recover the form contents, but failed.' );
	}
		
	// Send the page content, with an error header
	header('HTTP/1.1 500 Internal Server Error');
	$xml->release_to_browser('xsl/config_form.xsl');
	exit(1);
}
?>