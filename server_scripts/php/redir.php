<?
$patch = trim(file_get_contents('LIVEFOLDER'));
$file = $_SERVER['QUERY_STRING'] . '.html';
if ($file == '.html') $file = 'Raid_T11_372.html';
if (($patch == 'nil') || preg_match('/[^a-zA-Z0-9_\-\.]/', $file)) {
	header("HTTP/1.0 404 Not Found");
} else {
	header("Location: $patch/$file");
}
?>
