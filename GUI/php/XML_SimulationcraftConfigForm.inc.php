<?php
/**
 * Builds the configuration form for the simulationcraft simulation 
 */
class XML_SimulationcraftConfigForm extends SimpleXMLElement_XSL
{
	/**
	 * This gets around the weird 'feature' of SimpleXMLElement that 
	 * prevents you from extending the constructor.
	 * @return SXML_SimulationcraftConfigForm
	 */
	static public function factory()
	{
		// Create the output XML object
		$xml = new XML_SimulationcraftConfigForm('<xml></xml>');
			
		// Add the simulation config form XML to the outgoing XML object
		$xml->append_simulation_config_form_elements();
	
		// Add the wow servers
		$xml->add_wow_servers();

		// Return the newly created object
		return $xml;
	}
	
	/**
	 * Append the configuration form contents to the XML element
	 * 
	 * This function is capable of caching the configuration fields
	 * @return null
	 */
	public function append_simulation_config_form_elements( )
	{
		// If we're not allowed to use the cached config options, or if they don't exist
		$str_options_xml = read_cache_value('config_options_xml');
		if( USE_CACHE_CONFIG_OPTIONS === false || is_null($str_options_xml) ) {
		
			// Build the configuration form by parsing the C++ source files
			$str_options_xml = $this->build_simulation_config_options()->asXML();
	
			// cache these values for future reference
			write_cache_value('config_options_xml', $str_options_xml);
		}
	
		// Append the config options to the passed xml target
		SimpleXMLElement_XSL::simplexml_append($this, new XML_SimulationcraftConfigForm($str_options_xml));
		
		//Set the max-file-size for uploaded files
		$this->options->addAttribute('max_file_size', return_bytes(ini_get('upload_max_filesize')));
		
		// Add the 'allow simulation' define
		$this->options->addAttribute('allow_simulation', ALLOW_SIMULATION?1:0);
	}
	
