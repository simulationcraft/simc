
/**
 * Initalize the insert counter, to keep the html form submission field names straight
 */
var last_insert_number = 1;

/**
 * When the page is sufficiently loaded, run this setup functionality 
 */
$( function() {
	
	// Reset the event handlers
	reset_handlers();
	
	/*  Clicking a member of the new-member list should insert a new raider of that class */
	$('ul#new_member li.supported_class').click( function() {
		
		// Build an array of the css classes this element has
		var classes = $(this).attr('class').split(' ');
		
		// Loop over the array of css classes that this clicked element had
		for (var i=0; i<classes.length; i++) {
						
			// If this class name corresponds to one of the indices in arr_prototype_class, then insert this element
			if( $('div#hidden_div_'+classes[i]).size() ) {
				
				// Create the string of html for the new element
				var new_member = $('div#hidden_div_'+classes[i]).html(); 
				
				// Replace the counter placeholder string in the text with the real index
				last_insert_number++;
				new_member = new_member.replace(/{INDEX_VALUE}/g, last_insert_number);
				
				// Append the new list element to the list of raiding players
				$('ul#raid_members').append(new_member);

				// Reset the event handlers
				reset_handlers();
				
				// Once one is found, we can safely end
				break;
			}
		}
	});
	
});

/**
 * Reset the click handlers for the various actions
 * 
 * This works better as a separate function, because these
 * handlers have to be re-established every time an 
 * element is added to the page.  Jquery 1.3.2 has
 * functionality to automatically keep these handlers
 * established, but lo!  1.3.2 appears to have issues with 
 * XSL parsed pages.
 * @return
 */
function reset_handlers()
{
	// Reset the event handlers for the close buttons
	$('div.close_button').unbind('click');
	$('div.close_button').click( function() {
		$(this).parents('li.raider').remove(); 
	});
	
	/* Allow folded fieldsets to toggle their folded-ness on click of the legend */
	$('fieldset.foldable legend').unbind('click');
	$('fieldset.foldable legend').click( function() {
		$(this).parent().toggleClass('folded');
	});

	
	
}