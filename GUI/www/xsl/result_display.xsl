<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<xsl:output method="html" encoding="utf-8" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" cdata-section-elements="pre script style" indent="no" />
		
	<!--  Top-level XML tag this matches the root of the xml, and is the top of the xsl parsing tree -->
	<xsl:template match="/xml">
	
	<html xmlns="http://www.w3.org/1999/xhtml">
		
		<head>
			<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
			<meta name="description" content="" />
			<meta name="keywords" content="" />
			
			<title>SimulationCraft</title>
		</head>
				
		<body>
			<xsl:value-of select="generated_report" disable-output-escaping="yes" />
		</body>
	</html>

	</xsl:template>

</xsl:stylesheet>