	/**
	 * Generate xml representing the configuration options
	 * @return SimpleXMLElement
	 */
	protected function build_simulation_config_options()
	{ 
		// The options XML root
		$xml = new XML_SimulationcraftConfigForm('<options />');
		
		
		// Build the array of C++ source files that constitute simulationcraft
		$arr_source_files = scandir(SIMULATIONCRAFT_PATH);
		foreach($arr_source_files as $index=>$directory_element) {
			if( pathinfo($directory_element, PATHINFO_EXTENSION)!=='cpp' ) {
				unset($arr_source_files[$index]);
			}
		}
				
		
		// Loop over the source files, and add the options	
		foreach( $arr_source_files as $file_name ) {		
				
			// Retrieve the array of options from the source file 
			$arr_options = parse_source_file_for_options( get_valid_path(SIMULATIONCRAFT_PATH) . $file_name );
			
			// Add the options to the XML
			$xml->add_options_from_array( $arr_options );
		}		
			
		// Add the additional hand-tweaked options
		$xml->hand_edit_options();
		
		
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
	protected function hand_edit_options( )
	{
		// Add the talent meta-parameter
		$this->add_options_from_array(array(array(
					'name' => 'talents',
					'type' => 'OPT_STRING',
					'optdoc_loc' => 'player/all/general',
					'optdoc_title' => 'General',
					'file' => 'meta_options'					
			)));
	
		// Add the optimal-raid meta-parameter
		$this->add_options_from_array(array(array(
					'name' => 'optimal_raid',
					'type' => 'OPT_BOOL',
					'optdoc_loc' => 'global/general',
					'optdoc_title' => 'General',
					'file' => 'meta_options'					
			)));
	
		// Add the patch meta-parameter
		$this->add_options_from_array(array(array(
					'name' => 'patch',
					'type' => 'OPT_STRING',
					'optdoc_loc' => 'global/general',
					'optdoc_title' => 'General',
					'file' => 'meta_options'
			)));
			
			
		// Remove some fields that shouldn't be sent to the UI
		$items = $this->xpath('player/all/general/option[@name="name" or @name="quiet" or @name="sleeping" or @name="id" or @name="save"]');
		foreach($items as $item) {
			$dom = dom_import_simplexml($item);
	    $dom->parentNode->removeChild($dom);
		}
    
    $items = $this->xpath('global/general/option[@name="threads" or @name="max_time" or @name="iterations"]');
		foreach($items as $item) {
			$dom = dom_import_simplexml($item);
	    $dom->parentNode->removeChild($dom);
		}
		

		
	}
	
	/**
	 * Given an array of options, add the options to the given xml element
	 * 
	 * The array of options should be formated as subarrays with a 'name' and 'type' attribute.
	 * @param array $arr_options
	 * @return null
	 */
	protected function add_options_from_array( array $arr_options )
	{
		// Loop over the supplied array of options
		foreach($arr_options as $arr_option ) {
	
			// Ignore elements with a 'skip' optdoc loc
			if( $arr_option['optdoc_loc'] === 'skip' ) {
				continue;
			}
			
			
			// === Convert the C++ types to html field types ===
			// OPT_INTs are just text fields
			if( $arr_option['type']==='OPT_INT' ) {
				$interpreted_type = 'integer';
			}
			
			// OPT_BOOL's are boolean
			else if( $arr_option['type']==='OPT_BOOL' ) {
				$interpreted_type = 'boolean';
			}
			
			// strings are text fields
			else if( $arr_option['type']==='OPT_STRING' ) {
				$interpreted_type = 'string';
			}
			
			// floats are text fields
			else if( $arr_option['type']==='OPT_FLT' ) {
				$interpreted_type = 'float';
			}
			
			// If the type is not recognized, something is wrong
			else {
				error_log("The option {$arr_option['name']} from file ".basename($arr_option['file'])." has an unknown type ({$arr_option['type']})", E_USER_NOTICE);
				continue;
			}
	
			
			// Parse the opt-doc loc, if it exists
			$arr_optdoc_loc = false;
			if( !empty($arr_option['optdoc_loc']) ) {
				$arr_optdoc_loc = explode( '/', $arr_option['optdoc_loc'] );
			}
			
			// Build the insertion tree, from the option doc
			$target_xml = $this;
			while( $arr_optdoc_loc!==false && $optdoc_element = array_shift($arr_optdoc_loc)) {
				if( $target_xml->$optdoc_element ) {
					$target_xml = $target_xml->$optdoc_element;
				}
				else {
					$target_xml = $target_xml->addChild($optdoc_element);
					$target_xml->addAttribute('title', ucwords(strtolower(str_replace('_', ' ', $optdoc_element))) );
				}
			}
			
			// Set the parent optdoc title, if necessary
			if( !empty($arr_option['optdoc_title'])  ) {
				$target_xml['title'] = $arr_option['optdoc_title'];
			}
			
			// Add the option to the xml object given		
			$xml_new = $target_xml->addChild('option');
			$xml_new->addAttribute('name', $arr_option['name']);
			$xml_new->addAttribute('label', ucwords(strtolower(str_replace('_', ' ', $arr_option['name']))) );
			$xml_new->addAttribute('value', '');
			$xml_new->addAttribute('type', $interpreted_type);
			//$xml_new->addAttribute('optdoc_loc', $arr_option['optdoc_loc']);
			$xml_new->addAttribute('cpp_type', $arr_option['type']);
			$xml_new->addAttribute('cpp_file', basename($arr_option['file']) );
		}
	}
	
	/**
	 * Attempt to set some default values
	 * @return unknown_type
	 */
	public function set_default_option_values( )
	{
		if( $this->options->global ) {
			$global_options = $this->options->global;
			
			// Most of these were lifted from the Globals_T8 file
			$global_options->set_single_xml_xpath_property(".//option[@name='optimal_raid']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='smooth_rng']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='normalize_scale_factors']", 'value', 1);
			//$global_options->set_single_xml_xpath_property(".//option[@name='threads']", 'value', 2);
			$global_options->set_single_xml_xpath_property(".//option[@name='queue_lag']", 'value', 0.075);
			$global_options->set_single_xml_xpath_property(".//option[@name='gcd_lag']", 'value', 0.150);
			$global_options->set_single_xml_xpath_property(".//option[@name='channel_lag']", 'value', 0.250);
			$global_options->set_single_xml_xpath_property(".//option[@name='travel_variance']", 'value', 0.075);
			$global_options->set_single_xml_xpath_property(".//option[@name='target_level']", 'value', 83);
			//$global_options->set_single_xml_xpath_property(".//option[@name='max_time']", 'value', 300);
			//$global_options->set_single_xml_xpath_property(".//option[@name='iterations']", 'value', 1000);
			$global_options->set_single_xml_xpath_property(".//option[@name='infinite_mana']", 'value', 0);
			$global_options->set_single_xml_xpath_property(".//option[@name='regen_periodicity']", 'value', 1.0);
			$global_options->set_single_xml_xpath_property(".//option[@name='target_armor']", 'value', 10643);
			$global_options->set_single_xml_xpath_property(".//option[@name='target_race']", 'value', 'humanoid');
			$global_options->set_single_xml_xpath_property(".//option[@name='heroic_presence']", 'value', 0);
			$global_options->set_single_xml_xpath_property(".//option[@name='faerie_fire']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='mangle']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='battle_shout']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='sunder_armor']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='thunder_clap']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='blessing_of_kings']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='blessing_of_wisdom']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='blessing_of_might']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='judgement_of_wisdom']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='sanctified_retribution']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='swift_retribution']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='crypt_fever']", 'value', 1);
			$global_options->set_single_xml_xpath_property(".//option[@name='calculate_scale_factors']", 'value', 1);
		}
	}
			
	/**
	 * Given an array of values from the form, set the values in the passed form
	 * @param $arr_options
	 * @return null
	 */
	public function set_option_values_from_array( array $arr_options )
	{
		// Append each of the globals values that was present in the array
		if( $this->options->global && is_array($arr_options['globals']) ) {
			foreach($arr_options['globals'] as $index => $value ) {
				$target = $this->options->global->xpath(".//option[@name='$index']");
				$target[0]['value'] = $value;
			}
		}
		
		// For each of the raiders, add a new raider with those attributes
		if( is_array($arr_options['raider']) ) {
			foreach($arr_options['raider'] as $arr_values ) {
				$this->add_raider_from_array($arr_values);
			}
		}
	}
	
	/**
	 * Set the option values in the passed config form XML from the
	 * .simc-style file at the given path
	 * @param string $file_path the file path
	 * @return null
	 */
	public function set_option_values_from_file( $file_path, $just_raiders=false)
	{
		// Fetch the file's contents
		$file_content = file_get_contents($file_path);
		if( $file_content === false ) {
			throw new Exception("File read failed for file ($file_path)");
		}
		
		// Set the options from the file's contents, and return whatever set_option_values_from_file_contents() returns
		return $this->set_option_values_from_file_contents($file_content, $just_raiders);
	}
	
	/**
	 * Set the options values in this xml object from the contents of a file
	 * @param $file_content
	 * @param $just_raiders
	 * @return unknown_type
	 */
	protected function set_option_values_from_file_contents( $file_content, $just_raiders=false)
	{
		// We need the 'supported classes' define for this one
		global $ARR_SUPPORTED_CLASSES;
		
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
			if( in_array($field_name, $ARR_SUPPORTED_CLASSES) ) {
				
				// Commit the previous collection of player options to xml, if any existed
				if(!empty($arr_options['raiders'])) {
					$this->add_raider_from_array($arr_options['raiders']);
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
			$this->add_raider_from_array($arr_options['raiders']);
		}
	
		
		// Commit the global values
		if( $just_raiders !== true ) {
			foreach($arr_options['globals'] as $field_name => $field_value ) {
					$xpath_result = $this->xpath("//options/global//option[@name='$field_name']");
					if( is_array($xpath_result) && count($xpath_result) > 0 ) {
						$xpath_result[0]['value'] = $field_value;
					}		
			}
		}
		
	}
	
	/**
	 * Add a raider definition to the xml, from the web
	 * 
	 * This involves fetching the character from the armory, and then fetching any additional information
	 * from various sources
	 * 
	 * @param $character_name
	 * @param $server_name
	 * @return unknown_type
	 */
	public function add_raider_from_web( $character_name, $server_name )
	{
		// If one of the values is missing, don't even bother
		if( empty($character_name) || empty($server_name) ) {
			throw new Exception("Either the character name ($character_name) or the server name ($server_name) are empty, and therefore invalid.");
		}
		
		// Attempt to pull out the region from the server name (if its there)
		$region_name = 'us';
		if( strpos($server_name, ':')) {
			list($region_name, $server_name) = explode(':', $server_name);
			$region_name = strtolower($region_name);
		}
		
		// Develop the simulationcraft command from the form input
		$simulationcraft_command = "./simc iterations=0 max_time=1 armory=" . escapeshellarg("$region_name,$server_name,$character_name") . " save={OUTPUT_FILENAME}";
		
		// Execute the command
		list($file_contents, $simulationcraft_output) = execute_simulationcraft_command($simulationcraft_command);
		if( empty($file_contents) ) {
			throw new Exception("Simulationcraft did not succeed in importing the Player.\n\nsimulationcraft command:\n$simulationcraft_command\n\nsimulationcraft STDOUT:\n$simulationcraft_output\n\nsimulationcraft file content:\n$file_contents");
		}
				
		// Add the raider definition in the config file to the XML
		$this->set_option_values_from_file_contents($file_contents, true);
	}
	
	/**
	 * Add the WoW servers to the XML
	 * @return unknown_type
	 */
	public function add_wow_servers( )
	{
		$arr_servers = get_arr_wow_servers();
	
		$xml_servers = $this->addChild('servers');
		foreach( $arr_servers as $region => $arr_list ) {
			foreach( $arr_list as $arr_server ) {
				$new_server = $xml_servers->addChild('server');
				$new_server->addAttribute('name', $arr_server['name']);
				$new_server->addAttribute('region', $region);
			}
		}
	}
	
	/**
	 * Add A raider to the target XML object
	 * 
	 * @param $arr_option_values
	 * @return unknown_type
	 */
	protected function add_raider_from_array( array $arr_option_values )
	{
		// globalize the supported-class define array 
		global $ARR_SUPPORTED_CLASSES;
		
		// Make sure the class name is an allowed class
		if( !in_array($arr_option_values['class'], $ARR_SUPPORTED_CLASSES) ) {
			throw new Exception("The supplied class name ({$arr_option_values['class']}) is not an allowed class.");
		}
			
		// If the raid-content tag isn't already present in the target xml, add it
		$xml_raid_content = $this->raid_content ? $this->raid_content : $this->addChild('raid_content');
		
		// Add the player tag
		$new_raider = $xml_raid_content->addChild($arr_option_values['class']); 
		
		// Add any options this raider has
		foreach($arr_option_values as $option_name => $option_value) {
			$new_raider->addAttribute($option_name, $option_value);	
		}
		
		// Ensure that the name of this raider is distinct from any of the existing members
		while( count($xml_raid_content->xpath("player[@name='{$new_raider['name']}']")) > 1 ) {
			$new_raider['name'] = $new_raider['name'].'A';
		}
	}
	
}
?>