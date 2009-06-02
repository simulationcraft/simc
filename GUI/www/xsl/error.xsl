<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<xsl:output method="html" encoding="utf-8" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" cdata-section-elements="pre script style" indent="yes" />
	
	<!--  Top-level XML tag this matches the root of the xml, and is the top of the xsl parsing tree -->
	<xsl:template match="/xml">
	<html>
	
		<head>
			<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
			<meta name="description" content="A tool to explore combat mechanics in the popular MMO RPG World of Warcraft (tm)" />
			<meta name="keywords" content="simulationcraft,simcraft,world of warcraft,wow,simulation" />
			<title>SimulationCraft Error</title>

			<!-- Thanks to http://www.favicon.cc/?action=icon&file_id=3266 for the favicon Creative Commons, no attribution necessary -->
			<link rel="icon" type="image/x-icon" href="images/favicon.ico" />
			
			<!-- CSS File links -->
			<link rel="stylesheet" type="text/css" media="all" title="Default Style" href="css/reset.css" />
			<link rel="stylesheet" type="text/css" media="all" title="Default Style" href="css/error.css" />
			
			<!-- Javascript file includes -->
			<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.2.6/jquery.min.js"></script>
			<script type="text/javascript" src="js/jquery.corner.js"></script>
			<script type="text/javascript" src="js/error.js"></script>
		</head>
		
		<body>
			<h1>We're Sorry, There's been an error</h1>
			<p><xsl:value-of select="exception/@message" /></p>
		</body>
	</html>
	</xsl:template> 

</xsl:stylesheet>