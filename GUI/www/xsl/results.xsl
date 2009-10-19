<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<xsl:output method="html" encoding="utf-8" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" cdata-section-elements="pre script style" indent="yes" />
	
	<!--  Top-level XML tag this matches the root of the xml, and is the top of the xsl parsing tree -->
	<xsl:template match="/xml">
	<html>
	
		<head>
			<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
			<meta name="description" content="A tool to explore combat mechanics in the popular MMO RPG World of Warcraft (tm)" />
			<meta name="keywords" content="simulationcraft,simcraft,simc,world of warcraft,wow,simulation" />
			
			<title>SimulationCraft Results</title>

			<!-- Thanks to http://www.favicon.cc/?action=icon&file_id=3266 for the favicon Creative Commons, no attribution necessary -->
			<link rel="icon" type="image/x-icon" href="images/favicon.ico" />
			
			<!-- CSS File links -->
			<!-- <link rel="stylesheet" type="text/css" media="all" title="Default Style" href="css/reset.css" /> -->
			<link rel="stylesheet" type="text/css" media="all" title="Default Style" href="css/results.css" />
			
			<!-- Javascript file includes -->
			<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.2.6/jquery.min.js"></script>
			<script type="text/javascript" src="js/jquery.corner.js"></script>
			<script type="text/javascript" src="js/results.js"></script>
		</head>
		
		<body>

			<!-- Raid Summary Data -->
			<h1>Raid</h1>
			<xsl:for-each select="raid_summary/chart">
				<img>
					<xsl:attribute name="src"><xsl:value-of select="@url" /></xsl:attribute>
				</img>
			</xsl:for-each>
			
			<hr />
			
			
			
			<!-- Scale Factors -->
			<xsl:if test="players/player/scale_factors">
				<h1>DPS Scale Factors (dps increase per unit stat)</h1>					

				<table class="scale_factors">
					<thead>
					  <tr>
							<xsl:for-each select="players/player[1]/scale_factors/scale_factor">
								<th><xsl:value-of select="@name" /></th>
							</xsl:for-each>
							
							<xsl:for-each select="players/player[1]/scale_factors/scale_factor_link">
								<th><xsl:value-of select="@type" /></th>
							</xsl:for-each>
							
							<xsl:for-each select="players/player[1]/scale_factors/pawn_string">
								<th>Pawn</th>
							</xsl:for-each>
					  </tr>
					</thead>
					
					<tbody>
						<xsl:for-each select="players/player">
						  <tr>
								<xsl:for-each select="scale_factors/scale_factor">
									<td>
										<xsl:value-of select="@value" />
									</td>
								</xsl:for-each>
								
								<xsl:for-each select="scale_factors/scale_factor_link">
									<td>
										<a>
											<xsl:attribute name="href"><xsl:value-of select="@url" /></xsl:attribute>
											<xsl:value-of select="@type" />
										</a>
									</td>
								</xsl:for-each>
								
								<xsl:for-each select="scale_factors/pawn_string">
									<td class="pawn">
										<xsl:value-of select="." />
									</td>
								</xsl:for-each>
							</tr>
						</xsl:for-each>
					</tbody>
				</table>
			</xsl:if>

			<hr />



			<!-- Menu -->
			<a href="javascript:hideElement(document.getElementById('trigger_menu'));">-</a> <a href="javascript:showElement(document.getElementById('trigger_menu'));">+</a> Menu
			<div id="trigger_menu" style="display:none;">
			  <a href="javascript:hideElements(document.getElementById('players').getElementsByTagName('img'));">-</a> <a href="javascript:showElements(document.getElementById('players').getElementsByTagName('img'));">+</a> All Charts<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_dpet'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_dpet'));">+</a> DamagePerExecutionTime<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_uptimes'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_uptimes'));">+</a> Up-Times and Procs<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_sources'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_sources'));">+</a> Damage Sources<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_gains'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_gains'));">+</a> Resource Gains<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_dps_timeline'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_dps_timeline'));">+</a> DPS Timeline<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_dps_distribution'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_dps_distribution'));">+</a> DPS Distribution<br />
			  <a href="javascript:hideElements(document.getElementsByName('chart_resource_timeline'));">-</a> <a href="javascript:showElements(document.getElementsByName('chart_resource_timeline'));">+</a> Resource Timeline<br />
			</div>
			
			<hr />
			
			
			
			<!-- Players -->
			<div id="players">
				<xsl:for-each select="players/player">
					<h1>
						<a>
							<xsl:attribute name="href"><xsl:value-of select="@talent_url" /></xsl:attribute>
							<xsl:value-of select="@name" />
						</a>
					</h1>
					
					<xsl:for-each select="chart">
						<img>
							<xsl:attribute name="name"><xsl:value-of select="@type" /></xsl:attribute>
							<xsl:attribute name="src"><xsl:value-of select="@url" /></xsl:attribute>
						</img>
					</xsl:for-each>

					<hr />
				</xsl:for-each>
			</div>
			
			
			
			<!-- Raw Simulation Output -->
			<h1>Raw Text Output</h1>
			
			<ul>
			 <li><b>DPS=Num:</b> <i>Num</i> is the <i>damage per second</i></li>
			 <li><b>DPR=Num:</b> <i>Num</i> is the <i>damage per resource</i></li>
			 <li><b>RPS=Num1/Num2:</b> <i>Num1</i> is the <i>resource consumed per second</i> and <i>Num2</i> is the <i>resource regenerated per second</i></li>
			 <li><b>Count=Num|Time:</b> <i>Num</i> is number of casts per fight and <i>Time</i> is average time between casts</li>
			 <li><b>DPE=Num:</b> <i>Num</i> is the <i>damage per execute</i></li>
			 <li><b>DPET=Num:</b> <i>Num</i> is the <i>damage per execute</i> divided by the <i>time to execute</i> (this value includes GCD costs and Lag in the calculation of <i>time to execute</i>)</li>
			 <li><b>Hit=Num:</b> <i>Num</i> is the average damage per non-crit hit</li>
			 <li><b>Crit=Num1|Num2|Pct:</b> <i>Num1</i> is average crit damage, <i>Num2</i> is the max crit damage, and <i>Pct</i> is the percentage of crits <i>per execute</i> (not <i>per hit</i>)</li>
			 <li><b>Tick=Num:</b> <i>Num</i> is the average tick of damage for the <i>damage over time</i> portion of actions</li>
			 <li><b>Up-Time:</b> This is <i>not</i> the percentage of time the buff/debuff is present, but rather the ratio of <i>actions it affects</i> over <i>total number of actions it could affect</i>.  If spell S is cast 10 times and buff B is present for 3 of those casts, then buff B has an up-time of 30%.</li>
			 <li><b>Waiting</b>: This is percentage of total time not doing anything (except auto-attack in the case of physical dps classes).  This can occur because the player is resource constrained (Mana, Energy, Rage) or cooldown constrained (as in the case of Enhancement Shaman).</li>
			</ul>
	
			<pre>
				<xsl:value-of select="simulation_details/raw_text" />
			</pre>
		</body>
	</html>
	</xsl:template> 

</xsl:stylesheet>