jQuery.noConflict();
jQuery(document).ready(function($) {
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
    anchor.next('.toggle-content').find('.charts').each(function() {
      $(this).children('span').each(function() {
        img_class = $(this).attr('class');
        img_alt = $(this).attr('title');
        img_src = $(this).html().replace(/&amp;/g, "&");
        var img = new Image();
        $(img).attr('class', img_class);
        $(img).attr('src', img_src);
        $(img).attr('alt', img_alt);
        $(this).replaceWith(img);
        $(this).load();
      });
    })
    setTimeout('window.scrollTo(0, anchor.position().top', 500);
  }
  var anchor_check = document.location.href.split('#');
  if (anchor_check.length > 1) {
  	var anchor = anchor_check[anchor_check.length - 1];
  }
  var pcol = document.location.protocol;
  if (pcol != 'file:') {
    var whtt = document.createElement("script");
    whtt.src = pcol + "//static.wowhead.com/widgets/power.js";
    $('body').append(whtt);
  }
  $('a[ rel="_blank"]').each(function() {
    $(this).attr('target', '_blank');
  });
  $('.toggle-content, .help-box').hide();
  $('.open').next('.toggle-content').show();
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
    $(this).next('.toggle-content').find('.charts').each(function() {
      $(this).children('span').each(function() {
        img_class = $(this).attr('class');
        img_alt = $(this).attr('title');
        img_src = $(this).html().replace(/&amp;/g, "&");
        var img = new Image();
        $(img).attr('class', img_class);
        $(img).attr('src', img_src);
        $(img).attr('alt', img_alt);
        $(this).replaceWith(img);
        $(this).load();
      });
    });
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
    $('#active-help .help-box').fadeIn(150);
    var t = e.pageY - 20;
    var l = e.pageX - 20;
    $('#active-help').css({top:t,left:l});
    $('#active-help').fadeIn(250);
  });
  $('#active-help a.close').click(function(e) {
    e.preventDefault();
    $('#active-help').fadeOut(250);
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
