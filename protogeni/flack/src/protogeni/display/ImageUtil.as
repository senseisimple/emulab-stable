/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
 
 /* Common.as
 
    Functions used all around the project
*/
 
 package protogeni.display
{
	public class ImageUtil
	{
		[Bindable]
		[Embed(source="../../../images/flack.png")]
		public static var logoIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/chart_bar.png")]
		public static var statisticsIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/document_signature.png")]
		public static var credentialIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/information.png")]
		public static var infoIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/help.png")]
		public static var helpIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_left.png")]
		public static var leftIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_right.png")]
		public static var rightIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_up.png")]
		public static var upIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_down.png")]
		public static var downIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/status_online.png")]
		public static var userIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/status_offline.png")]
		public static var noUserIcon:Class;
		
		[Bindable]
        [Embed(source="../../../images/tick.png")]
        public static var availableIcon:Class;

        [Bindable]
        [Embed(source="../../../images/cross.png")]
        public static var crossIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/administrator.png")]
        public static var ownedIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/server.png")]
		public static var exclusiveIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/server_stanchion.png")]
		public static var sharedIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/drive_network.png")]
		public static var physicalNodeIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/link.png")]
        public static var linkIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_green.png")]
        public static var flagGreenIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_red.png")]
        public static var flagRedIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_yellow.png")]
        public static var flagYellowIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_orange.png")]
        public static var flagOrangeIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/error.png")]
        public static var errorIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/exclamation.png")]
        public static var exclamationIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/lightbulb.png")]
		public static var onIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/lightbulb_off.png")]
		public static var offIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/add.png")]
		public static var addIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/cancel.png")]
		public static var cancelIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/stop.png")]
		public static var stopIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_stop_blue.png")]
		public static var stopControlIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_pause_blue.png")]
		public static var pauseControlIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_play_blue.png")]
		public static var playControlIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_repeat_blue.png")]
		public static var repeatControlIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/delete.png")]
		public static var deleteIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_refresh.png")]
		public static var refreshIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/map.png")]
		public static var mapIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/graph.png")]
		public static var graphIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page.png")]
		public static var pageIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_code.png")]
		public static var pageCodeIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_white.png")]
		public static var pageWhiteIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_white_code.png")]
		public static var pageWhiteCodeIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/wand.png")]
		public static var actionIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/ssl_certificates.png")]
		public static var sslIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/keyboard.png")]
		public static var keyboardIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/system_monitor.png")]
		public static var consoleIcon:Class;
		
		// Entities
		
		[Bindable]
		[Embed(source="../../../images/entity.png")]
		public static var authorityIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/building.png")]
		public static var managerIcon:Class;
		
		// Operations
		
		[Bindable]
		[Embed(source="../../../images/find.png")]
		public static var findIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/magnifier.png")]
		public static var searchIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/disk.png")]
		public static var saveIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/folder.png")]
		public static var openIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/paste_plain.png")]
		public static var pasteIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_white_copy.png")]
		public static var copyIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/key.png")]
		public static var passwordIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/shield.png")]
		public static var authenticationIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/draw_eraser.png")]
		public static var eraseIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/email.png")]
		public static var emailIcon:Class;
	}
}