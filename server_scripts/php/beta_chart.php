<?
header("Cache-Control: no-cache, must-revalidate");
if (!$patch) $patch = trim(file_get_contents('BETAFOLDER'));
$array = explode('/', $_SERVER['QUERY_STRING']);
$file = $array[0] . '.html';
$number = (is_numeric($array[1]) && $array[1] > 0) ? $array[1] - 1 : 0;
if (($patch == 'nil') || preg_match('/[^a-zA-Z0-9_\-\.]/', $file)) {
	header("Location: pixel.gif");
	exit;
}
if ($file == '.html') $file = 'Raid_T11_372.html';
$string = file_get_contents("$patch/$file", false, NULL, -1, 2000000);
if (preg_match_all('/("|>)(http:\/\/[0-9]\.chart\.apis\.google\.com.*?)("|<)/', $string, $m)) {
	$chart = htmlspecialchars_decode($m[2][$number]);
	$chart = str_replace('DPS  Ranking', $patch[0] . '.' . $patch[1] . '.' . $patch[2] . '+' . str_replace('_', '+', preg_replace('/[0-9]+$/', '', $array[0])), $chart);
	header("Location: $chart");
} else {
	header("Location: pixel.gif");
}
?>
