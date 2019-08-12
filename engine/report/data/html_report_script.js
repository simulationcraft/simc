(function($) {
    $.fn.validate_section = function() {
        if (this[0].id == 'masthead') {
            return this;
        }
        var $prev = this.prev('.section');
        var $next = this.next('.section');
        if (this.children('.toggle.open').length) {
            this.removeClass('grouped-first grouped-last');
            this.addClass('section-open');
            if ($prev.length && $prev[0].id != 'masthead' && !$prev.children('.toggle.open').length) {
                $prev.addClass('grouped-last');
            }
            if ($next.length && !$next.children('.toggle.open').length) {
                $next.addClass('grouped-first');
            }
        } else {
            this.removeClass('section-open grouped-first grouped-last');
            if ($prev.hasClass('section-open')) {
                this.addClass('grouped-first');
            } else {
                $prev.removeClass('grouped-last');
            }
            if ($next.hasClass('section-open') || !$next.length) {
                this.addClass('grouped-last');
            } else {
                $next.removeClass('grouped-first');
            }
        }
        return this;
    };
    $.fn.oddstripe = function() {
        var $rows = this.find('.toprow');
        $rows.filter(':even').removeClass('odd');
        $rows.filter(':odd').addClass('odd');
        return this;
    };
}(jQuery));
jQuery.noConflict();
jQuery(document).ready(function ($) {
    var anchor = document.location.href.split('#');
    if (anchor.length > 1) {
        anchor = '#' + anchor[anchor.length - 1];
        anchor = $(anchor).children('h2').first();
        if (!anchor.hasClass('open')) {
            $anchor.click();
        }
    }
    $('.stripetoprow').oddstripe();
    $('.toggle.open').parent('.section').validate_section();
    $('#masthead').next().validate_section();
    $('.section').last().validate_section();
    $('a.ext').mouseover(function () {
        $(this).attr('target', '_blank');
    });
    $('ul.toc li a').click(function (e) {
        e.preventDefault();
        var href = this.getAttribute('href');
        var $anchor = $(href).children('h2').first();
        if (!$anchor.hasClass('open')) {
            $anchor.click();
        }
        $('html, body').animate( {
            scrollTop: $(href).offset().top - $anchor.height()
        }, 300);
    });
    $('.toggle').click(function (e) {
        e.preventDefault();
        var $me = $(this);
        $me.toggleClass('open');
        $me.next('.toggle-content').slideToggle(150);
        $me.prev('.toggle-thumbnail').toggleClass('hide');
        var section = $me.parent('.section');
        if (section.length) {
            section.validate_section();
            if ($me.hasClass('open')) {
                var wh = $(window).height() / 3;
                var pos = section[0].getBoundingClientRect().top;
                if (pos > wh * 2) {
                    $('html, body').animate( {
                        scrollTop: section.offset().top - wh
                    }, 300);
                }
            }
        }
    });
    $('.toggle-details').click(function (e) {
        e.preventDefault();
        $(this).toggleClass('open');
        $(this).parents('tr').nextAll('.details').first().fadeToggle(150);
    });
	$('.toggle, .toggle-details').each(function() {
		if ( __chartData[this.id] === undefined ) return;
		$(this).one('click', function() {
			var d = __chartData[this.id];
			for (var idx in d) {
				$('#' + d[idx]['target']).highcharts(d[idx]['data']);
			}
		});
    });
    var hoverTimeout;
    function hoverHide() {
        clearTimeout(hoverTimeout);
        $('#active-help','.help-box').hide();
        $('#active-help').removeAttr('style');
    }
    $('.help').hover(function (e) {
        var $target = $(this.dataset.help + ' .help-box');
        var content;
        if (!$target.length) {
            content = '<h3>NO HELP FOUND</h3><p>PLEASE REPORT AS A BUG</p>';
        } else {
            content = $target.html();
        }
        $('#active-help-dynamic .help-box').html(content);
        $('#active-help .help-box').show();
        var pos = $(this).offset();
        $('#active-help').css({
            top: pos.top - $('#active-help').height() - $(this).height(),
            left: pos.left,
        });
        hoverTimeout = setTimeout(function() {
            $('#active-help').fadeIn(450);
        }, 750);
    }, hoverHide);
    window.onblur = hoverHide;
    function getCell(elem, i, both) {
        var $row;
        var ret;
        var idx = i;
        if ($(elem).is('tr')) {
            $row = $(elem);
        } else {
            $row = $(elem).find('tr').first();
        }
        ret = $row.children('td').eq(idx).text().trim();
        if (!ret.length && both) {
            var span = $row.children('td[rowspan]').length;
            $row = $row.nextAll(':not(.details)').first();
            if (idx > span) {
                idx -= span;
                ret = $row.children('td').eq(idx).text().trim();
            }
        }
        return ret;
    }
    function strip(val) {
        return val.slice(val.indexOf('\xa0') + 1).replace(/[^\d.-]/g,'');
    }
    function numberSort(a, b, i, dsc, both) {
        var va = +strip(getCell(a, i, both)) || 0;
        var vb = +strip(getCell(b, i, both)) || 0;
        return dsc ? vb - va : va - vb;
    }
    function alphaSort(a, b, i, dsc, both) {
        var va = getCell(a, i, both);
        var vb = getCell(b, i, both);
        if (dsc) {
            return va < vb ? 1 : -1;
        } else {
            return va > vb ? 1 : -1;
        }
    }
    $('.toggle-sort').click(function (e) {
        e.preventDefault();
        var $me = $(this);
        var $col = $me.closest('th');
        var $thd = $col.closest('thead');
        var $tbl = $thd.closest('table.sort');
        var $sib = $col.siblings('.asc-sorted, .dsc-sorted');
        $sib.removeClass('asc-sorted dsc-sorted');
        var idx = $col.index();
        var doAlpha = this.dataset.sorttype == 'alpha';
        var doAsc = this.dataset.sortdir == 'asc';
        var doRows = this.dataset.sortrows == 'both';
        if (!($me.hasClass('dsc-sorted') || $me.hasClass('asc-sorted'))) {
            $me.toggleClass('dsc-sorted', !doAsc);
        } else {
            $me.toggleClass('dsc-sorted');
        }
        var isDsc = $me.hasClass('dsc-sorted');
        $me.toggleClass('asc-sorted', !isDsc);
        this.offsetHeight;
        var srt;
        if (doAlpha) {
            srt = function(i, dsc, both) {
                return function(a, b) {
                    return alphaSort(a, b, i, dsc, both);
                };
            };
        } else {
            srt = function(i, dsc, both) {
                return function(a, b) {
                    return numberSort(a, b, i, dsc, both);
                };
            };
        }
        var $bucket = $thd.nextUntil('.petrow');
        if ($bucket.length == 1) {
            $bucket = $bucket.children('tr').first().nextUntil('.petrow').addBack();
        }
        var $remain = $bucket.last().nextAll();
        var $petrow;
        do {
            $bucket.sort(srt(idx, isDsc, doRows));
            $tbl.append($bucket);
            if (!$remain.length) {
                break;
            }
            $tbl.append($remain);
            $petrow = $remain.first();
            $bucket = $petrow.nextUntil('.petrow');
            $remain = $bucket.last().nextAll();
        } while ($bucket.length > 1);
        if ($tbl.hasClass('stripetoprow')) {
            $tbl.oddstripe();
        }
    });
});