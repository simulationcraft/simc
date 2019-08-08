<script type="text/javascript">
    function load_images(containers) {
        var $j = jQuery.noConflict();
        containers.each(function () {
            $j(this).children('span').each(function () {
                var j = jQuery.noConflict();
                img_class = $j(this).attr('class');
                img_alt = $j(this).attr('title');
                img_src = $j(this).html().replace(/&amp;/g, '&');
                var img = new Image();
                $j(img).attr('class', img_class);
                $j(img).attr('src', img_src);
                $j(img).attr('alt', img_alt);
                $j(this).replaceWith(img);
                $j(this).load();
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
        setTimeout("var ypos=0;var e=document.getElementById('" + section.attr('id') + "');while( e != null ) {ypos += e.offsetTop;e = e.offsetParent;}window.scrollTo(0,ypos);", 500);
    }
    jQuery.noConflict();
    jQuery(document).ready(function ($) {
        var chart_containers = false;
        var anchor_check = document.location.href.split('#');
        if (anchor_check.length > 1) {
            var anchor = anchor_check[anchor_check.length - 1];
        }
        $('a.ext').mouseover(function () {
            $(this).attr('target', '_blank');
        });
        $('.toggle').click(function (e) {
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
            $(this).prev('.toggle-thumbnail').toggleClass('hide');
            chart_containers = $(this).next('.toggle-content').find('.charts');
            load_images(chart_containers);
        });
        $('.toggle-details').click(function (e) {
            e.preventDefault();
            $(this).toggleClass('open');
            $(this).parents('tr').nextAll('.details').first().toggleClass('hide');
        });
        $('.toggle-db-details').click(function (e) {
            e.preventDefault();
            $(this).toggleClass('open');
            $(this).parent().next('.toggle-content').toggle(150);
        });
        var hoverTimeout;
        $('.help').hover(function (e) {
            var target = $(this).data('help') + ' .help-box';
            var content = $(target).html();
            $('#active-help-dynamic .help-box').html(content);
            $('#active-help .help-box').show();
            var pos = $(this).offset();
            $('#active-help').css({
                top: pos.top - $('#active-help').height() - $(this).height() - 4,
                left: pos.left,
            });
            hoverTimeOut = setTimeout(function() {
                $('#active-help').fadeIn(500);
            }, 1500);
        }, function () {
            clearTimeout(hoverTimeout);
            $('#active-help').stop();
            $('#active-help').hide();
        });
        window.onblur = function () {
            clearTimeout(hoverTimeout);
            $('#active-help').stop();
            $('#active-help').hide();
        };
        if (anchor) {
            anchor = '#' + anchor;
            target = $(anchor).children('h2:first');
            open_anchor(target);
        }
        $('ul.toc li a').click(function (e) {
            anchor = $(this).attr('href');
            target = $(anchor).children('h2:first');
            open_anchor(target);
        });
        function oddstripe() {
            var rows = $(this).find('.toprow');
            rows.filter(':even').removeClass('odd');
            rows.filter(':odd').addClass('odd');
        }
        $('.stripetoprow').each(oddstripe);
        function getCell(elem, i, both) {
            var $row;
            if ($(elem).is('tr')) {
                $row = $(elem);
            } else {
                $row = $(elem).find('tr:first');
            }
            return $row.children('td').eq(i).text().trim();
        }
        function numberSort(a, b, i, dsc, both) {
            var va = parseFloat(getCell(a, i, both)) || 0;
            var vb = parseFloat(getCell(b, i, both)) || 0;
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
            var $col = $(this).closest('th');
            var $tbl = $col.closest('table.sort');
            var $sib = $col.siblings('th').find('.asc-sorted, .dsc-sorted');
            $sib.add($col.siblings('.asc-sorted, .dsc-sorted'));
            $sib.removeClass('asc-sorted dsc-sorted');
            var idx = $col.index();
            var doAlpha = $(this).data('sorttype') == 'alpha';
            var doAsc = $(this).data('sortdir') == 'asc';
            var doRows = $(this).data('sortrows') == 'both';
            if (!($(this).hasClass('dsc-sorted') || $(this).hasClass('asc-sorted'))) {
                $(this).toggleClass('dsc-sorted', !doAsc);
            } else {
                $(this).toggleClass('dsc-sorted');
            }
            var isDsc = $(this).hasClass('dsc-sorted');
            $(this).toggleClass('asc-sorted', !isDsc);
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
            var $bucket = $tbl.children('tbody:not(.nosort)');
            var $remain = $tbl.children('tbody.nosort');
            if ($bucket.length == 1) {
                $bucket = $bucket.children('tr');
            }
            if ($bucket.length) {
                $bucket.sort(srt(idx, isDsc, doRows));
                $tbl.append($bucket);
                $tbl.append($remain);
                if ($tbl.hasClass('stripetoprow')) {
                    oddstripe.call($tbl);
                }
            }
        });
    });
</script>