<?php
/**
 * Extend php's default SimpleXMLElement to provide some additional functions 
 */
class SimpleXMLElement_XSL extends SimpleXMLElement
{
	/**
	 * Convert the XML object into a string of XML, with an alternate processing-instruction defining
	 * a XSL URL for processing and display.
	 * 
	 * @param string $xsl_path a relative or absolute URL to the appropriate XSL template for this content
	 * @return string the XML object converted to a string
	 */
	public function asXML_with_XSL($xsl_path=null) 
	{
		// Create a DomElement from this simpleXML object
		$dom_sxe = dom_import_simplexml($this);
		
		// Create a new DomDocument
		$dom = new DOMDocument('1.0', 'UTF-8');
		$dom->formatOutput = true;
		
		// If an xsl path has been passed, attach it as a processing instruction to the new DomDocument
		if( !is_null($xsl_path) ) {
			$pi = $dom->createProcessingInstruction("xml-stylesheet", 'type="text/xsl" href="' . $xsl_path . '"' );
			$dom->appendChild($pi);
		}
		
		// Import the DomElement into the DomDocument
		$dom_sxe = $dom->importNode($dom_sxe, true);
		$dom_sxe = $dom->appendChild($dom_sxe);
	
		// Output the DomDocument as an XML string
		return $dom->saveXML();
	}

	/**
	 * Release the XML to the browser, with all the appropriate headers 
	 * @param string $xsl_path
	 * @return null
	 */
	public function release_to_browser( $xsl_path=null )
	{
		// Send the page header for xml content
		header('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
		header('Last-Modified: ' . gmdate('D, d M Y H:i:s') . ' GMT'); // always modified
		header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
		header('Cache-Control: post-check=0, pre-check=0', false); // HTTP/1.1
		header('Pragma: no-cache'); // HTTP/1.0
		header('Content-type: text/xml');
		header( 'X-UA-Compatible: IE=8' );
		
		// Send the output string
		echo $this->asXML_with_XSL($xsl_path);
	}
	
	/**
	 * Append one SimpleXMLElement element to another
	 * 
	 * Thanks to l dot j dot peters at student dot utwente dot nl 
	 * from http://us.php.net/manual/en/function.simplexml-element-addChild.php
	 * 
	 * @param SimpleXMLElement $parent
	 * @param SimpleXMLElement $new_child
	 * @return null
	 */
	static public function simplexml_append( SimpleXMLElement $parent, SimpleXMLElement $new_child )
	{
		$node1 = dom_import_simplexml($parent);
		$dom_sxe = dom_import_simplexml($new_child);
		$node2 = $node1->ownerDocument->importNode($dom_sxe, true);
		$node1->appendChild($node2);
	}
	
	/**
	 * Simple helper function to simplify the laborious xpath process with SimpleXMLElement
	 * @param $str_xpath
	 * @return unknown_type
	 */
	public function fetch_single_XML_xpath( $str_xpath )
	{
		// Attempt to match the element
		$match = $this->xpath($str_xpath);
		
		// One and only one match must be made
		if( !is_array($match) || count($match) == 0) {
			throw new Exception("No Elements were found that match the xpath search ($str_xpath)");
		}
		else if( count($match) > 1) {
			throw new Exception("More than one result was matched on this xpath search ($str_xpath)");
		}
		
		// Return the valid match
		return $match[0];
	}

	/**
	 * Another simple helper function.  This one sets a single property on the fetched element
	 * @param $str_xpath
	 * @param $attribute_name
	 * @param $attribute_value
	 * @return unknown_type
	 */
	public function set_single_xml_xpath_property( $str_xpath, $attribute_name, $attribute_value )
	{
		$target = $this->fetch_single_XML_xpath($str_xpath);
		$target[$attribute_name] = $attribute_value;
	}	
}
?>