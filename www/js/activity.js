
var Activity = {};

Activity.Indicator = function(element, backgroundImage) {
    this.element = element;
    this.inProgress = 0;
    this.timeout = null;
    this.backgroundImage = backgroundImage;

    this.timer_cb = function() {
	if (this.backgroundImage != null)
	    this.element.style.backgroundImage = this.backgroundImage;
	else
	    this.element.style.display = "block";
	this.timeout = null;
    };

    this.increment = function() {
	if (this.inProgress == 0) {
	    var outerThis = this;

	    this.timeout = setTimeout(function() { outerThis.timer_cb(); },
				      250);
	}
	this.inProgress += 1;
    };

    this.decrement = function() {
	if (this.timeout != null) {
	    clearTimeout(this.timeout);
	    this.timeout = null;
	}
	this.inProgress -= 1;
	if (this.inProgress == 0) {
	    if (this.backgroundImage != null)
		this.element.style.backgroundImage = "";
	    else
		this.element.style.display = "none";
	}
    };
};
