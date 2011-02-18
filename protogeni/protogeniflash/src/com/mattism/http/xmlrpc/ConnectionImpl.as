/**
* @author       Matt Shaw <xmlrpc@mattism.com>
* @url          http://sf.net/projects/xmlrpcflash
*                       http://www.osflash.org/doku.php?id=xmlrpcflash
*
* @author   Daniel Mclaren (http://danielmclaren.net)
* @note     Updated to Actionscript 3.0
*/

package com.mattism.http.xmlrpc
{
  import com.mattism.http.xmlrpc.Connection;
  import com.mattism.http.xmlrpc.MethodCall;
  import com.mattism.http.xmlrpc.MethodCallImpl;
  import com.mattism.http.xmlrpc.MethodFault;
  import com.mattism.http.xmlrpc.MethodFaultImpl;
  import com.mattism.http.xmlrpc.Parser;
  import com.mattism.http.xmlrpc.ParserImpl;
  
  import flash.events.ErrorEvent;
  import flash.events.Event;
  import flash.events.EventDispatcher;
  import flash.events.IOErrorEvent;
  import flash.events.SecurityErrorEvent;
  import flash.events.TimerEvent;
  import flash.net.URLLoader;
  import flash.net.URLRequest;
  import flash.net.URLRequestMethod;
  import flash.utils.Timer;

  public class ConnectionImpl extends EventDispatcher implements Connection
  {
    // Metadata
    private var _VERSION : String = "1.0.0";
    private var _PRODUCT : String = "ConnectionImpl";

    private var ERROR_NO_URL : String =  "No URL was specified for XMLRPCall.";

    private var _url : String;
    public var _method : MethodCall;
    private var _rpc_response : Object;
    private var _parser : Parser;
    public var _response:URLLoader
	private var _parsed_response : Object;

    private var _fault : MethodFault;
	
	public var timeout:int = 60;

    public function ConnectionImpl(url : String)
    {
      //prepare method response handler
      //this.ignoreWhite = true;

      //init method
      this._method = new MethodCallImpl();

      //init parser
      this._parser = new ParserImpl();

      //init response
	  if(Main.useJavascript)
        this._response = new JSLoader();
      else
        this._response = new URLLoader();
			
      this._response.addEventListener(Event.COMPLETE, this._onLoad);
//      this._response.addEventListener(HTTPStatusEvent.HTTP_STATUS, httpStatus);
      this._response.addEventListener(IOErrorEvent.IO_ERROR, ioError);
//      this._response.addEventListener(Event.OPEN, open);
//      this._response.addEventListener(ProgressEvent.PROGRESS, progress);
      this._response.addEventListener(SecurityErrorEvent.SECURITY_ERROR,
                                      securityError);
      if (url)
      {
        this.setUrl( url );
      }
    }

    public function cleanup() : void
    {
      this._response.removeEventListener(Event.COMPLETE, this._onLoad);
//      this._response.removeEventListener(HTTPStatusEvent.HTTP_STATUS,
//                                         httpStatus);
      this._response.removeEventListener(IOErrorEvent.IO_ERROR, ioError);
//      this._response.removeEventListener(Event.OPEN, open);
//    this._response.removeEventListener(ProgressEvent.PROGRESS, progress);
      this._response.removeEventListener(SecurityErrorEvent.SECURITY_ERROR,
                                         securityError);
	  this._response.removeEventListener(IOErrorEvent.NETWORK_ERROR, networkError);
	  
                if(observeTimer != null)
                {
                        observeTimer.stop();
                        observeTimer = null;
                }
    }

    public function call(method : String) : void
    {
      this._call( method );
    }

    private function _call( method:String ):void
    {
      if ( !this.getUrl() )
      {
        trace(ERROR_NO_URL);
        throw Error(ERROR_NO_URL);
      }
      else
      {
        this.debug( "Call -> " + method+"() -> " + this.getUrl());

        this._method.setName( method );

        var request:URLRequest = new URLRequest();
        request.contentType = 'text/xml';
        request.data = this._method.getXml();
        request.method = URLRequestMethod.POST;
        request.url = this.getUrl();

		try {
			this._response.load(request);
		}
		catch (error:ArgumentError)
		{
			dispatchEvent(new ErrorEvent(ErrorEvent.ERROR, false, false, "Argument error on load"));
			trace("An ArgumentError has occurred.");
		}
		catch (error:SecurityError)
		{
			dispatchEvent(new SecurityErrorEvent(SecurityErrorEvent.SECURITY_ERROR, false, false, "Security error on load"));
			trace("A SecurityError has occurred.");
		}

        observeTimeout(timeout);
      }
    }

    public var observeTimer:Timer;
    private function observeTimeout(sec:Number):void
    {
        observeTimer = new Timer(sec * 1000, 1);
        observeTimer.addEventListener(
           TimerEvent.TIMER, timeoutError, false, 1, true);
        observeTimer.start();
    }
    private function timeoutError(e:TimerEvent):void
        {
                _fault = null;
                this._response.close();
                dispatchEvent(new ErrorEvent(ErrorEvent.ERROR, false, false, "Operation timed out after " + observeTimer.delay/1000 + " seconds"));
                //dispatchEvent(new IOErrorEvent(IOErrorEvent.IO_ERROR, false, false, 'request timeout'));
                observeTimer = null;
        }

    private function _onLoad( evt:Event ):void
    {
		try
		{
	        if (observeTimer) {
	                observeTimer.removeEventListener(TimerEvent.TIMER, timeoutError);
	            }
	      var responseXML:XML = new XML(this._response.data);
	      _fault = null;
		  if (this._response.bytesLoaded == 0)
		  {
			  trace("XMLRPC Fault: No data");
			  dispatchEvent(new ErrorEvent(ErrorEvent.ERROR, false, false, "No data"));
		  }
		  else if (responseXML.fault.length() > 0)
	      {
	        // fault
	        var parsedFault:Object = parseResponse(responseXML.fault.value.*[0]);
	        _fault = new MethodFaultImpl( parsedFault );
	        trace("XMLRPC Fault (" + _fault.getFaultCode() + "):\n"
	              + _fault.getFaultString());
	
	        dispatchEvent(new ErrorEvent(ErrorEvent.ERROR));
	      }
	      else if (responseXML.params)
	      {
	        _parsed_response = parseResponse(responseXML.params.param.value[0]);
	
	        dispatchEvent(new Event(Event.COMPLETE));
	      }
	      else
	      {
	        dispatchEvent(new ErrorEvent(ErrorEvent.ERROR));
	      }
		}
		catch (err:Error)
		{
			dispatchEvent(new ErrorEvent(ErrorEvent.ERROR));
		}
			
    }

    protected function ioError(event : IOErrorEvent) : void
    {
      _fault = null;
      dispatchEvent(new ErrorEvent(ErrorEvent.ERROR, false, false,
                                   event.toString()));
    }

    protected function securityError(event : SecurityErrorEvent) : void
    {
      _fault = null;
      dispatchEvent(event);
    }
	
	protected function networkError(event:IOErrorEvent) : void
	{
		_fault = null;
		dispatchEvent(event);
	}

    private function parseResponse(xml:XML):Object
    {
      return this._parser.parse( xml );
    }

    //public function __resolve( method:String ):void { this._call( method ); }

    public function getUrl() : String
    {
      return this._url;
    }

    public function setUrl(a : String) : void
    {
      this._url = a;
    }

    public function addParam(o : Object, type : String) : void
    {
      this._method.addParam( o,type );
    }

    public function removeParams() : void
    {
      this._method.removeParams();
    }

    public function getResponse() : Object
    {
      return this._parsed_response;
    }

    public function getFault() : MethodFault
    {
      return this._fault;
    }

    override public function toString() : String
    {
      return '<xmlrpc.ConnectionImpl Object>';
    }

    private function debug(a : String) : void
    {
      /*trace( this._PRODUCT + " -> " + a );*/
    }
  }
}
