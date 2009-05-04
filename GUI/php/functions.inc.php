<?php 
// === ATTACHING THE CONFIGURATION FORM ===

/**
 * Append the configuration form contents to the XML element
 * 
 * optionally, an array of options can be passed (Which should just be $_POST, the form submission array)
 * 
 * This function is capable of caching the configuration fields
 * @param SimpleXMLElement $xml
 * @param array $arr_options
 * @return null
 */
function append_simulation_config_form( SimpleXMLElement $xml )
{
	// If we're not allowed to use the cached config options, or if they don't exist
	$str_options_xml = read_cache_value('config_options_xml');
	if( USE_CACHE_CONFIG_OPTIONS === false || is_null($str_options_xml) ) {
	
		// Build the configuration form by parsing the C++ source files
		$str_options_xml = build_simulation_config_options()->asXML();

		// cache these values for future reference
		write_cache_value('config_options_xml', $str_options_xml);
	}

	// Append the config options to the passed xml target
	simplexml_append($xml, new SimpleXMLElement($str_options_xml));
	
	//Set the max-file-size for uploaded files
	$xml->options->addAttribute('max_file_size', return_bytes(ini_get('upload_max_filesize')));
	
	// Add the 'allow simulation' define
	$xml->options->addAttribute('allow_simulation', ALLOW_SIMULATION?1:0);
}

/**
 * Attempt to set some default values
 * @param $xml
 * @return unknown_type
 */
function set_default_values( SimpleXMLElement $xml )
{
	// Reach in and get the global-options tag
	$global_options = $xml->options->global_options ? $xml->options->global_options : $xml->options->addChild('global_options');
	
	// Get the all-classes options tag
	$all_classes = fetch_single_XML_xpath($xml->options->supported_classes, "class[@class='all_classes']");
	
	// Most of these were lifted from the Globals_T7 file
	set_single_xml_xpath_property($global_options, "option[@name='patch']", 'value', '3.1.0');
	set_single_xml_xpath_property($global_options, "option[@name='queue_lag']", 'value', 0.075);
	set_single_xml_xpath_property($global_options, "option[@name='gcd_lag']", 'value', 0.150);
	set_single_xml_xpath_property($global_options, "option[@name='channel_lag']", 'value', 0.250);
	set_single_xml_xpath_property($global_options, "option[@name='target_level']", 'value', 83);
	set_single_xml_xpath_property($global_options, "option[@name='max_time']", 'value', 300);
	set_single_xml_xpath_property($global_options, "option[@name='iterations']", 'value', 1000);
	set_single_xml_xpath_property($global_options, "option[@name='infinite_mana']", 'value', 0);
	set_single_xml_xpath_property($global_options, "option[@name='regen_periodicity']", 'value', 1.0);
	set_single_xml_xpath_property($global_options, "option[@name='target_armor']", 'value', 10643);
	set_single_xml_xpath_property($global_options, "option[@name='target_race']", 'value', none);
	set_single_xml_xpath_property($global_options, "option[@name='faerie_fire']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='mangle']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='battle_shout']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='sunder_armor']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='thunder_clap']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='blessing_of_kings']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='blessing_of_wisdom']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='blessing_of_might']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='judgement_of_wisdom']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='sanctified_retribution']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='swift_retribution']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='crypt_fever']", 'value', 1);
	set_single_xml_xpath_property($global_options, "option[@name='calculate_scale_factors']", 'value', 0);
	set_single_xml_xpath_property($global_options, "option[@name='optimal_raid']", 'value', 1);
}

/**
 * Given an array of values from the form, set the values in the passed form
 * @param SimpleXMLElement $xml
 * @param $array
 * @return null
 */
function set_values_from_array( SimpleXMLElement $xml, array $arr_options )
{
	// Reach in and get the global-options tag (or create it if it isn't present)
	$global_options = $xml->options->global_options ? $xml->options->global_options : $xml->options->addChild('global_options');

	// Append each of the globals values that was present in the array
	if( is_array($arr_options['globals']) ) {
		foreach($arr_options['globals'] as $index => $value ) {
			$target = $global_options->xpath("option[@name='$index']");
			$target[0]['value'] = $value;
		}
	}
	
	// For each of the raiders, add a new raider with those attributes
	if( is_array($arr_options['raider']) ) {
		foreach($arr_options['raider'] as $arr_values ) {
			add_raider_to_XML($xml, $arr_values);
		}
	}
}

/**
 * Set the values of a passed config form from an uploaded config file
 * @return unknown_type
 */
