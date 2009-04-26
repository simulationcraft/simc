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
	 * @param string $xsl a relative or absolute URL to the appropriate XSL template for this content
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

}


/**
 * Thanks to l dot j dot peters at student dot utwente dot nl 
 * from http://us.php.net/manual/en/function.simplexml-element-addChild.php
 * 
 * Append one element to another.
 * @param $parent
 * @param $new_child
 * @return unknown_type
 */
function simplexml_append(SimpleXMLElement $parent, SimpleXMLElement $new_child){
    $node1 = dom_import_simplexml($parent);
    $dom_sxe = dom_import_simplexml($new_child);
    $node2 = $node1->ownerDocument->importNode($dom_sxe, true);
    $node1->appendChild($node2);
} 

?>