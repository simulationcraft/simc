<?php 
/**
 * Should cached copies of data from remote sources be used instead?  Servers, sever-status, etc.
 * @var boolean
 */
define('USE_CACHE_WOW_DATA', true);


/**
 * Should cached copies of the configuration data from simulationcraft source code be used?
 * @var boolean
 */
define('USE_CACHE_CONFIG_OPTIONS', true);

/**
 * Should the simulation be allowed to run?
 * @var boolean
 */
define('ALLOW_SIMULATION', true);

/**
 * The cache file path, relative to the index.php file that is executing all the scripts
 * 
 * note that this file needs to be read/writeable by the user which this script will run as
 * @var string
 */
define('CACHE_FILE_PATH', '../data/cachefile.dat' );

/**
 * Where is the simulationcraft executable directory (relative to the index.php script)
 * 
 * This path is used for running simulationcraft as well as generating the config data from source (when necessary)
 * 
 * With the current setup of the simulationcraft project, the root should be two directories higher
 * @var string
 */
define('SIMULATIONCRAFT_PATH', '../../');


/**
 * This array describes the currently-supported classes  
 * 
 * @var array
 */
$ARR_SUPPORTED_CLASSES = array(	'death_knight', 'druid', 'hunter', 'mage', 'paladin', 'priest', 'rogue', 'shaman', 'warlock', 'warrior' );
?>