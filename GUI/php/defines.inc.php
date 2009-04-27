<?php 
/**
 * Should cached copies of data from remote sources be used instead?  Servers, sever-status, etc.
 * @var boolean
 */
define('USE_CACHE_WOW_DATA', true);


/**
 * Should cached copies of the configuration data from simcraft source code be used?
 * @var boolean
 */
define('USE_CACHE_CONFIG_OPTIONS', true);

/**
 * The cache file path, relative to the index.php file that is executing all the scripts
 * 
 * note that this file needs to be read/writeable by the user which this script will run as
 * @var string
 */
define('CACHE_FILE_PATH', '../data/cachefile.dat');

/**
 * Where is the simulationcraft executable directory (relative to the index.php script)
 * 
 * This path is used for running simcraft as well as generating the config data from source (when necessary)
 * 
 * With the current setup of the simulationcraft project, the root should be two directories higher
 * @var string
 */
define('SIMULATIONCRAFT_PATH', '../../');


/**
 * This array describes the currently-supported classes, and which C++ files contain their unique options  
 * 
 * @var array
 */
$ARR_SUPPORTED_CLASSES = array(
		//'death_knight' 		=> array('source_file'=>'sc_death_knight.cpp' ),
		'druid' 					=> array('source_file'=>'sc_druid.cpp' ),
		'hunter' 					=> array('source_file'=>'sc_hunter.cpp' ),
		'mage' 						=> array('source_file'=>'sc_mage.cpp' ),
		//'paladin' 				=> array('source_file'=>'sc_paladin.cpp' ),
		'priest' 					=> array('source_file'=>'sc_priest.cpp' ),
		'rogue' 					=> array('source_file'=>'sc_rogue.cpp' ),
		'shaman' 					=> array('source_file'=>'sc_shaman.cpp' ),
		'warlock' 				=> array('source_file'=>'sc_warlock.cpp' ),
		'warrior' 				=> array('source_file'=>'sc_warrior.cpp' )
	);

	
/**
 * This array lists C++ source files which are considered to contain additional class-independent player options
 * @var array
 */
$ARR_PLAYER_RELATED_SOURCE_FILES = array(
		'sc_player.cpp',
		'sc_unique_gear.cpp'
	);
?>