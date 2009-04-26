<?php 

/**
 * Fetch the array of WoW servers, with the option to cache them locally
 * 
 * @param $only_active_servers
 * @return unknown_type
 */
function get_arr_wow_servers($only_active_servers=false)
{
	// If we're to use the cached values, use them if they exist
	$arr_cached_servers = read_cache_value('arr_wow_servers');
	if( USE_CACHE_WOW_DATA === true && !is_null($arr_cached_servers) ) {
		
		// Return the cached value
		return $arr_cached_servers;
	}
	
	// Otherwise, fetch the servers from the internet, and cache them
	else {

		// create curl resource
		$ch = curl_init();
		
		// set url
		curl_setopt($ch, CURLOPT_URL, 'http://www.worldofwarcraft.com/realmstatus/status.xml');
		
		// return the transfer as a string
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		
		// $output contains the output string
		$server_xml = curl_exec($ch);
		
		// close curl resource to free up system resources
		curl_close($ch);
		
		// Create an XML object of the servers
		$xml = new SimpleXMLElement($server_xml);
		
		// Assemble the array
		$arr_return = array();
		foreach($xml->xpath('rs/r') as $realm) {
			if($only_active_servers===false || (string)$realm['s']==1 ) {
				$arr_return[] = array( 
						'name' => (string)$realm['n'],
						'status' => (string)$realm['s']==1 ? 'up' : 'down',
					);
			}
		}
		
		// cache these values for future reference
		write_cache_value('arr_wow_servers', $arr_return);
		
		// Return the array of servers
		return $arr_return;
	}
}


?>