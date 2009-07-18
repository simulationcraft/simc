
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
	
	// Form submit buttons should all remove the form target, except simulate which submits to a popup
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
	
	// Set up all the error box close button
	$('div#error_report div.close_button').click( function() { $(this).parent().hide(); });
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
	$('ul#raid_members li.raider div.close_button').unbind('click').click( function() {
		$(this).parent('li.raider').remove(); 
	});
	
	
	// Option Menus
	$('ul.menu li').unbind('click').click( function() {
		
		// Pull this menu element id
		var this_id = $(this).attr('id').substring('menu_item_'.length);

		// Show any submenus associated with this menu
		$('ul.menu.child_menu', $(this).parents('div.content_list')).hide();
		$('ul.menu.child_of_'+this_id, $(this).parents('div.content_list')).show();

		// Make the button (and any parents) look selected
		$('ul.menu li.selected', $(this).parents('div.content_list')).removeClass('selected');
		var target_element = $(this);
		while( target_element ) {
			
			// Make this element selected and show the menu list it's in
			target_element.addClass('selected').parent().show();
			
			// If this element's menu is a sub-menu of another menu element, make the appropriate parent selected as well
			if( target_element.parent().hasClass('child_menu') ) {
				var this_parent_class = target_element.parent().attr('class').split(' ')[2].substring('child_of_'.length);
				target_element = $('ul.menu li#menu_item_'+this_parent_class, target_element.parents('div.content_list'));
				if(target_element.length == 0) {
					target_element = false;
				}
			}
			else {
				target_element = false;
			}
		}
		
		// If this menu element doesn't have a pane associated with it, but it has children, select the first one
		if( $('div#menu_pane_'+this_id, $(this).parents('div.content_list')).length == 0 ) {
			$('ul.menu.child_of_'+this_id+':first li:first-child', $(this).parents('div.content_list')).click();
		}
		
		// Show the pane associated with this menu item
		else {
			$('div.option_pane', $(this).parents('div.content_list')).hide();
			$('div#menu_pane_'+this_id, $(this).parents('div.content_list')).show();
		}
	});
	

	// define the 'clone' operation
	$('ul#raid_members li.raider div.clone_button').unbind('click').click( function() {
		
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