function set_values_from_uploaded_file( SimpleXMLElement $xml )
{
	// We need the 'supported classes' define for this one
	global $ARR_SUPPORTED_CLASSES;
	
	// Fetch the file's contents
	$file_content = file_get_contents($_FILES['import_file_path']['tmp_name']);
	
	// match the file for assignment statements
	preg_match_all('/^([^#]*)=(.*)$/Um', $file_content, $arr_matches, PREG_SET_ORDER);
	
	// This array holds all the options for a new player
	$arr_options = array();
	$arr_options['raiders'] = array();
	$arr_options['globals'] = array();
	
	// For each of the matches
	foreach($arr_matches as $arr_match) {
		
		// Pull out the option and it's value
		$field_name = trim($arr_match[1]);
		$field_value = trim($arr_match[2]);
		
		// If this option is the name of one of the supported classes, then a new player is beginning
		if( in_array($field_name, array_keys($ARR_SUPPORTED_CLASSES)) ) {
			
			// Commit the previous collection of player options to xml, if any existed
			if(!empty($arr_options['raiders'])) {
				add_raider_to_XML($xml, $arr_options['raiders']);
			}
			
			// Prepare a new array of player options 
			$arr_options['raiders'] = array();
			$arr_options['raiders']['class'] = $field_name;
			$arr_options['raiders']['name'] = $field_value;
			
			// Continue to the next iteration
			continue;
		} 
		
		// If a current player is active, sort this option into the current raider array, else it goes in the globals array
		$target_sub_array_index = !empty($arr_options['raiders']) ? 'raiders' : 'globals';

		// Set the value (combining the variable+= definitions)
		//  If the field name ends in +, append the value
		if( substr($field_name, -1) === '+' ) {
			$arr_options[$target_sub_array_index][rtrim($field_name, '+')] .= $field_value;
		}
		
		// Else overwrite anything that currently exists
		else {
			$arr_options[$target_sub_array_index][$field_name] = $field_value;	
		}
	}

	// Commit the last player that wasn't committed above
	if(!empty($arr_options['raiders'])) {
		add_raider_to_XML($xml, $arr_options['raiders']);
	}

	// Commit the global values
	foreach($arr_options['globals'] as $field_name => $field_value ) {
			$xpath_result = $xml->xpath("//options/global_options/option[@name='$field_name']");
			if( is_array($xpath_result) && count($xpath_result) > 0 ) {
				$xpath_result[0]['value'] = $field_value;
			}		
	}
	
}

/**
 * Add A raider to the target XML object
 * 
 * @param $class_name
 * @param $xml
 * @return unknown_type
 */
function add_raider_to_XML( SimpleXMLElement $xml, array $arr_option_values )
{
	// globalize the supported-class define array 
	global $ARR_SUPPORTED_CLASSES;
	
	// Make sure the class name is an allowed class
	if( !in_array($arr_option_values['class'], array_keys($ARR_SUPPORTED_CLASSES)) ) {
		throw new Exception("The supplied class name ({$arr_option_values['class']}) is not an allowed class.");
	}
		
	// If the raid-content tag isn't already present in the target xml, add it
	$xml_raid_content = $xml->raid_content ? $xml->raid_content : $xml->addChild('raid_content');
	
	// Add the player tag
	$new_raider = $xml_raid_content->addChild('player'); 
	
	// Add any options this raider has
	foreach($arr_option_values as $option_name => $option_value) {
		$new_raider->addAttribute($option_name, $option_value);	
	}
	
	// Ensure that the name of this raider is distinct from any of the existing members
	while( count($xml_raid_content->xpath("player[@name='{$new_raider['name']}']")) > 1 ) {
		$new_raider['name'] = $new_raider['name'].'A';
	}
}

/**
 * Add a raider definition to the xml, from the web
 * 
 * This involves fetching the character from the armory, and then fetching any additional information
 * from various sources
 * 
 * FIXME: Currently, this function only pulls the output of fetch_character_from_armory().  This is incomplete
 * since the armory doesn't contain all the necessary data for a simulation member
 * @param $xml
 * @param $character_name
 * @param $server_name
 * @return unknown_type
 */
function add_raider_from_web( SimpleXMLElement $xml, $character_name, $server_name )
{
	// Fetch the given character from the armory
	$arr_character = fetch_character_from_armory($character_name, $server_name);
	if( $arr_character === false ) {
		return false;
	}
	
	// Add the raider to the xml, with the given values
	add_raider_to_XML($xml, $arr_character);
}

/**
 * Add the WoW servers to the XML
 * @param $xml
 * @return unknown_type
 */
