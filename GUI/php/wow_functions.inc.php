<?php 
/**
 * Fetch the array of WoW servers, with the option to cache them locally
 * 
 * @param $only_active_servers
 * @return unknown_type
 */
function get_arr_wow_servers( )
{
	// If we're to use the cached values, use them if they exist
	$arr_cached_servers = read_cache_value('arr_wow_servers');
	if( USE_CACHE_WOW_DATA === true && !is_null($arr_cached_servers) ) {
		
		// Return the cached value
		return $arr_cached_servers;
	}
	
	// Otherwise, fetch the servers from the internet, and cache them
	else {
		
		// Build the array of target regions
		$realm_list = array();
		$realm_list['US'] = 'http://www.worldofwarcraft.com/realmstatus/index.xml';
		$realm_list['EU'] = 'http://www.wow-europe.com/realmstatus/index.xml';
		
		// Loop over the various realm lists
		$arr_return = array();
		foreach($realm_list as $list_name => $url) {
			
			// create curl resource
			$ch = curl_init();
			
			// set url
			curl_setopt($ch, CURLOPT_URL, $url);
			
			// return the transfer as a string
			curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
			
			// Pretend to be a browser that understands XML
			curl_setopt($ch, CURLOPT_USERAGENT,  "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.2) Gecko/20070319 Firefox/2.0.0.3");
		
			// $output contains the output string
			$response_xml = curl_exec($ch);
			
			// close curl resource to free up system resources
			curl_close($ch);
			
			// Create an XML object of the response
			$xml = new SimpleXMLElement(preg_replace('/<\?.*\?>/', '', $response_xml));
			
			// Assemble the array
			$arr_return[$list_name] = array();
			foreach($xml->xpath('channel/item') as $realm) {
				
				// Pull out the realm info
				$realm_name = (string) $realm->link;
				$realm_status = 1;

				// Clean up the realm name
				if( strpos($realm_name, 'r=') ) {
					$realm_name = substr( $realm_name, strpos($realm_name, 'r=')+2 );
				}
				else if( strpos($realm_name, '#') ) {
					$realm_name = substr( $realm_name, strpos($realm_name, '#')+1 );
				}
				
				// Add the realm to the list
				$arr_return[$list_name][] = array( 
						'name' => $list_name.':'.$realm_name,
						'label' => $realm_name
					);
			}
		}
		
		// cache these values for future reference
		write_cache_value('arr_wow_servers', $arr_return);
		
		// Return the array of servers
		return $arr_return;
	}
}

/**
 * Given a character name and server name, fetch the character's useful information from the armory
 * 
 * FIXME: This function does not pull the complete list of useful data from the armory return.  The additional fields need to be added at the bottom
 * @param $character_name
 * @param $server_name
 * @return unknown_type
 */
function fetch_character_from_armory($character_name, $server_name)
{
	// If one of the values is missing, don't even bother
	if( empty($character_name) || empty($server_name) ) {
		return false;
	}
	
	// Attempt to pull out the region from the server name (if its there)
	if( strpos($server_name, ':')) {
		list($region_name, $server_name) = explode(':', $server_name);
	}

	// Determine the armory root, from the region
	if( $region_name == 'US') {
		$armory_root_url = 'http://www.wowarmory.com/';
	}
	else if( $region_name == 'EU') {
		$armory_root_url = 'http://eu.wowarmory.com/';
	}
	else {
		$armory_root_url = 'http://www.wowarmory.com/';
	}
	
	
	// === FETCH THE CHARACTER SHEET FOR THE CHARACTER ===
	// character sheet
	$ch = curl_init();
	curl_setopt($ch, CURLOPT_URL, $armory_root_url."character-sheet.xml?r=".urlencode($server_name)."&n=".urlencode($character_name) );
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
	curl_setopt($ch, CURLOPT_USERAGENT,  "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.2) Gecko/20070319 Firefox/2.0.0.3");
	$response_xml = curl_exec($ch);
	curl_close($ch);
	$character_xml = new SimpleXMLElement(preg_replace('/<\?.*\?>/', '', $response_xml));
	if(count($character_xml->xpath('//errorhtml')) > 0) {
		return false;
	}

	// talent sheet
	$ch = curl_init();
	curl_setopt($ch, CURLOPT_URL, $armory_root_url."character-talents.xml?r=".urlencode($server_name)."&n=".urlencode($character_name) );
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
	curl_setopt($ch, CURLOPT_USERAGENT,  "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.2) Gecko/20070319 Firefox/2.0.0.3");
	$response_xml = curl_exec($ch);
	curl_close($ch);
	$talent_xml = new SimpleXMLElement(preg_replace('/<\?.*\?>/', '', $response_xml));
	if(count($talent_xml->xpath('//errorhtml')) > 0) {
		return false;
	}

	// Pull out some useful values
	$class_id = (string)fetch_single_XML_xpath($character_xml, 'characterInfo/character/@classId');
	$talent_string = (string)fetch_single_XML_xpath($talent_xml, 'characterInfo/talentGroups/talentGroup[@active="1"]/talentSpec/@value');
	
	
	// === PREPARE THE CHARACTER OUTPUT ARRAY ===
	// These fields should match up with fields in the config options 
	$arr_character = array();
	
	// base stats
	$arr_character['class'] = strtolower((string)fetch_single_XML_xpath($character_xml, 'characterInfo/character/@class'));
	$arr_character['name'] = strtolower((string)fetch_single_XML_xpath($character_xml, 'characterInfo/character/@name'));
	$arr_character['race'] = strtolower((string)fetch_single_XML_xpath($character_xml, 'characterInfo/character/@race'));
	$arr_character['talents'] = "http://www.wowarmory.com/talent-calc.xml?cid=$class_id&tal=$talent_string";

	// glyphs
	$arr_glyphs = $character_xml->xpath('characterInfo/characterTab/glyphs/glyph/@name');
	if( is_array($arr_glyphs) ) {
		foreach($arr_glyphs as $glyph_name) {
			$stripped_name = str_replace(array('glyph of ', ' '), array('glyph ', '_'), strtolower($glyph_name));
			$arr_character[$stripped_name] = 1; 
		}
	}
	
	// TODO the rest of the fields need to be scraped out of the armory output
	
	// Return the array defining the character
	return $arr_character;
}

?>