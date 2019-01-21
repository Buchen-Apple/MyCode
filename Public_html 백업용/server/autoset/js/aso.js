/* ASO 스크립트 */

var gColWidth1 = 20;
var gColWidth2 = 80;
var gLang = "ko";
var gLangRes = {
	apply: {en:"Apply", ko:"변경사항 적용"},
	confirm: {en:"Confirm", ko:"확인"},
	list: {en:"List", ko:"관리 목록"},
	delete_conf: {en:"Delete", ko:"설정 삭제"},
	help: {en:"Help", ko:"도움말"}
};

function setLang(code)
{
	gLang = code;
}

function doSubmit()
{
	document.forms['asofrm'].asoVAR_cmdSubmit.value = "DoEvent";
}

function setColumn(col1,col2)
{
	gColWidth1 = col1;
	gColWidth2 = col2;
}

function header(title)
{
	title = title.replace(" > ","&nbsp;&gt;&nbsp;");

	document.write('<form name="asofrm" style="margin:0;">');
	document.write('<input type="hidden" name="asoVAR_cmdOpenDir" value="nothing">');
	document.write('<input type="hidden" name="asoVAR_cmdOpenDir2" value="nothing">');
	document.write('<input type="hidden" name="asoVAR_cmdOpenDir3" value="nothing">');
	document.write('<input type="hidden" name="asoVAR_cmdOpenDir4" value="nothing">');
	document.write('<input type="hidden" name="asoVAR_cmdOpenDir5" value="nothing">');
	document.write('<input type="hidden" name="asoVAR_cmdOpenDir6" value="nothing">');

	document.write('<input type="hidden" name="asoVAR_cmdOpenFile" value="nothing">');
	document.write('<input type="hidden" name="asoVAR_cmdSubmit" value="nothing">');

	document.write('<table border="0" cellpadding="0" cellspacing="0" width="100%">');
	document.write('<tr>');
	document.write('<td style="width:6px;height:30px;background:url(../../img/bg_top_left.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('<td style="height:30px;background:url(../../img/bg_top_center.gif) repeat-x;font-size:9pt;font-family:tahoma,dotum;color:#555555;padding-left:5px;">');
	document.write('<span style="height:1;filter:dropshadow(color:#ffffff,offx:1,offy:1,positive:1);">' + title + '</span>');
	document.write('</td>');
	document.write('<td style="width:10px;height:30px;background:url(../../img/bg_top_right.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('</tr>');
	document.write('<tr>');
	document.write('<td style="width:6px;background:url(../../img/bg_middle_left.gif) repeat-y;font-size:1pt;">&nbsp;</td>');
	document.write('<td style="background:#ffffff;font-size:9pt;font-family:tahoma,dotum;line-height:150%;">');
	document.write('<div style="width:100%;padding:10px;">');
	document.write('<table border="0" cellpadding="5" cellspacing="0" width="100%">');
	document.write('<col width="' + gColWidth1 + '%">');
	document.write('<col width="' + gColWidth2 + '%">');
}

function footer(button)
{
	if (button == undefined)
		var button = '';

	document.write('</table>');
	document.write('</div>');

	document.write('<input type="hidden" name="endposition" value="END">');
	document.write('<input type="hidden" name="_eof" value="">');

	if (button == 'apply')
	{
		document.write('<div style="width:100%;text-align:right;padding:5px;">');
		document.write('<a href="javascript:doSubmit();"><img src="../../img/btn_apply' + getSuffix() + '.gif" border="0" alt="' + getString('apply') + '"></a>');
		document.write('</div>');
	}
	else if (button == 'confirm')
	{
		document.write('<div style="width:100%;text-align:right;padding:5px;">');
		document.write('<a href="javascript:doSubmit();"><img src="../../img/btn_confirm' + getSuffix() + '.gif" border="0" alt="' + getString('confirm') + '"></a>');
		document.write('</div>');
	}
	else if (button == 'vhost')
	{
		document.write('<div style="float:left;padding:5px;">');
		document.write('<a href="javascript:vhost_list();"><img src="../../img/btn_list' + getSuffix() + '.gif" border="0" alt="' + getString('list') + '"></a>&nbsp;');
		document.write('<a href="javascript:delete_form(document.forms.asofrm);"><img src="../../img/btn_delete_conf' + getSuffix() + '.gif" border="0" alt="' + getString('delete_conf') + '"></a>');
		document.write('</div>');

		document.write('<div style="float:right;padding:5px;">');
		document.write('<a href="javascript:doSubmit();"><img src="../../img/btn_apply' + getSuffix() + '.gif" border="0" alt="' + getString('apply') + '"></a>');
		document.write('</div>');
	}

	document.write('</td>');
	document.write('<td style="width:10px;background:url(../../img/bg_middle_right.gif) repeat-y;font-size:1pt;">&nbsp;</td>');
	document.write('</tr>');
	document.write('<tr>');
	document.write('<td style="width:6px;height:8px;background:url(../../img/bg_bottom_left.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('<td style="height:8px;background:url(../../img/bg_bottom_center.gif) repeat-x;font-size:1;">&nbsp;</td>');
	document.write('<td style="width:10px;height:8px;background:url(../../img/bg_bottom_right.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('</tr>');
	document.write('</table>');

	document.write('</form>');
}

function createForm(name, value)
{
	if (value == undefined)
		var value = "";
	
	document.write('<input type="hidden" name="' + name + '" value="' + value + '">');
}

function createItem(name, value)
{
	document.write('<tr>');

	if (value == undefined)
	{
		document.write('<td colspan="2" style="border-bottom:1px solid #d4d4d4;">' + name + '</td>');
	}
	else
	{
		document.write('<td style="border-bottom:1px solid #d4d4d4;">' + name + '</td>');
		document.write('<td style="border-bottom:1px solid #d4d4d4;">' + value + '</td>');
	}
	document.write('</tr>');
}


function createDesc(value, title)
{
	document.write('<table border="0" cellpadding="0" cellspacing="0" width="100%" style="margin-top:5px;">');
	document.write('<tr>');
	document.write('<td style="width:6px;height:30px;background:url(../../img/bg_top_left.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('<td style="height:30px;background:url(../../img/bg_top_center.gif) repeat-x;font-size:9pt;font-family:tahoma,dotum;color:#555555;padding-left:5px;">');
	document.write('<span style="height:1;filter:dropshadow(color:#ffffff,offx:1,offy:1,positive:1);">' + (title || getString('help')) + '</span>');
	document.write('</td>');
	document.write('<td style="width:10px;height:30px;background:url(../../img/bg_top_right.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('</tr>');
	document.write('<tr>');
	document.write('<td style="width:6px;background:url(../../img/bg_middle_left.gif) repeat-y;font-size:1pt;">&nbsp;</td>');
	document.write('<td style="background:#ffffff;font-size:9pt;font-family:tahoma,dotum;line-height:150%;">');
	document.write('<div style="width:100%;padding:10px;">' + value + '</div>');
	document.write('</td>');
	document.write('<td style="width:10px;background:url(../../img/bg_middle_right.gif) repeat-y;font-size:1pt;">&nbsp;</td>');
	document.write('</tr>');
	document.write('<tr>');
	document.write('<td style="width:6px;height:8px;background:url(../../img/bg_bottom_left.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('<td style="height:8px;background:url(../../img/bg_bottom_center.gif) repeat-x;font-size:1;">&nbsp;</td>');
	document.write('<td style="width:10px;height:8px;background:url(../../img/bg_bottom_right.gif) no-repeat;font-size:1pt;">&nbsp;</td>');
	document.write('</tr>');
	document.write('</table>');

}

function getSuffix()
{
	return gLang == "ko" ? "" : "_en";
}

function getString(code)
{
	return gLangRes[code][gLang];
}
