<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml">

	<xsl:output method="html" encoding="utf-8" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" cdata-section-elements="pre script style" indent="yes" />
	
	<!--  Top-level XML tag this matches the root of the xml, and is the top of the xsl parsing tree -->
	<xsl:template match="/xml">

	<html xmlns="http://www.w3.org/1999/xhtml">
		
		<head>
			<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
			<meta name="description" content="A tool to explore combat mechanics in the popular MMO RPG World of Warcraft (tm)" />
			<meta name="keywords" content="simulationcraft,simulationcraft,simc,world of warcraft,wow,simulation" />
			
			<title>SimulationCraft</title>

			<!-- Thanks to http://www.favicon.cc/?action=icon&file_id=3266 for the favicon Creative Commons, no attribution necessary -->
			<link rel="icon" type="image/x-icon" href="images/favicon.ico" />
			
			<!-- CSS File links -->
			<link rel="stylesheet" type="text/css" media="all" title="Default Style" href="css/reset.css" />
			<link rel="stylesheet" type="text/css" media="all" title="Default Style" href="css/config_form.css" />
			
			<!-- Javascript file includes -->
			<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.2.6/jquery.min.js"></script>
			<script type="text/javascript" src="js/jquery.corner.js"></script>
			<script type="text/javascript" src="js/config_form.js"></script>
		</head>
				
		<body>
		
			<!-- Call the template defined below, which handles initializing the prototypical new-raiders in javascript -->
			<xsl:call-template name="javascript_prefill" />

			<!-- Match on any exceptions, and show the warning (template defined below) -->
			<xsl:apply-templates select="exceptions" />

			<!-- The overall page content -->		
			<div id="content_wrapper">
				
				<!-- Add the title bar -->
				<xsl:call-template name="titlebar" />

				<!-- Add the raid configuration form -->
				<form id="config_form" name="config_form" action="simulation.xml" method="post" enctype="multipart/form-data">										
										
					<!-- Add the side bar -->
					<xsl:call-template name="sidebar" />

					<!-- Add the bottom bar -->
					<xsl:call-template name="bottombar" />
		
					<!-- Add the main content form -->										
					<xsl:call-template name="content_form" />
				</form>	
			</div>
		</body>
	</html>
	</xsl:template>

	<!-- === ERRORS === -->
	<xsl:template match="exceptions">
		<div id="error_report">
			<div class="close_button" title="Close this error box">Close</div>
			<ul>
			<xsl:for-each select="exception">
				<li><xsl:value-of select="@message" /></li>
			</xsl:for-each>
			</ul>
		</div>
	</xsl:template>

	<!-- === TITLE === -->
	<xsl:template name="titlebar">
		<div id="title_bar">
			<h1><a href="http://code.google.com/p/simulationcraft/" target="_blank">SimulationCraft</a></h1>
			<p>A World of Warcraft Combat Simulation</p>
		</div>
	</xsl:template>

	<!-- === SIDEBAR === -->
	<xsl:template name="sidebar">

		<!-- Sidebar (handles adding new members to the raid) -->	
		<div id="sidebar">
		
			<!--  Menu of classes, used to add new members to the raid -->
			<div class="sidebar_section">
				<h1>Add by Class</h1>
								
				<!-- The list of buttons to add a new raider by class -->
				<ul id="new_member_by_class">
				
					<!-- For each of the classes defined in the XML file (Except 'all_classes'), add a list element -->
					<xsl:for-each select="/xml/options/player/*[not(name()='all')]">
						<xsl:sort select="name()" />
					
						<li>
							<xsl:attribute name="class">supported_class <xsl:value-of select="name()" /></xsl:attribute>
							<xsl:attribute name="title"><xsl:value-of select="@title" /></xsl:attribute>
							<span><xsl:value-of select="@title" /></span>
						</li>
					
					</xsl:for-each>
				</ul>	

				<!-- Instructional caption -->
				<span class="sidebar_caption">Select one of the above classes to add a new member to the raid</span>
			</div>

			<!--  Menu of classes, used to add new members to the raid -->
			<div class="sidebar_section">
				<h1>Add from Armory</h1>
								
				<ul id="new_member_by_armory" class="subscript_labels">
					<li>
						<label for="armory_server">Server</label>
						<select name="add_from_armory[server]" id="armory_server">
							<option value=""></option>
							
							<xsl:for-each select="/xml/servers/server">
								<xsl:if test="position()=1 or @region != preceding-sibling::server[1]/@region">
									
									<optgroup>
										<xsl:attribute name="label"><xsl:value-of select="@region" /></xsl:attribute>
									
										<xsl:variable name="region"><xsl:value-of select="@region" /></xsl:variable>
										<xsl:for-each select="/xml/servers/server[@region=$region]">
											<xsl:sort select="@region" />
											<xsl:sort select="@label" />
											<option>
												<xsl:attribute name="value"><xsl:value-of select="concat(@region, ':', @name)" /></xsl:attribute>
												<xsl:if test="/xml/options/@selected_server = concat(@region, ':', @name)">
													<xsl:attribute name="selected">selected</xsl:attribute>
												</xsl:if>
												<xsl:value-of select="@name" /> (<xsl:value-of select="@region" />)</option>
										</xsl:for-each>
									</optgroup>
								</xsl:if>
							</xsl:for-each>
						</select>
					</li>
					
					<li>
						<label for="armory_character_name">Character Name</label>
						<input type="text" name="add_from_armory[name]" id="armory_character_name" />
					</li>
				</ul>
				
				<input type="submit" value="Import" name="import_from_armory" />
			</div>
		</div>
	</xsl:template>

	<!--  === FLOATING BOTTOM BAR === -->
	<xsl:template name="bottombar">
		<div id="bottombar">
		
			<!-- The 'run' buttons -->
			<div class="bottombar_section">
				<h3>Simulation</h3>
								
				<xsl:if test="/xml/options/@allow_simulation='1'">
					<input name="simulate" id="simulate" type="submit" value="Run Simulation" title="Execute the SimulationCraft simulation and display the results" />
				</xsl:if>
				
				<input type="reset" value="Reset Form" title="Reset the fields of this simulation to their values as of the past page load" />
			</div>
			
			<!-- The config file import functions -->
			<div class="bottombar_section">
				<h3>Import SimulationCraft Files</h3>
				
				<ul class="subscript_labels stacked_fields">
					<li>
						<label for="import_file_path">Import File</label>
						
						<!-- Max upload file size, in bytes -->
						<input name="MAX_FILE_SIZE" type="hidden" value="30000">
							<xsl:attribute name="value"><xsl:value-of select="/xml/options/@max_file_size" /></xsl:attribute>
						</input>					
						<input type="file" name="import_file_path" id="import_file_path" title="Which file should be imported?" />
					</li>
					
					<li>
						<label for="clear_before_import">Clear Form?</label>
						<input name="clear_before_import" id="clear_before_import" type="checkbox" value="1" title="Should the current simulation be cleared before adding this file's configuration?" />
					</li>
								
					<li>
						<input name="import_file" type="submit" value="Import from File" title="Import the specified .simc file into the current simulation" />
					</li>
				</ul>
			</div>

			<!-- The config file export functions -->
			<div class="bottombar_section">
				<h3>Export SimulationCraft Files</h3>
				
				<input name="export_file" type="submit" value="Export to File" title="Download an export of the current simulation build to a .simc file" />
			</div>

		</div>
	</xsl:template>
	
	<!-- === THE MAIN SUBMISSION FORM === -->
	<xsl:template name="content_form">
		<div id="config_content">
		
			<!-- Raid Composition -->										
			<div id="raid_composition" class="page_panel">
				<h2 class="page_panel">Raid Composition</h2>
								
				<!--  The main list of raid members (generated with a template defined below) -->
				<ul id="raid_members">
					<xsl:apply-templates select="/xml/raid_content/*" />
				</ul>
			</div>	


			<!-- Global simulation settings -->
			<div id="global_settings" class="page_panel">
				<h2 class="page_panel">Global Settings</h2>

				<div id="global_options_wrapper">
	
					<!-- List of options for this user (except for class and name, which were explicity positioned above already) -->
					<xsl:call-template name="option_list">
						<xsl:with-param name="which" select="/xml/options/global/*" />
						<xsl:with-param name="field_root_name" select="'globals'" />
						<xsl:with-param name="index" select="''" />
					</xsl:call-template>
					
				</div>
			</div>
		</div>
	</xsl:template>
	
	<!-- === Javascript support section, describing the classes === -->
	<xsl:template name="javascript_prefill">
		<div class="hidden" id="prototype_character_classes">
			<xsl:for-each select="/xml/options/player/*[not(name()='all')]">
	
				<!-- Because javascript sucks and has no HEREDOC equivalent, we have to do this the hard way... -->
				<!-- each class will get a hidden div holding a prototype new-raider, with a replacable tag string in place of it's index number -->
				<!-- On add, javascript will copy this hidden div, and replace the tag string with the actual index number -->
				<div class="hidden">
					<xsl:attribute name="id">hidden_div_<xsl:value-of select="name()" /></xsl:attribute>

					<ul>
						<!-- Call the template that handles displaying a raider instance (same as the one used to print the visible raider list elements) -->
						<xsl:call-template name="raid_content_player">
							<xsl:with-param name="index" select="'{INDEX_VALUE}'" />
						</xsl:call-template>
					</ul>
				</div>
				
			</xsl:for-each>
		</div>
	</xsl:template>
		
	<!-- === raider element === -->
	<!-- players in the raid-content tag.  This template gets used to generate the visible elements as well as the prototypical hidden raiders, 
			which get used by javascript to insert new players -->
	<xsl:template match="raid_content/*" name="raid_content_player">
		
		<!-- What index should we use (default to the XML element's position index) -->
		<xsl:param name="index" select="position()" />

		<!-- What's the character-class for this new player? (defaults to the player tag's name attribute, if its present) -->
		<xsl:param name="class" select="name()" />

		<li class="raider">

			<!-- Clone button -->
			<div class="clone_button" title="Clone this Player">Clone</div>						

			<!-- Close button -->
			<div class="close_button" title="Remove this Player">Close</div>						
		
		
			<!--  Class description block (with the text looked up in the supported_classes tag) -->
			<div>
				<xsl:attribute name="class">member_class <xsl:value-of select="$class" /></xsl:attribute>
				<xsl:value-of select="/xml/options/player/*[name()=$class]/@title" /> - 

				<!-- option defining the name for the raider -->
				<xsl:call-template name="simulationcraft_option">
					<xsl:with-param name="type">text</xsl:with-param>
					<xsl:with-param name="variable_name">raider</xsl:with-param>
					<xsl:with-param name="array_index"><xsl:value-of select="$index" /></xsl:with-param>
					<xsl:with-param name="field_name">name</xsl:with-param>
					<xsl:with-param name="value"><xsl:value-of select="@name" /></xsl:with-param>
					<xsl:with-param name="show_label" select="'false'" />
				</xsl:call-template>
			</div>
			
			
			<!-- Hidden option variable defining the class for the raider -->
			<xsl:call-template name="simulationcraft_option">
				<xsl:with-param name="type">hidden</xsl:with-param>
				<xsl:with-param name="variable_name">raider</xsl:with-param>
				<xsl:with-param name="array_index"><xsl:value-of select="$index" /></xsl:with-param>
				<xsl:with-param name="field_name">class</xsl:with-param>
				<xsl:with-param name="value"><xsl:value-of select="$class" /></xsl:with-param>
			</xsl:call-template>
			

			<!-- List of options for this user (except for class and name, which were explicity positioned above already) -->
			<xsl:call-template name="option_list">
				<xsl:with-param name="which" select="/xml/options/player/*[name()='all' or name()=$class]/*" />
				<xsl:with-param name="field_root_name" select="'raider'" />
				<xsl:with-param name="index" select="$index" />
				<xsl:with-param name="field_value_source_element" select="." />
			</xsl:call-template>
		</li>
	</xsl:template>

	<!-- A group of options for a raid member -->
	<xsl:template name="option_list">
		
		<!-- Which option elements should be selected to build the list? -->
		<xsl:param name="which" />
		
		<!-- The root name of the option HTML field names -->
		<xsl:param name="field_root_name" />
		
		<!-- What index should we use in the HTML field name (ensures that PHP gets an array of values) -->
		<xsl:param name="index" />
		
		<!-- 'this' raider, defined by the current context.  Used below in a loop, where the context would be different -->
		<xsl:param name="field_value_source_element" select="false()" />
		
		
		<div class="content_list">
		
			<!-- Set up the menus for the list -->
			<xsl:call-template name="option_menu">
				<xsl:with-param name="which" select="$which" />
				<xsl:with-param name="field_root_name" select="$field_root_name" />
				<xsl:with-param name="index" select="$index" />
			</xsl:call-template>
			
			
			<!-- Set up the divs for the list -->
			<xsl:call-template name="option_panes">
				<xsl:with-param name="which" select="$which" />
				<xsl:with-param name="field_root_name" select="$field_root_name" />
				<xsl:with-param name="index" select="$index" />
				<xsl:with-param name="field_value_source_element" select="$field_value_source_element" />
			</xsl:call-template>
		</div>
		
	</xsl:template>

	<!-- Menu bars for the option-lists -->
	<xsl:template name="option_menu">

		<!-- Which option elements should be selected to build the list? -->
		<xsl:param name="which" />
		
		<!-- The root name of the option HTML field names -->
		<xsl:param name="field_root_name" />
		
		<!-- What index should we use in the HTML field name (ensures that PHP gets an array of values) -->
		<xsl:param name="index" />

		<xsl:param name="with_parent_id" select="false()" />
	
		<!-- If there are any sub-elements that aren't options -->
		<xsl:if test="$which[not(name()='option')]">
			
			<!-- build their menu structure as a hierarchical list -->
			<ul>
				<xsl:attribute name="class">menu<xsl:if test="$with_parent_id"> child_menu child_of_<xsl:value-of select="generate-id($which/..)" /></xsl:if></xsl:attribute>
		
				<!-- Loop over each of the elements passed (should be optdoc 'tiers') -->
				<xsl:for-each select="$which[not(name()='option')]">
					<xsl:sort select="name()" />

					<li>
						<xsl:attribute name="id">menu_item_<xsl:value-of select="generate-id(.)" /></xsl:attribute>
						<span class="menu_tab"><xsl:value-of select="@title" /></span>
					</li>

				</xsl:for-each>
			</ul>
			
			<!-- Recurse for any sub-lists -->
			<xsl:call-template name="option_menu">
				<xsl:with-param name="which" select="$which[not(name()='option')]/*[not(name()='option')]" />
				<xsl:with-param name="field_root_name" select="$field_root_name" />
				<xsl:with-param name="index" select="$index" />
				<xsl:with-param name="with_parent_id" select="true()" />
			</xsl:call-template>

			
		</xsl:if>	
	</xsl:template>

	<!-- Option content blocks for the option-lists -->
	<xsl:template name="option_panes">

		<!-- Which option elements should be selected to build the list? -->
		<xsl:param name="which" />
		
		<!-- The root name of the option HTML field names -->
		<xsl:param name="field_root_name" />
		
		<!-- What index should we use in the HTML field name (ensures that PHP gets an array of values) -->
		<xsl:param name="index" />

		<!-- 'this' raider, defined by the current context.  Used below in a loop, where the context would be different -->
		<xsl:param name="field_value_source_element" select="false()" />

	
		<!-- If there are any sub-elements that aren't options -->
		<xsl:if test="$which[not(name()='option')]">
			
			<!-- Loop over each of the elements passed (should be optdoc 'tiers') -->
			<xsl:for-each select="$which[not(name()='option')]">
				<xsl:sort select="name()" />

				<xsl:if test="option">
				
					<div class="option_pane">
						<xsl:attribute name="id">menu_pane_<xsl:value-of select="generate-id(.)" /></xsl:attribute>
					
						<!-- Show any options at this level -->			
						<ul class="stacked_fields subscript_labels">
					
							<!-- For each of the options selected in the $which parameter, build a list-element for it -->
							<xsl:for-each select="option">
								<xsl:sort select="@name" />
					
								<li>
								
									<!-- Mark checkbox list elements specially, for CSS hooking -->
									<xsl:if test="@type='boolean'">
										<xsl:attribute name="class">checkbox</xsl:attribute>
									</xsl:if>
			
									<!-- Call the template that handles options, with the appropriate parameters -->
									<!-- If a value was already set for this option, don't use the parent element's attribute-value for the value (use the option's value attribute) -->
									<xsl:choose>
										
										<xsl:when test="$field_value_source_element">
											<xsl:variable name="this_field_name" select="@name" />
											<xsl:variable name="this_field_value" select="$field_value_source_element/@*[name()=$this_field_name]" />
										
											<xsl:apply-templates select=".">
												<xsl:with-param name="variable_name"><xsl:value-of select="$field_root_name" /></xsl:with-param>
												<xsl:with-param name="array_index"><xsl:value-of select="$index" /></xsl:with-param>
												<xsl:with-param name="value"><xsl:value-of select="$this_field_value" /></xsl:with-param>
											</xsl:apply-templates>
										</xsl:when>
										
										<xsl:otherwise>
											<xsl:apply-templates select=".">
												<xsl:with-param name="variable_name"><xsl:value-of select="$field_root_name" /></xsl:with-param>
												<xsl:with-param name="array_index"><xsl:value-of select="$index" /></xsl:with-param>
											</xsl:apply-templates>
										</xsl:otherwise>
									</xsl:choose>
									
								</li>
								
							</xsl:for-each>
						</ul>
					</div>
				</xsl:if>
				
				<!-- Recurse for any sub-lists -->
				<xsl:call-template name="option_panes">
					<xsl:with-param name="which" select="./*[not(name()='option')]" />
					<xsl:with-param name="field_root_name" select="$field_root_name" />
					<xsl:with-param name="index" select="$index" />
					<xsl:with-param name="field_value_source_element" select="$field_value_source_element" />
				</xsl:call-template>

			</xsl:for-each>
		</xsl:if>	
	</xsl:template>		

	<!-- === OPTION FIELDS === -->
	<!-- Any of the option fields in the page, this template gets called in multiple places above -->
	<xsl:template match="option" name="simulationcraft_option">
	
		<!-- Optional type override, defaults to the option's type attribute -->
		<xsl:param name="type" select="@type" />

		<!-- Optional value override, defaults to the option's value attribute -->
		<xsl:param name="value" select="@value" />
		
		<!-- the variable name to use in the form submission -->
		<xsl:param name="variable_name" />
		
		<!--An alternate index attribute, to turn the variable into an array -->
		<xsl:param name="array_index" select="''"  />
	
		<!-- the name of the option parameter -->
		<xsl:param name="field_name" select="@name" />

		<!-- The label to show the user -->
		<xsl:param name="field_label" select="@label" />
	
		<!-- Should the field label be shown? -->
		<xsl:param name="show_label" select="'true'" />

	
		<!-- Generate the string to use for ID's -->
		<xsl:variable name="id_string"><xsl:value-of select="$variable_name" />_<xsl:if test="not($array_index='')"><xsl:value-of select="$array_index"/>_</xsl:if><xsl:value-of select="$field_name" /></xsl:variable>

		<!-- Generate the string to use for the field name -->
		<xsl:variable name="name_string"><xsl:value-of select="$variable_name" /><xsl:if test="not($array_index='')">[<xsl:value-of select="$array_index"/>]</xsl:if>[<xsl:value-of select="$field_name" />]</xsl:variable>

		<!-- Show the label for non-hidden fields -->
		<xsl:if test="not($type='hidden') and $show_label='true'">
			<label>
				<xsl:attribute name="for"><xsl:value-of select="$id_string" /></xsl:attribute>
				<xsl:value-of select="$field_label" />
			</label>
		</xsl:if>
		
		
		<!-- Build the input field -->
		<input>
		
			<!-- Set the field name and id attributes -->
			<xsl:attribute name="name"><xsl:value-of select="$name_string" /></xsl:attribute>
			<xsl:attribute name="id"><xsl:value-of select="$id_string" /></xsl:attribute>
		
			<!-- Set the css class if it's set -->
			<xsl:attribute name="class">field_<xsl:value-of select="$field_name" /></xsl:attribute>
		
			<!-- set the input-type from the xml type attribute -->	
			<xsl:attribute name="type">
				<xsl:choose>
					<xsl:when test="$type='integer'">text</xsl:when>
					<xsl:when test="$type='float'">text</xsl:when>
					<xsl:when test="$type='string'">text</xsl:when>
					<xsl:when test="$type='boolean'">checkbox</xsl:when>
					<xsl:when test="$type='hidden'">hidden</xsl:when>
					<xsl:otherwise></xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>									
		
			<!-- For non-checkbox fields, set the default value from the value attribute -->
			<xsl:attribute name="value">
				<xsl:choose>
					<xsl:when test="$type='boolean'">1</xsl:when>
					<xsl:otherwise><xsl:value-of select="$value" /></xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>

			<!--  for boolean types, set the checked value of the checkbox from the value -->		
			<xsl:if test="$type='boolean' and $value=1">
				<xsl:attribute name="checked">checked</xsl:attribute>
			</xsl:if>
		
		</input>					
	</xsl:template> 

</xsl:stylesheet>