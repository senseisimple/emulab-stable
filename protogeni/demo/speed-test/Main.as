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

package
{
  import flash.display.MovieClip;
  import flash.display.Shape;
  import flash.text.TextField;
  import flash.events.Event;
  import flash.events.ProgressEvent;
  import flash.events.HTTPStatusEvent;
  import flash.events.IOErrorEvent;
  import flash.events.SecurityErrorEvent;
  import flash.geom.ColorTransform;
  import flash.net.URLRequest;
  import flash.net.URLLoader;
  import flash.net.URLRequestHeader;
  import flash.utils.getTimer;

  public class Main
  {
    public function Main(parent : MovieClip, newBack : BackgroundClip,
                         newStatusText : TextField,
                         newThroughput : TextField) : void
    {
      parent.addEventListener(Event.ENTER_FRAME, enterFrame);
      back = newBack;
      statusText = newStatusText;
      throughput = newThroughput;

      var myUrl = parent.stage.loaderInfo.loaderURL;
      var urlIndex = myUrl.lastIndexOf("/");
      url = myUrl.substring(0, urlIndex) + "/download.jpg";

      sizeX = parent.stage.stageWidth;
      sizeY = parent.stage.stageHeight;
      loader = null;
      isLoading = false;
      deadline = getTimer() + Math.floor(Math.random()*WAIT_TIME);
      startTime = 0;
    }

    function enterFrame(event : Event) : void
    {
      if (! isLoading && getTimer() > deadline)
      {
        loadUrl();
      }
    }

    function draw(color : ColorTransform) : void
    {
      back.back.transform.colorTransform = color;
    }

    function loadUrl() : void
    {
      startTime = getTimer();
      statusText.text = "Downloading";
      var request = new URLRequest(url + "?foo=" + String(Math.random()));
      var headers : Array
        = new Array(new URLRequestHeader("Cache-Control", "no-store"),
                    new URLRequestHeader("Pragma", "no-cache"));
      request.requestHeaders = headers;
      loader = new URLLoader(request);
      loader.addEventListener(Event.COMPLETE, complete);
      loader.addEventListener(ProgressEvent.PROGRESS, progress);
      loader.addEventListener(HTTPStatusEvent.HTTP_STATUS, statusFail);
      loader.addEventListener(IOErrorEvent.IO_ERROR, ioFail);
      loader.addEventListener(SecurityErrorEvent.SECURITY_ERROR,
                              securityFail);
      isLoading = true;
    }

    function cleanupUrl() : void
    {
      loader.removeEventListener(Event.COMPLETE, complete);
      loader.removeEventListener(ProgressEvent.PROGRESS, progress);
      loader.removeEventListener(HTTPStatusEvent.HTTP_STATUS, statusFail);
      loader.removeEventListener(IOErrorEvent.IO_ERROR, ioFail);
      loader.removeEventListener(SecurityErrorEvent.SECURITY_ERROR,
                                 securityFail);
      loader = null;
      isLoading = false;
      deadline = getTimer() + WAIT_TIME;
    }

    function complete(event : Event) : void
    {
      draw(GREEN);
      statusText.text = "Download Succeeded";
      var diff = (getTimer() - startTime)/1000;
      var bits = loader.bytesLoaded * 8;
      var bps = 0;
      if (diff > 0)
      {
        bps = bits/diff;
      }
      if (bps < 1000)
      {
        throughput.text = String(int(bps)) + " bps";
      }
      else
      {
        var kbps = bps / 1000;
        if (kbps < 1000)
        {
          throughput.text = String(int(kbps)) + " Kbps";
        }
        else
        {
          var mbps = kbps / 1000;
          throughput.text = String(int(mbps)) + " Mbps";
        }
      }
      cleanupUrl();
    }

    function progress(event : ProgressEvent) : void
    {
      draw(GREEN);
    }

    function statusFail(event : HTTPStatusEvent) : void
    {
    }

    function ioFail(event : IOErrorEvent) : void
    {
      draw(RED);
      statusText.text = "Download Failed";
      throughput.text = "";
      cleanupUrl();
    }

    function securityFail(event : SecurityErrorEvent) : void
    {
      draw(RED);
      statusText.text = "Security Failure";
      throughput.text = "";
      cleanupUrl();
    }

    var back : BackgroundClip;
    var statusText : TextField;
    var throughput : TextField;
    var url : String;
    var sizeX : int;
    var sizeY : int;
    var loader : URLLoader;
    var isLoading : Boolean;
    var deadline : int;
    var startTime : int;

    static var WAIT_TIME = 10000;

    static var RED : ColorTransform = new ColorTransform(1.5, 0.5, 0.5, 1.0,
                                                         0, 0, 0, 0);
    static var GREEN : ColorTransform = new ColorTransform(0.5, 1.5, 0.5, 1.0,
                                                           0, 0, 0, 0);
    static var YELLOW : ColorTransform = new ColorTransform(1.5, 1.5, 0.5, 1.0,
                                                            0, 0, 0, 0);
  }
}