function add_wow_servers( SimpleXMLElement $xml )
{
	$arr_servers = get_arr_wow_servers();
	//print_r($arr_servers);
	$xml_servers = $xml->addChild('servers');
	foreach( $arr_servers as $region => $arr_list ) {
		foreach( $arr_list as $arr_server ) {
			$new_server = $xml_servers->addChild('server');
			$new_server->addAttribute('name', $arr_server['name']);
			$new_server->addAttribute('label', $arr_server['label']);
			$new_server->addAttribute('region', $region);
		}
	}
} 

// === EXPORTING CONFIG FILE ===

/**
 * Build a .simcraft config file from an array of options
 *  
 * @param array $arr_options
 * @return string
 */
function build_config_file_from_array( array $arr_options )
{
	// Prime the output string
	$return_string = "#!simcraft\n\n";
	
	// Append each of the globals values that was present in the array
	if( is_array($arr_options['globals']) ) {
		$return_string .= "# GLOBAL SETTINGS\n";
		foreach($arr_options['globals'] as $index => $value ) {
			if( $value!=='' ) {
				$return_string .= "$index=$value\n";
			}
		}
		$return_string .= "\n\n";
	}
	
	// For each of the raiders, add a new raider with those attributes
	if( is_array($arr_options['raider']) ) {
		$return_string .= "# CHARACTER DEFINITIONS\n";
		foreach($arr_options['raider'] as $arr_values ) {
			foreach($arr_values as $index => $value ) {
				
				// If the attribute name is 'class', handle the special class=name attribute
				if( $index=='class' ) {
					$index=$value;
					$value = $arr_values['name'];
				}
				
				// Be sure to skip the 'name' attribute too, since it's handled specially with the class attribute above
				if( $value!=='' && $index!='name' ) {
					$return_string .= "$index=$value\n";
				}
			}
			$return_string .= "\n\n";
		}
		$return_string .= "\n\n";
	}
	
	// Return the output
	return $return_string;
} 

// === BUILDING THE SIMCRAFT COMMAND ===

/**
 * Generate the simcraft system command, from the given array of options
 * @param array $arr_options
 * @return string
 */
function generate_simcraft_command( array $arr_options, $output_file=null )
{
	// Start with the simcraft invocation
	$simcraft_command = './simcraft';

	// Add the options from 'globals' and 'raider' sub-arrays
	$simcraft_command .= recursive_build_option_string($arr_options['globals']);	
	$simcraft_command .= recursive_build_option_string($arr_options['raider']);		
	
	// Add the output directives
	if( !is_null($output_file) ) {
		$simcraft_command .= " html='$output_file'";
	}
	
	// Return the assembled command
	return $simcraft_command;
}

/**
 * Recursively construct the options string
 * 
 * The array passed into this function is ultimately derived from the HTML form's fields
 * @param $array
 * @return string
 */
