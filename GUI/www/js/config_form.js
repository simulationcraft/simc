
/**
 * Initalize the insert counter, to keep the html form submission field names straight
 */
var last_insert_number = 1;

/**
 * When the page is sufficiently loaded, run this setup functionality 
 */
$( function() {
	
	// Set the last insert number to start after the existing raider records
	last_insert_number = $('ul#raid_members li.raider').size();
	
	// Form submit buttons should all remove the form target, except simulate
	$("form#config_form input[type='submit']").click( function() {
		$("form#config_form").removeAttr('target');
		return true;
	});
	$("form#config_form input#simulate").click( function() {
		$("form#config_form").attr('target', '_blank');
		return true;
	});
	
	
	// Clicking a member of the new-member list should insert a new raider of that class.  Set the event handler.
	$('ul#new_member_by_class li.supported_class').click( function() {
		
		// Build an array of the css classes this element has
		var classes = $(this).attr('class').split(' ');
		
		// Loop over the array of css classes that this clicked element had
		for (var i=0; i<classes.length; i++) {
						
			// If this class name corresponds to one of the indices in arr_prototype_class, then insert this element
			if( $('div#prototype_character_classes div#hidden_div_'+classes[i]+' ul').size() ) {
				
				// Add a new raider of the given class
				add_new_raider(classes[i]);
				
				// Once one is found, we can safely end
				break;
			}
		}
	});
	
	// Reset the event handlers
	reset_list_elements();
});


/**
 * Add a new raid member, with the given class name
 * @param class_name
 * @return
 */
function add_new_raider(class_name, arr_values)
{
	// Create the string of html for the new element
	var str_new_member = $('div#hidden_div_'+class_name+' ul').html(); 
	if( !str_new_member ) {
		alert('error adding new member.');
	}
	
	// Replace the counter placeholder string in the text with the real index
	last_insert_number++;
	str_new_member = str_new_member.replace(/{INDEX_VALUE}/g, last_insert_number);
	
	// Append the new list element to the list of raiding players
	$('ul#raid_members').append(str_new_member);

	// Reset the event handlers
	reset_list_elements();

	// If any values were passed, set the new element's field values
	if(typeof arr_values == 'object') {
		for(index in arr_values) {
			$('ul#raid_members li.raider:last input.field_'+index).val(arr_values[index]);
		}
	}

}

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
function reset_list_elements()
{
	// Apply some rounded corner fluff
	$('ul#raid_members li.raider div.member_class').corner('br 10px');
	
	
	// Reset the event handlers for the close buttons
	$('ul#raid_members li.raider div.close_button').unbind('click');
	$('ul#raid_members li.raider div.close_button').click( function() {
		$(this).parent('li.raider').remove(); 
	});
	
	
	/* Allow folded fieldsets to toggle their folded-ness on click of the legend */
	$('fieldset.foldable legend').unbind('click');
	$('fieldset.foldable legend').click( function() {
		$(this).parent().toggleClass('folded');
	});


	// NOTE: this button is commented out in the XSL - this function doesn't work well yet (field value copy doesnt work)
	// define the 'clone' operation
	$('ul#raid_members li.raider div.clone_button').unbind('click');
	$('ul#raid_members li.raider div.clone_button').click( function() {
		
		// Determine the class of this element
		var class_name = $('input.field_class', $(this).parent()).val();
		
		// Add a new copy of the same class
		add_new_raider(class_name);

		// Copy all the field values from the cloned raider to the new raider
		$("input[type!='hidden']", $(this).parent()).each( function() {
			$("input[type!='checkbox']."+this.className, $('ul#raid_members li.raider:last')).val( $(this).val() );
			if(this.checked) {
				$("input[type='checkbox']."+this.className, $('ul#raid_members li.raider:last')).attr('checked', true);
			}
		});
		
		// append to the name the last_insert_number, to make sure the characters are kept distinct
		$("input.field_name", $('ul#raid_members li.raider:last')).val($("input.field_name", $('ul#raid_members li.raider:last')).val()+'_'+last_insert_number);
	});
	
}