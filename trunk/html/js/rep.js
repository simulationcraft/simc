jQuery.noConflict();
jQuery(document).ready(function($) {
	function load_images(containers) {
		containers.each(function() {
			$(this).children('span').each(function() {
				img_class = $(this).attr('class');
				img_alt = $(this).attr('title');
				img_src = $(this).html().replace(/&amp;/g, '&');
				var img = new Image();
				$(img).attr('class', img_class);
				$(img).attr('src', img_src);
				$(img).attr('alt', img_alt);
				$(this).replaceWith(img);
				$(this).load();
			});
		});
	}
	function open_anchor(anchor) {
		var img_id = '';
		var src = '';
		var target = '';
		anchor.addClass('open');
		var section = anchor.parent('.section');
		section.addClass('section-open');
		section.removeClass('grouped-first');
		section.removeClass('grouped-last');
		if (!(section.next().hasClass('section-open'))) {
			section.next().addClass('grouped-first');
		}
		if (!(section.prev().hasClass('section-open'))) {
			section.prev().addClass('grouped-last');
		}
		anchor.next('.toggle-content').show(150);
		chart_containers = anchor.next('.toggle-content').find('.charts');
		load_images(chart_containers);
		setTimeout('window.scrollTo(0, anchor.position().top', 500);
	}
	var chart_containers = false;
	var anchor_check = document.location.href.split('#');
	if (anchor_check.length > 1) {
		var anchor = anchor_check[anchor_check.length - 1];
	}
	$('a.ext').mouseover(function() {
		$(this).attr('target', '_blank');
	});
	$('.toggle').click(function(e) {
		var img_id = '';
		var src = '';
		var target = '';
		e.preventDefault();
		$(this).toggleClass('open');
		var section = $(this).parent('.section');
		if (section.attr('id') != 'masthead') {
			section.toggleClass('section-open');
		}
		if (section.attr('id') != 'masthead' && section.hasClass('section-open')) {
			section.removeClass('grouped-first');
			section.removeClass('grouped-last');
			if (!(section.next().hasClass('section-open'))) {
				section.next().addClass('grouped-first');
			}
			if (!(section.prev().hasClass('section-open'))) {
				section.prev().addClass('grouped-last');
			}
		} else if (section.attr('id') != 'masthead') {
			if (section.hasClass('final') || section.next().hasClass('section-open')) {
				section.addClass('grouped-last');
			} else {
				section.next().removeClass('grouped-first');
			}
			if (section.prev().hasClass('section-open')) {
				section.addClass('grouped-first');
			} else {
				section.prev().removeClass('grouped-last');
			}
		}
		$(this).next('.toggle-content').toggle(150);
		chart_containers = $(this).next('.toggle-content').find('.charts');
		load_images(chart_containers);
	});
	$('.toggle-details').click(function(e) {
		e.preventDefault();
		$(this).toggleClass('open');
		$(this).parents().next('.details').toggleClass('hide');
	});
	$('.toggle-db-details').click(function(e) {
		e.preventDefault();
		$(this).toggleClass('open');
		$(this).parent().next('.toggle-content').toggle(150);
	});
	$('.help').click(function(e) {
		e.preventDefault();
		var target = $(this).attr('href') + ' .help-box';
		var content = $(target).html();
		$('#active-help-dynamic .help-box').html(content);
		$('#active-help .help-box').show();
		var t = e.pageY - 20;
		var l = e.pageX - 20;
		$('#active-help').css({top:t,left:l});
		$('#active-help').show(250);
	});
	$('#active-help a.close').click(function(e) {
		e.preventDefault();
		$('#active-help').toggle(250);
	});
	if (anchor) {
		anchor = '#' + anchor;
		target = $(anchor).children('h2:first');
		open_anchor(target);
	}
	$('ul.toc li a').click(function(e) {
		anchor = $(this).attr('href');
		target = $(anchor).children('h2:first');
		open_anchor(target);
	});
});