function recursive_build_option_string( array $array=null )
{
	// Init this array's string
	$str_return = '';
	
	// Loop over the elements of this array
	if( is_array($array) ) {
		foreach($array as $index => $element) {
	
			// If the element is an array, recurse for deeper options 
			if( is_array($element) ) {
				$str_return .= recursive_build_option_string($element);
			}
			
			// Else, just append this element
			else {
				
				// If the element name is class, this is a special attribute that we inserted manually, handle it separately
				if( $index === 'class' ) {
					$str_return .= ' ' . $element . "='" . $array['name']."'";
				}
				
				// Since class and name are handled together, if this element is name, skip it
				else if( $index === 'name' ) {
				}
				
				// Otherwise, if the element isn't empty, append it to the string
				else if( $element !== '' ) {
					$str_return .= ' ' . $index . "='" . $element . "'";
				}
			}
		}
	} 
	
	// Return the created string
	return $str_return;
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
 * Simple helper function to speed up the laborious xpath process with SimpleXMLElement
 * @param $xml
 * @param $str_xpath
 * @return unknown_type
 */
function fetch_single_XML_xpath( SimpleXMLElement $xml, $str_xpath )
{
	// Attempt to match the element
	$match = $xml->xpath($str_xpath);

	// One and only one match must be made
	if( !is_array($match) || count($match) == 0) {
		throw new Exception("No Elements were found that match the xpath search");
	}
	else if( count($match) > 1) {
		throw new Exception("More than one result was matched on this XML element");
	}
	
	// Return the valid match
	return $match[0];
}
 
/**
 * Another simple helper function.  This one sets a single property on the fetched element
 * @return unknown_type
 */
function set_single_xml_xpath_property( SimpleXMLElement $xml, $str_xpath, $attribute_name, $attribute_value )
{
	$target = fetch_single_XML_xpath($xml, $str_xpath);
	$target[$attribute_name] = $attribute_value;
}

/**
 * Thanks to l dot j dot peters at student dot utwente dot nl 
 * from http://us.php.net/manual/en/function.simplexml-element-addChild.php
 * 
 * Append one element to another.
 * @param $parent
 * @param $new_child
 * @return unknown_type
 */
function simplexml_append(SimpleXMLElement $parent, SimpleXMLElement $new_child){
	$node1 = dom_import_simplexml($parent);
	$dom_sxe = dom_import_simplexml($new_child);
	$node2 = $node1->ownerDocument->importNode($dom_sxe, true);
	$node1->appendChild($node2);
}

/**
 * Convert php ini directive shorthand byte-counts into integers
 * @param string $val
 * @return integer
 */
function return_bytes($val) 
{
	$val = trim($val);
	$last = strtolower($val[strlen($val)-1]);
	switch($last) {
		// The 'G' modifier is available since PHP 5.1.0
		case 'g':
			$val *= 1024;
		case 'm':
			$val *= 1024;
		case 'k':
			$val *= 1024;
	}
	
	return $val;
}  

// === DATA CACHING ===

/**
 * Given any arbitrary variable, stash it in the data cache
 * @param $name
 * @param $value
 * @return unknown_type
 */
function write_cache_value($name, $value)
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
function read_cache_value($name)
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


// === GENERATING CONFIG OPTIONS FROM C++ SOURCE ===

/**
 * Generate xml representing the configuration options
 * @return SimpleXMLElement
 */
function build_simulation_config_options()
{ 
	// Allow access to the global definition arrays (from defines.inc.php)
	global $ARR_SUPPORTED_CLASSES, $ARR_PLAYER_RELATED_SOURCE_FILES;

	// The options XML root
	$xml = new SimpleXMLElement('<options />');
	
	
	// Build the array of C++ source files that constitute simulationcraft
	$arr_source_files = scandir(SIMULATIONCRAFT_PATH);
	foreach($arr_source_files as $index=>$directory_element) {
		if( pathinfo($directory_element, PATHINFO_EXTENSION)!=='cpp' ) {
			unset($arr_source_files[$index]);
		}
	}
	
	
	// Add the options from the individual character-class files
	$xml_player_options = $xml->addChild('supported_classes');
	foreach($ARR_SUPPORTED_CLASSES as $class_name => $class_array ) {
		
		// Remove this class's file from the array of source file names - it's already been handled
		unset($arr_source_files[array_search($class_array['source_file'], $arr_source_files)]);
		
		// Add the class element, with a class and label attribute
		$new_class = $xml_player_options->addChild('class');
		$new_class->addAttribute('class', $class_name);
		$new_class->addAttribute('label', ucwords(strtolower(str_replace('_', ' ', $class_name))));

		// Fetch the options for this class type
		if( !empty($class_array['source_file']) ) {
			$arr_options = parse_source_file_for_options( get_valid_path(SIMULATIONCRAFT_PATH) . $class_array['source_file'] );
			add_options_to_XML( $arr_options, $new_class);
		}		
	}

	
	// Add the options for all-classes
	$xml_all_class_options = $xml_player_options->addChild('class');
	$xml_all_class_options->addAttribute('class', 'all_classes');
	$xml_all_class_options->addAttribute('label', ucwords(strtolower(str_replace('_', ' ', 'all_classes'))));
	foreach($ARR_PLAYER_RELATED_SOURCE_FILES as $file_name ) {
		
		// Remove this class's file from the array of source file names - it's already been handled
		unset($arr_source_files[array_search($file_name, $arr_source_files)]);

		// Fetch the options for this class type
		if( !empty($file_name) ) {
			$arr_options = parse_source_file_for_options( get_valid_path(SIMULATIONCRAFT_PATH) . $file_name );
			add_options_to_XML( $arr_options, $xml_all_class_options);
		}		
	}
		
	
	// Add the global simulation options (the options in the files that weren't handled above)
	$xml_global_options = $xml->addChild('global_options');
	foreach( $arr_source_files as $file_name ) {		
		
		// Fetch the options for this class type
		if( !empty($file_name) ) {
			$arr_options = parse_source_file_for_options( get_valid_path(SIMULATIONCRAFT_PATH) . $file_name );
			add_options_to_XML( $arr_options, $xml_global_options);
		}		
	}
		
	// Add the additional hand-tweaked options
	add_hand_edited_options($xml);
	
	
	// return the generated XML
	return $xml;
}

/**
 * Add some manually-derived options to the outgoing XML
 * 
 * No matter how good the auto-parsing of the C++ source files is, there's still some 
 * hand-editing to do.  Hopefully these changes won't go stale very quickly.
 * 
 * A lot of these options were found in sc_option.cpp, starting around line 306, revision 2186
 * @return unknown_type
 */
function add_hand_edited_options( SimpleXMLElement $xml )
{
	// Reach in and get the global-options tag
	$global_options = $xml->global_options ? $xml->global_options : $xml->addChild('global_options');
	
	// Get the all-classes options tag
	$all_classes = fetch_single_XML_xpath($xml->supported_classes, "class[@class='all_classes']");

	// Add the talent meta-parameter
	add_options_to_XML(array(array(
				'name' => 'talents',
				'type' => 'OPT_STRING',
				'tag' => '',
				'file' => 'meta_options'
		)), 
		$all_classes);

	// Add the optimal-raid meta-parameter
	add_options_to_XML(array(array(
				'name' => 'optimal_raid',
				'type' => 'OPT_INT',
				'tag' => '',
				'file' => 'meta_options'
		)), 
		$global_options);

	// Add the patch meta-parameter
	add_options_to_XML(array(array(
				'name' => 'patch',
				'type' => 'OPT_STRING',
				'tag' => '',
				'file' => 'meta_options'
		)), 
		$global_options);
		
}

/**
 * Parse one of the simulationcraft C++ source files for the command-line options it handles
 * 
 * Optionally, you can cache the file's parse for later
 * 
 * this method relies heavily on regular expressions, and assumes the existence of a parse_options()
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
	$file_contents = str_replace("\n", '', $file_contents);
	
	// Match for the options (fun with regular expressions)
	preg_match('/bool [-_A-Za-z0-9]+::parse_option\(.*options\[\][\s]*=[\s]*{(.*);/U', $file_contents, $parse_matches);
	$options = $parse_matches[1];
	preg_match_all('/{[\s]*"([^,]*)",[\s]*([^,]*),[\s]*&\([\s]*([^,.]*)[^,]*\)[\s]*}[\s]*,/', $options, $parse_matches, PREG_SET_ORDER);
	//var_dump($parse_matches);
	
	// Arrange the options in a sane array
	$arr_output = array();
	foreach($parse_matches as $array) {
		$arr_output[] = array(
				'name' => trim($array[1]), 
				'type' => trim($array[2]), 
				'tag' => trim($array[3]), 
				'file' => $file_path
			);
	}
			
	// Return the array of matches	
	return $arr_output;
}

/**
 * Given an array of options, add the options to the given xml element
 * 
 * The array of options should be formated as subarrays with a 'name' and 'type' attribute.
 * @param array $arr_options
 * @param SimpleXMLElement $xml_object
 * @return null
 */
function add_options_to_XML( array $arr_options, SimpleXMLElement $xml_object)
{
	// Loop over the supplied array of options
	foreach($arr_options as $arr_option ) {

		// Ignore talent tags, these will be passed as a 'talents' URL
		if( $arr_option['tag'] === 'talents' ) {
			continue;
		}
		
		
		// === Convert the C++ types to html field types ===
		// certain types of OPT_INT options can be assumed to be boolean checkboxes
		if( $arr_option['type']==='OPT_INT' && in_array($arr_option['tag'], array('glyphs', 'idols', 'totems', 'tiers')) ) {
			$html_type = 'boolean';
		}
		
		// All other OPT_INTs are just text fields
		else if( $arr_option['type']==='OPT_INT' ) {
			$html_type = 'text';
		}
		
		// strings are text fields
		else if( $arr_option['type']==='OPT_STRING' ) {
			$html_type = 'text';
		}
		
		// floats are text fields
		else if( $arr_option['type']==='OPT_FLT' ) {
			$html_type = 'text';
		}
		
		// Do not display options that are not of the above types
		else {
			continue;
		}

		
		// Add the option to the xml object given		
		$xml_new = $xml_object->addChild('option');
		$xml_new->addAttribute('name', $arr_option['name']);
		$xml_new->addAttribute('label', ucwords(strtolower(str_replace('_', ' ', $arr_option['name']))) );
		$xml_new->addAttribute('type', $html_type);
		$xml_new->addAttribute('cpp_type', $arr_option['type']);
		$xml_new->addAttribute('tag', $arr_option['tag']);
		$xml_new->addAttribute('file', basename($arr_option['file']) );
		$xml_new->addAttribute('value', '');
	}
}
?>