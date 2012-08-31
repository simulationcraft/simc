<?
header("Cache-Control: no-cache, must-revalidate");
if (!$patch) $patch = trim(file_get_contents('LIVEFOLDER'));
$array = explode('/', $_SERVER['QUERY_STRING']);
$file = $array[0] . '.html';
$number = (is_numeric($array[1]) && $array[1] > 0) ? $array[1] - 1 : 0;
if (($patch == 'nil') || preg_match('/[^a-zA-Z0-9_\.]/', $file)) {
	header("Location: pixel.gif");
	exit;
}
if ($file == '.html') $file = 'Raid_T11_372.html';
$string = file_get_contents("$patch/$file", false, NULL, -1, 200000);
if (preg_match_all('/("|>)(http:\/\/[0-9]\.chart\.apis\.google\.com.*?)("|<)/', $string, $m)) {
	$chart = htmlspecialchars_decode($m[2][$number]);
	$chart = str_replace('DPS  Ranking', $patch[0] . '.' . $patch[1] . '.' . $patch[2] . '+' . str_replace('_', '+', $array[0]), $chart);
	if (preg_match('/chd=t:([\|0-9]+)/', $chart, $m)) {
		$array = explode('|', $m[1]);
		$value = $array[count($array) - 1];
		foreach ($array as $a) {
			$newvalue = $a - $value;
			$chart = str_replace("++$a++", "++$newvalue++", $chart); 
			$newarray[] = $newvalue;
		}
		$chart = str_replace($m[1], implode('|', $newarray), $chart);
	}
	if (preg_match('/chds=0,([0-9]+)/', $chart, $m)) {
		$max = max($array);
		$newsize = round($m[1] * ( ( $max - $value ) / $max ));
		$chart = str_replace($m[0], "chds=0,$newsize", $chart);
	}
	header("Location: $chart");
} else {
	header("Location: pixel.gif");
}
?>
