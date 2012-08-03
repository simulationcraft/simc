<?
function is_available($url, $timeout = 30) {
        $ch = curl_init(); // get cURL handle

        // set cURL options
        $opts = array(CURLOPT_RETURNTRANSFER => true, // do not output to browser
                                  CURLOPT_URL => $url,            // set URL
                                  CURLOPT_NOBODY => false,                 // do a HEAD request only
                                  CURLOPT_TIMEOUT => $timeout);   // set timeout
        curl_setopt_array($ch, $opts); 

        curl_exec($ch); // do it!

        $test = curl_getinfo($ch, CURLINFO_HTTP_CODE);
	$retval = $test == 200; // check if HTTP OK

        curl_close($ch); // close handle

        return $retval;
}

if (!$platform) $platform = "win";
$version = trim(file_get_contents("REALRELEASE_BETA$platform"));
$new = trim(file_get_contents("FRESHRELEASE_BETA"));
if ($version != $new) {
	if ($platform == "win") $filename = "simc-$new-win32.zip";
	else if ($platform == "mac") $filename = "simc-$new-osx-x86.dmg";
	if (is_available("http://simulationcraft.googlecode.com/files/$filename")) {
		copy("FRESHRELEASE_BETA", "REALRELEASE_BETA$platform");
		$version = $new;
	}
}
if ($platform == "win") $filename = "simc-$version-win32.zip";
else if ($platform == "mac") $filename = "simc-$version-osx-x86.dmg";
else {
	header("HTTP/1.0 404 Not Found");
	exit;
}
header("Location: http://simulationcraft.googlecode.com/files/$filename");
?>
