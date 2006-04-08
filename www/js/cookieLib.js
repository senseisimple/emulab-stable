/*
The contents of this file including all code, commments, etc. within is
Copyright Jim Auldridge 2005.

DISCLAIMER: THIS CODE SUPPLIED 'AS IS', WITH NO WARRANTY EXPRESSED OR IMPLIED.
YOU USE AT YOUR OWN RISK.  JIM AULDRIDGE DOES NOT ACCEPT ANY LIABILITY FOR
ANY LOSS OR DAMAGE RESULTING FROM USE, WHETHER CAUSED BY CODING MISTAKES (MINE
OR YOURS), 'HACKERS' FINDING HOLES, OR ANY OTHER MEANS.
*/
var cookieLib = {
	getCookie: function(cookieName) {
		var cookieNameStart,valueStart,valueEnd,value;
		cookieNameStart = document.cookie.indexOf(cookieName+'=');
		if (cookieNameStart < 0) {return null;}
		valueStart = document.cookie.indexOf(cookieName+'=') + cookieName.length + 1;
		valueEnd = document.cookie.indexOf(";",valueStart);
		if (valueEnd == -1){valueEnd = document.cookie.length;}
		value = document.cookie.substring(valueStart,valueEnd );
		value = JSON.parse(unescape(value));
		if (value == "") {return null;}
		return value;
	},
	setCookie: function(cookieName,value,hoursToLive,path,domain,secure) {
		var expireString,timerObj,expireAt,pathString,domainString,secureString,setCookieString;
		if (!hoursToLive || parseInt(hoursToLive)=='NaN'){expireString = "";}
		else {
			timerObj = new Date();
			timerObj.setTime(timerObj.getTime()+(parseInt(hoursToLive)*60*60*1000));
			expireAt = timerObj.toGMTString();
			expireString = "; expire="+expireAt;
		}
		pathString = "; path=";
		(!path || path=="") ? pathString += "/" : pathString += path;
		domainString = "; domain=";
		(!domain || domain=="") ? domainString += window.location.hostname : domainString += domain;
		(secure === true) ? secureString = "; secure" : secureString = "";
		value = escape(JSON.stringify(value));
		setCookieString = cookieName+"="+value+expireString+pathString+domainString;
		document.cookie = setCookieString;
	},
	delCookie: function(cookieName,path,domain){
		cookieLib.setCookie(cookieName,"",-8760);
	}
};
